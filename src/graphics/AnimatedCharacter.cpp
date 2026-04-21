#include "AnimatedCharacter.h"

#include "Shader.h"
#include "Texture.h"

#include <assimp/postprocess.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <numeric>
#include <stdexcept>

namespace
{
    constexpr unsigned int kAssimpFlags =
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenSmoothNormals |
        aiProcess_LimitBoneWeights |
        aiProcess_ImproveCacheLocality |
        aiProcess_FlipUVs;
}

AnimatedCharacter::AnimatedCharacter(const std::string& modelPath, const std::string& texturePath, bool shouldLoop)
    : looping(shouldLoop)
{
    try
    {
        loadModel(modelPath, std::string(), texturePath);
        clipTimeSeconds = 0.0;
        finished = false;
        updateAnimationPose();
        loaded = true;
    }
    catch (const std::exception& ex)
    {
        loadError = ex.what();
        std::cerr << "AnimatedCharacter load failed: " << loadError << "\n";
    }
}

AnimatedCharacter::AnimatedCharacter(
    const std::string& modelPath,
    const std::string& animationPath,
    const std::string& texturePath,
    bool shouldLoop)
    : looping(shouldLoop)
{
    try
    {
        loadModel(modelPath, animationPath, texturePath);
        clipTimeSeconds = 0.0;
        finished = false;
        updateAnimationPose();
        loaded = true;
    }
    catch (const std::exception& ex)
    {
        loadError = ex.what();
        std::cerr << "AnimatedCharacter load failed: " << loadError << "\n";
    }
}

AnimatedCharacter::~AnimatedCharacter()
{
    if (ebo != 0)
        glDeleteBuffers(1, &ebo);
    if (vbo != 0)
        glDeleteBuffers(1, &vbo);
    if (vao != 0)
        glDeleteVertexArrays(1, &vao);
}

void AnimatedCharacter::update(float deltaSeconds, bool restart)
{
    if (!loaded || !animationScene || !animation)
        return;

    if (restart)
    {
        clipTimeSeconds = 0.0;
        finished = false;
    }
    else if (!finished)
    {
        clipTimeSeconds += deltaSeconds;

        if (!looping && clipTimeSeconds >= getAnimationDurationSeconds())
        {
            clipTimeSeconds = getAnimationDurationSeconds();
            finished = true;
        }
    }

    updateAnimationPose();
}

double AnimatedCharacter::getNormalizedTime() const
{
    const double durationSeconds = getAnimationDurationSeconds();
    if (durationSeconds <= 0.0)
        return 0.0;

    if (looping)
        return std::fmod(clipTimeSeconds, durationSeconds) / durationSeconds;

    return std::clamp(clipTimeSeconds / durationSeconds, 0.0, 1.0);
}

void AnimatedCharacter::setNormalizedTime(double normalizedTime)
{
    const double durationSeconds = getAnimationDurationSeconds();
    if (durationSeconds <= 0.0)
        return;

    const double clampedNormalizedTime = looping
        ? std::fmod(std::fmod(normalizedTime, 1.0) + 1.0, 1.0)
        : std::clamp(normalizedTime, 0.0, 1.0);

    clipTimeSeconds = clampedNormalizedTime * durationSeconds;
    finished = !looping && clipTimeSeconds >= durationSeconds;
    updateAnimationPose();
}

void AnimatedCharacter::draw(
    Shader& shader,
    const glm::mat4& view,
    const glm::mat4& projection,
    const glm::vec3& worldPosition,
    float worldYawDegrees) const
{
    if (!loaded)
        return;

    glm::mat4 model = glm::mat4(1.0f);
    const float userScale = 3.0f;

    model = glm::translate(model, worldPosition);
    model = glm::rotate(model, glm::radians(worldYawDegrees + 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::translate(model, -fittedBasePivot);
    model = glm::scale(model, glm::vec3(fittedScale * userScale));

    shader.use();
    shader.setMat4("model", model);
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);
    shader.setVec3("lightDir", glm::normalize(glm::vec3(-0.4f, -1.0f, -0.25f)));
    shader.setVec3("tintColor", glm::vec3(1.0f));
    shader.setInt("diffuseTexture", 0);

    if (texture && texture->isLoaded())
        texture->bind(0);

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void AnimatedCharacter::loadModel(
    const std::string& modelPath,
    const std::string& animationPath,
    const std::string& texturePath)
{
    modelImporter = std::make_unique<Assimp::Importer>();
    modelScene = modelImporter->ReadFile(modelPath, kAssimpFlags);

    if (!modelScene || !modelScene->mRootNode || modelScene->mNumMeshes == 0)
        throw std::runtime_error("Failed to load model scene: " + std::string(modelImporter->GetErrorString()));

    if (animationPath.empty())
    {
        animationScene = modelScene;

        if (modelScene->mNumAnimations == 0)
            throw std::runtime_error("Animated character file has no embedded animation: " + modelPath);
    }
    else
    {
        animationImporter = std::make_unique<Assimp::Importer>();
        animationScene = animationImporter->ReadFile(animationPath, kAssimpFlags);

        if (!animationScene || !animationScene->mRootNode)
            throw std::runtime_error("Failed to load animation scene: " + std::string(animationImporter->GetErrorString()));
        if (animationScene->mNumAnimations == 0)
            throw std::runtime_error("Animation file has no animation clip: " + animationPath);
    }

    animation = animationScene->mAnimations[animationScene->mNumAnimations - 1];

    const unsigned int selectedAnimationIndex = animationScene->mNumAnimations - 1;

    std::cout << "AnimatedCharacter source: " << modelPath;
    if (!animationPath.empty())
        std::cout << " | animation: " << animationPath;
    std::cout << " | animations=" << animationScene->mNumAnimations
        << " | selectedIndex=" << selectedAnimationIndex
        << " | channels=" << animation->mNumChannels
        << " | duration=" << animation->mDuration
        << " | ticksPerSecond=" << animation->mTicksPerSecond
        << "\n";

    baseVertices.clear();
    renderVertices.clear();
    indices.clear();
    boneMapping.clear();
    boneInfos.clear();
    finalBoneTransforms.clear();

    bool foundSkinnedMesh = false;

    for (unsigned int meshIndex = 0; meshIndex < modelScene->mNumMeshes; ++meshIndex)
    {
        aiMesh* mesh = modelScene->mMeshes[meshIndex];
        if (!mesh || mesh->mNumVertices == 0)
            continue;

        if (mesh->mNumBones > 0)
            foundSkinnedMesh = true;

        extractMesh(mesh, static_cast<unsigned int>(baseVertices.size()));
    }

    if (!foundSkinnedMesh)
        throw std::runtime_error("Model has no skinned mesh/bones: " + modelPath);

    normalizeVertexWeights();

    renderVertices.resize(baseVertices.size());
    for (size_t i = 0; i < baseVertices.size(); ++i)
    {
        renderVertices[i].position = baseVertices[i].position;
        renderVertices[i].normal = baseVertices[i].normal;
        renderVertices[i].uv = baseVertices[i].uv;
    }

    animationRootNode = animationScene->mRootNode;
    if (!animationRootNode)
        throw std::runtime_error("Animation scene has no root node.");

    skeletonInverseTransform = glm::inverse(toGlm(animationRootNode->mTransformation));

    texture = std::make_unique<Texture>(texturePath);
    setupMesh();

    std::cout << "AnimatedCharacter loaded. Model meshes=" << modelScene->mNumMeshes
        << ", bones=" << boneInfos.size()
        << ", animation='" << animation->mName.C_Str() << "'"
        << ", animation source=" << (animationPath.empty() ? "embedded" : animationPath)
        << "\n";
}

void AnimatedCharacter::setupMesh()
{
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(renderVertices.size() * sizeof(RenderVertex)),
        renderVertices.data(),
        GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
        indices.data(),
        GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RenderVertex), reinterpret_cast<void*>(offsetof(RenderVertex, position)));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(RenderVertex), reinterpret_cast<void*>(offsetof(RenderVertex, normal)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(RenderVertex), reinterpret_cast<void*>(offsetof(RenderVertex, uv)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void AnimatedCharacter::extractMesh(aiMesh* mesh, unsigned int baseVertex)
{
    if (!mesh)
        throw std::runtime_error("Animated character mesh is missing");

    const size_t oldVertexCount = baseVertices.size();
    baseVertices.resize(oldVertexCount + mesh->mNumVertices);

    for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
    {
        Vertex vertex;
        vertex.position = toGlm(mesh->mVertices[i]);

        if (mesh->HasNormals())
            vertex.normal = glm::normalize(toGlm(mesh->mNormals[i]));

        if (mesh->HasTextureCoords(0))
            vertex.uv = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);

        baseVertices[oldVertexCount + i] = vertex;
    }

    indices.reserve(indices.size() + mesh->mNumFaces * 3);
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
    {
        const aiFace& face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j)
            indices.push_back(baseVertex + face.mIndices[j]);
    }

    readBoneWeights(mesh, baseVertex);
}

void AnimatedCharacter::readBoneWeights(aiMesh* mesh, unsigned int baseVertex)
{
    for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
    {
        aiBone* bone = mesh->mBones[boneIndex];
        const std::string boneName = bone->mName.C_Str();

        int mappedBoneIndex = 0;
        auto it = boneMapping.find(boneName);

        if (it == boneMapping.end())
        {
            mappedBoneIndex = static_cast<int>(boneInfos.size());
            boneMapping[boneName] = mappedBoneIndex;
            boneInfos.push_back({ toGlm(bone->mOffsetMatrix) });
        }
        else
        {
            mappedBoneIndex = it->second;
        }

        for (unsigned int weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex)
        {
            const aiVertexWeight& weight = bone->mWeights[weightIndex];
            const unsigned int vertexIndex = baseVertex + weight.mVertexId;

            if (vertexIndex < baseVertices.size())
                setVertexBoneData(baseVertices[vertexIndex], mappedBoneIndex, weight.mWeight);
        }
    }

    finalBoneTransforms.assign(boneInfos.size(), glm::mat4(1.0f));
}

void AnimatedCharacter::updateAnimationPose()
{
    if (!animation || !animationScene || !animationRootNode)
        return;

    const double ticksPerSecond = animation->mTicksPerSecond > 0.0 ? animation->mTicksPerSecond : 30.0;
    const double durationSeconds = getAnimationDurationSeconds();
    const double localClipSeconds = looping
        ? std::fmod(clipTimeSeconds, durationSeconds)
        : std::min(clipTimeSeconds, durationSeconds);

    const double animationTimeTicks = localClipSeconds * ticksPerSecond;

    std::fill(finalBoneTransforms.begin(), finalBoneTransforms.end(), glm::mat4(1.0f));
    readNodeHierarchy(animationTimeTicks, animationRootNode, glm::mat4(1.0f));

    for (size_t i = 0; i < baseVertices.size(); ++i)
    {
        const Vertex& source = baseVertices[i];
        glm::vec4 skinnedPosition(0.0f);
        glm::vec3 skinnedNormal(0.0f);

        for (int j = 0; j < kMaxWeightsPerVertex; ++j)
        {
            const float weight = source.boneWeights[j];
            if (weight <= 0.0f)
                continue;

            const int boneId = source.boneIds[j];
            if (boneId < 0 || boneId >= static_cast<int>(finalBoneTransforms.size()))
                continue;

            const glm::mat4 boneTransform = finalBoneTransforms[boneId];
            skinnedPosition += boneTransform * glm::vec4(source.position, 1.0f) * weight;
            skinnedNormal += glm::mat3(boneTransform) * source.normal * weight;
        }

        if (glm::length(skinnedPosition) < 0.0001f)
            skinnedPosition = glm::vec4(source.position, 1.0f);

        if (glm::length(skinnedNormal) < 0.0001f)
            skinnedNormal = source.normal;

        renderVertices[i].position = glm::vec3(skinnedPosition);
        renderVertices[i].normal = glm::normalize(skinnedNormal);
        renderVertices[i].uv = source.uv;
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        static_cast<GLsizeiptr>(renderVertices.size() * sizeof(RenderVertex)),
        renderVertices.data());

    updateFittedBounds();
}

void AnimatedCharacter::updateFittedBounds()
{
    if (renderVertices.empty())
        return;

    glm::vec3 minBounds = renderVertices[0].position;
    glm::vec3 maxBounds = renderVertices[0].position;

    for (const auto& vertex : renderVertices)
    {
        minBounds = glm::min(minBounds, vertex.position);
        maxBounds = glm::max(maxBounds, vertex.position);
    }

    const glm::vec3 extents = maxBounds - minBounds;
    const glm::vec3 targetExtents(0.6f, 1.0f, 0.6f);

    const float scaleX = (extents.x > 0.0001f) ? targetExtents.x / extents.x : 1.0f;
    const float scaleY = (extents.y > 0.0001f) ? targetExtents.y / extents.y : 1.0f;
    const float scaleZ = (extents.z > 0.0001f) ? targetExtents.z / extents.z : 1.0f;

    fittedScale = std::min(scaleX, std::min(scaleY, scaleZ));
    fittedBasePivot = glm::vec3(
        (minBounds.x + maxBounds.x) * 0.5f,
        minBounds.y,
        (minBounds.z + maxBounds.z) * 0.5f);
}

void AnimatedCharacter::readNodeHierarchy(double animationTimeTicks, const aiNode* node, const glm::mat4& parentTransform)
{
    if (!node)
        return;

    glm::mat4 nodeTransform = toGlm(node->mTransformation);
    const glm::vec3 bindTranslation = glm::vec3(nodeTransform[3]);
    const aiNodeAnim* nodeAnim = findNodeAnim(animation, node->mName.C_Str());

    if (nodeAnim)
    {
        const glm::vec3 scaling = interpolateScaling(animationTimeTicks, nodeAnim);
        const glm::quat rotation = interpolateRotation(animationTimeTicks, nodeAnim);
        glm::vec3 translation = interpolatePosition(animationTimeTicks, nodeAnim);

        const std::string nodeName = node->mName.C_Str();
        const bool isRootMotionNode =
            nodeName == "mixamorig:Hips" ||
            nodeName == "Hips" ||
            nodeName == "Armature";

        if (looping && isRootMotionNode)
        {
            // Keep the locomotion cycle planted in place and let gameplay code move the character.
            translation.x = bindTranslation.x;
            translation.z = bindTranslation.z;
        }

        nodeTransform =
            glm::translate(glm::mat4(1.0f), translation) *
            glm::toMat4(rotation) *
            glm::scale(glm::mat4(1.0f), scaling);
    }

    const glm::mat4 globalTransform = parentTransform * nodeTransform;

    auto it = boneMapping.find(node->mName.C_Str());
    if (it != boneMapping.end())
    {
        const int boneIndex = it->second;
        finalBoneTransforms[boneIndex] =
            skeletonInverseTransform * globalTransform * boneInfos[boneIndex].offset;
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i)
        readNodeHierarchy(animationTimeTicks, node->mChildren[i], globalTransform);
}

const aiNodeAnim* AnimatedCharacter::findNodeAnim(const aiAnimation* anim, const std::string& nodeName) const
{
    if (!anim)
        return nullptr;

    for (unsigned int i = 0; i < anim->mNumChannels; ++i)
    {
        const aiNodeAnim* channel = anim->mChannels[i];
        if (channel && nodeName == channel->mNodeName.C_Str())
            return channel;
    }

    return nullptr;
}

double AnimatedCharacter::getAnimationDurationSeconds() const
{
    if (!animation)
        return 1.0 / 30.0;

    const double ticksPerSecond = animation->mTicksPerSecond > 0.0 ? animation->mTicksPerSecond : 30.0;
    return std::max(animation->mDuration / ticksPerSecond, 1.0 / 30.0);
}

glm::vec3 AnimatedCharacter::interpolatePosition(double animationTimeTicks, const aiNodeAnim* nodeAnim) const
{
    if (nodeAnim->mNumPositionKeys == 0)
        return glm::vec3(0.0f);

    if (nodeAnim->mNumPositionKeys == 1)
        return toGlm(nodeAnim->mPositionKeys[0].mValue);

    for (unsigned int i = 0; i < nodeAnim->mNumPositionKeys - 1; ++i)
    {
        const aiVectorKey& current = nodeAnim->mPositionKeys[i];
        const aiVectorKey& next = nodeAnim->mPositionKeys[i + 1];

        if (animationTimeTicks < next.mTime)
        {
            const double span = next.mTime - current.mTime;
            const float factor = span > 0.0
                ? static_cast<float>((animationTimeTicks - current.mTime) / span)
                : 0.0f;

            return glm::mix(
                toGlm(current.mValue),
                toGlm(next.mValue),
                glm::clamp(factor, 0.0f, 1.0f));
        }
    }

    if (looping && animation && nodeAnim->mNumPositionKeys > 1)
    {
        const aiVectorKey& last = nodeAnim->mPositionKeys[nodeAnim->mNumPositionKeys - 1];
        const aiVectorKey& first = nodeAnim->mPositionKeys[0];
        const double wrappedNextTime = animation->mDuration + first.mTime;
        const double span = wrappedNextTime - last.mTime;
        const float factor = span > 0.0
            ? static_cast<float>((animationTimeTicks - last.mTime) / span)
            : 0.0f;

        return glm::mix(
            toGlm(last.mValue),
            toGlm(first.mValue),
            glm::clamp(factor, 0.0f, 1.0f));
    }

    return toGlm(nodeAnim->mPositionKeys[nodeAnim->mNumPositionKeys - 1].mValue);
}

glm::quat AnimatedCharacter::interpolateRotation(double animationTimeTicks, const aiNodeAnim* nodeAnim) const
{
    if (nodeAnim->mNumRotationKeys == 0)
        return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    if (nodeAnim->mNumRotationKeys == 1)
        return glm::normalize(toGlm(nodeAnim->mRotationKeys[0].mValue));

    for (unsigned int i = 0; i < nodeAnim->mNumRotationKeys - 1; ++i)
    {
        const aiQuatKey& current = nodeAnim->mRotationKeys[i];
        const aiQuatKey& next = nodeAnim->mRotationKeys[i + 1];

        if (animationTimeTicks < next.mTime)
        {
            const double span = next.mTime - current.mTime;
            const float factor = span > 0.0
                ? static_cast<float>((animationTimeTicks - current.mTime) / span)
                : 0.0f;

            return glm::normalize(glm::slerp(
                toGlm(current.mValue),
                toGlm(next.mValue),
                glm::clamp(factor, 0.0f, 1.0f)));
        }
    }

    if (looping && animation && nodeAnim->mNumRotationKeys > 1)
    {
        const aiQuatKey& last = nodeAnim->mRotationKeys[nodeAnim->mNumRotationKeys - 1];
        const aiQuatKey& first = nodeAnim->mRotationKeys[0];
        const double wrappedNextTime = animation->mDuration + first.mTime;
        const double span = wrappedNextTime - last.mTime;
        const float factor = span > 0.0
            ? static_cast<float>((animationTimeTicks - last.mTime) / span)
            : 0.0f;

        return glm::normalize(glm::slerp(
            toGlm(last.mValue),
            toGlm(first.mValue),
            glm::clamp(factor, 0.0f, 1.0f)));
    }

    return glm::normalize(toGlm(nodeAnim->mRotationKeys[nodeAnim->mNumRotationKeys - 1].mValue));
}

glm::vec3 AnimatedCharacter::interpolateScaling(double animationTimeTicks, const aiNodeAnim* nodeAnim) const
{
    if (nodeAnim->mNumScalingKeys == 0)
        return glm::vec3(1.0f);

    if (nodeAnim->mNumScalingKeys == 1)
        return toGlm(nodeAnim->mScalingKeys[0].mValue);

    for (unsigned int i = 0; i < nodeAnim->mNumScalingKeys - 1; ++i)
    {
        const aiVectorKey& current = nodeAnim->mScalingKeys[i];
        const aiVectorKey& next = nodeAnim->mScalingKeys[i + 1];

        if (animationTimeTicks < next.mTime)
        {
            const double span = next.mTime - current.mTime;
            const float factor = span > 0.0
                ? static_cast<float>((animationTimeTicks - current.mTime) / span)
                : 0.0f;

            return glm::mix(
                toGlm(current.mValue),
                toGlm(next.mValue),
                glm::clamp(factor, 0.0f, 1.0f));
        }
    }

    if (looping && animation && nodeAnim->mNumScalingKeys > 1)
    {
        const aiVectorKey& last = nodeAnim->mScalingKeys[nodeAnim->mNumScalingKeys - 1];
        const aiVectorKey& first = nodeAnim->mScalingKeys[0];
        const double wrappedNextTime = animation->mDuration + first.mTime;
        const double span = wrappedNextTime - last.mTime;
        const float factor = span > 0.0
            ? static_cast<float>((animationTimeTicks - last.mTime) / span)
            : 0.0f;

        return glm::mix(
            toGlm(last.mValue),
            toGlm(first.mValue),
            glm::clamp(factor, 0.0f, 1.0f));
    }

    return toGlm(nodeAnim->mScalingKeys[nodeAnim->mNumScalingKeys - 1].mValue);
}

const aiNode* AnimatedCharacter::findNodeByName(const aiNode* root, const std::string& nodeName) const
{
    if (!root)
        return nullptr;

    if (nodeName == root->mName.C_Str())
        return root;

    for (unsigned int i = 0; i < root->mNumChildren; ++i)
    {
        if (const aiNode* child = findNodeByName(root->mChildren[i], nodeName))
            return child;
    }

    return nullptr;
}

const aiNode* AnimatedCharacter::findSkeletonRoot(const aiScene* targetScene) const
{
    if (!targetScene || !targetScene->mRootNode || boneMapping.empty())
        return nullptr;

    std::vector<const aiNode*> commonPath;
    bool firstBone = true;

    for (const auto& pair : boneMapping)
    {
        const aiNode* boneNode = findNodeByName(targetScene->mRootNode, pair.first);
        if (!boneNode)
            continue;

        std::vector<const aiNode*> path;
        for (const aiNode* cursor = boneNode; cursor != nullptr; cursor = cursor->mParent)
            path.push_back(cursor);

        std::reverse(path.begin(), path.end());

        if (firstBone)
        {
            commonPath = path;
            firstBone = false;
            continue;
        }

        size_t sharedLength = 0;
        const size_t compareLength = std::min(commonPath.size(), path.size());

        while (sharedLength < compareLength && commonPath[sharedLength] == path[sharedLength])
            ++sharedLength;

        commonPath.resize(sharedLength);
    }

    if (commonPath.empty())
        return nullptr;

    return commonPath.back();
}

glm::mat4 AnimatedCharacter::computeNodeGlobalTransform(const aiNode* node) const
{
    glm::mat4 global(1.0f);
    std::vector<const aiNode*> path;

    for (const aiNode* cursor = node; cursor != nullptr; cursor = cursor->mParent)
        path.push_back(cursor);

    for (auto it = path.rbegin(); it != path.rend(); ++it)
        global *= toGlm((*it)->mTransformation);

    return global;
}

glm::mat4 AnimatedCharacter::toGlm(const aiMatrix4x4& matrix)
{
    glm::mat4 result(1.0f);

    result[0][0] = matrix.a1; result[1][0] = matrix.a2; result[2][0] = matrix.a3; result[3][0] = matrix.a4;
    result[0][1] = matrix.b1; result[1][1] = matrix.b2; result[2][1] = matrix.b3; result[3][1] = matrix.b4;
    result[0][2] = matrix.c1; result[1][2] = matrix.c2; result[2][2] = matrix.c3; result[3][2] = matrix.c4;
    result[0][3] = matrix.d1; result[1][3] = matrix.d2; result[2][3] = matrix.d3; result[3][3] = matrix.d4;

    return result;
}

glm::vec3 AnimatedCharacter::toGlm(const aiVector3D& value)
{
    return glm::vec3(value.x, value.y, value.z);
}

glm::quat AnimatedCharacter::toGlm(const aiQuaternion& value)
{
    return glm::quat(value.w, value.x, value.y, value.z);
}

void AnimatedCharacter::setVertexBoneData(Vertex& vertex, int boneId, float weight)
{
    int weakestIndex = 0;

    for (int i = 0; i < kMaxWeightsPerVertex; ++i)
    {
        if (vertex.boneWeights[i] == 0.0f)
        {
            vertex.boneIds[i] = boneId;
            vertex.boneWeights[i] = weight;
            return;
        }

        if (vertex.boneWeights[i] < vertex.boneWeights[weakestIndex])
            weakestIndex = i;
    }

    if (weight > vertex.boneWeights[weakestIndex])
    {
        vertex.boneIds[weakestIndex] = boneId;
        vertex.boneWeights[weakestIndex] = weight;
    }
}

void AnimatedCharacter::normalizeVertexWeights()
{
    for (auto& vertex : baseVertices)
    {
        const float totalWeight = std::accumulate(
            std::begin(vertex.boneWeights),
            std::end(vertex.boneWeights),
            0.0f);

        if (totalWeight <= 0.0f)
            continue;

        for (float& weight : vertex.boneWeights)
            weight /= totalWeight;
    }
}
