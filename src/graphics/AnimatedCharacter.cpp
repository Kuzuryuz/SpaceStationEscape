#include "AnimatedCharacter.h"

#include "Shader.h"
#include "Texture.h"

#include <assimp/Importer.hpp>
#include <assimp/matrix4x4.h>
#include <assimp/postprocess.h>
#include <assimp/quaternion.h>
#include <assimp/scene.h>
#include <assimp/vector3.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
#include <stdexcept>

namespace
{
    const aiAnimation* findAnimationByName(const aiScene* scene, const std::string& wantedName)
    {
        if (!scene)
            return nullptr;

        for (unsigned int i = 0; i < scene->mNumAnimations; ++i)
        {
            const aiAnimation* animation = scene->mAnimations[i];
            if (animation && wantedName == animation->mName.C_Str())
                return animation;
        }

        return scene->mNumAnimations > 0 ? scene->mAnimations[0] : nullptr;
    }
}

AnimatedCharacter::AnimatedCharacter(const std::string& modelPath, const std::string& texturePath)
{
    try
    {
        loadModel(modelPath, texturePath);
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

void AnimatedCharacter::update(float deltaSeconds, bool isWalking)
{
    if (!loaded || !scene)
        return;

    if (walking != isWalking)
    {
        walking = isWalking;
        clipTimeSeconds = 0.0;
    }
    else
    {
        clipTimeSeconds += deltaSeconds;
    }

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
    float userScale = 3.0f;
    model = glm::translate(model, worldPosition + glm::vec3(0.0f, -0.25f, 0.0f));
    model = glm::rotate(model, glm::radians(worldYawDegrees + 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
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

void AnimatedCharacter::loadModel(const std::string& modelPath, const std::string& texturePath)
{
    importer = std::make_unique<Assimp::Importer>();
    scene = importer->ReadFile(
        modelPath,
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenSmoothNormals |
        aiProcess_LimitBoneWeights |
        aiProcess_ImproveCacheLocality |
        aiProcess_FlipUVs);

    if (!scene || !scene->mRootNode || scene->mNumMeshes == 0)
        throw std::runtime_error("Failed to load FBX scene: " + std::string(importer->GetErrorString()));

    glm::mat4 skeletonGlobalTransform(1.0f);
    if (findNodeByName(scene->mRootNode, "Armature", glm::mat4(1.0f), skeletonGlobalTransform))
        skeletonInverseTransform = glm::inverse(skeletonGlobalTransform);
    else
        skeletonInverseTransform = glm::mat4(1.0f);

    extractMesh(scene->mMeshes[0]);
    normalizeVertexWeights();
    renderVertices.resize(baseVertices.size());

    for (size_t i = 0; i < baseVertices.size(); ++i)
    {
        renderVertices[i].position = baseVertices[i].position;
        renderVertices[i].normal = baseVertices[i].normal;
        renderVertices[i].uv = baseVertices[i].uv;
    }

    texture = std::make_unique<Texture>(texturePath);
    setupMesh();
    updateAnimationPose();
    loaded = true;
}

bool AnimatedCharacter::findNodeForMesh(const aiNode* node, unsigned int meshIndex, const glm::mat4& parentTransform, glm::mat4& outGlobalTransform) const
{
    if (!node)
        return false;

    const glm::mat4 currentTransform = parentTransform * toGlm(node->mTransformation);

    for (unsigned int i = 0; i < node->mNumMeshes; ++i)
    {
        if (node->mMeshes[i] == meshIndex)
        {
            outGlobalTransform = currentTransform;
            return true;
        }
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        if (findNodeForMesh(node->mChildren[i], meshIndex, currentTransform, outGlobalTransform))
            return true;
    }

    return false;
}

bool AnimatedCharacter::findNodeByName(const aiNode* node, const std::string& nodeName, const glm::mat4& parentTransform, glm::mat4& outGlobalTransform) const
{
    if (!node)
        return false;

    const glm::mat4 currentTransform = parentTransform * toGlm(node->mTransformation);
    if (nodeName == node->mName.C_Str())
    {
        outGlobalTransform = currentTransform;
        return true;
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        if (findNodeByName(node->mChildren[i], nodeName, currentTransform, outGlobalTransform))
            return true;
    }

    return false;
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

void AnimatedCharacter::extractMesh(aiMesh* mesh)
{
    if (!mesh)
        throw std::runtime_error("Animated character mesh is missing");

    baseVertices.resize(mesh->mNumVertices);

    for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
    {
        Vertex vertex;
        vertex.position = toGlm(mesh->mVertices[i]);
        if (mesh->HasNormals())
            vertex.normal = glm::normalize(toGlm(mesh->mNormals[i]));
        if (mesh->HasTextureCoords(0))
            vertex.uv = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        baseVertices[i] = vertex;
    }

    indices.clear();
    indices.reserve(mesh->mNumFaces * 3);
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
    {
        const aiFace& face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j)
            indices.push_back(face.mIndices[j]);
    }

    readBoneWeights(mesh);
}

void AnimatedCharacter::readBoneWeights(aiMesh* mesh)
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
            if (weight.mVertexId < baseVertices.size())
                setVertexBoneData(baseVertices[weight.mVertexId], mappedBoneIndex, weight.mWeight);
        }
    }

    finalBoneTransforms.assign(boneInfos.size(), glm::mat4(1.0f));
}

void AnimatedCharacter::updateAnimationPose()
{
    const aiAnimation* animation = findAnimationByName(scene, "Armature|ArmatureAction");
    if (!animation)
        return;

    const double ticksPerSecond = animation->mTicksPerSecond > 0.0 ? animation->mTicksPerSecond : 30.0;
    const double clipStartSeconds = (walking ? walkClip.startFrame : idleClip.startFrame) / 30.0;
    const double clipEndSeconds = (walking ? walkClip.endFrame : idleClip.endFrame) / 30.0;
    const double clipDurationSeconds = std::max(clipEndSeconds - clipStartSeconds, 1.0 / 30.0);
    const double localClipSeconds = std::fmod(clipTimeSeconds, clipDurationSeconds);
    const double animationTimeTicks = (clipStartSeconds + localClipSeconds) * ticksPerSecond;

    readNodeHierarchy(animationTimeTicks, scene->mRootNode, glm::mat4(1.0f));

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

    float scaleX = (extents.x > 0.0001f) ? targetExtents.x / extents.x : 1.0f;
    float scaleY = (extents.y > 0.0001f) ? targetExtents.y / extents.y : 1.0f;
    float scaleZ = (extents.z > 0.0001f) ? targetExtents.z / extents.z : 1.0f;

    fittedScale = std::min(scaleX, std::min(scaleY, scaleZ));
    fittedBasePivot = glm::vec3(
        (minBounds.x + maxBounds.x) * 0.5f,
        minBounds.y,
        (minBounds.z + maxBounds.z) * 0.5f
    );
}

void AnimatedCharacter::readNodeHierarchy(double animationTimeTicks, const aiNode* node, const glm::mat4& parentTransform)
{
    if (!node)
        return;

    const aiAnimation* animation = findAnimationByName(scene, "Armature|ArmatureAction");
    glm::mat4 nodeTransform = toGlm(node->mTransformation);

    if (animation)
    {
        const aiNodeAnim* nodeAnim = findNodeAnim(animation, node->mName.C_Str());
        if (nodeAnim)
        {
            const glm::vec3 scaling = interpolateScaling(animationTimeTicks, nodeAnim);
            const glm::quat rotation = interpolateRotation(animationTimeTicks, nodeAnim);
            const glm::vec3 translation = interpolatePosition(animationTimeTicks, nodeAnim);

            nodeTransform = glm::translate(glm::mat4(1.0f), translation)
                * glm::toMat4(rotation)
                * glm::scale(glm::mat4(1.0f), scaling);
        }
    }

    const glm::mat4 globalTransform = parentTransform * nodeTransform;

    auto it = boneMapping.find(node->mName.C_Str());
    if (it != boneMapping.end())
    {
        const int boneIndex = it->second;
        finalBoneTransforms[boneIndex] = skeletonInverseTransform * globalTransform * boneInfos[boneIndex].offset;
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i)
        readNodeHierarchy(animationTimeTicks, node->mChildren[i], globalTransform);
}

const aiNodeAnim* AnimatedCharacter::findNodeAnim(const aiAnimation* animation, const std::string& nodeName) const
{
    if (!animation)
        return nullptr;

    for (unsigned int i = 0; i < animation->mNumChannels; ++i)
    {
        const aiNodeAnim* channel = animation->mChannels[i];
        if (channel && nodeName == channel->mNodeName.C_Str())
            return channel;
    }
    return nullptr;
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
            const float factor = span > 0.0 ? static_cast<float>((animationTimeTicks - current.mTime) / span) : 0.0f;
            return glm::mix(toGlm(current.mValue), toGlm(next.mValue), glm::clamp(factor, 0.0f, 1.0f));
        }
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
            const float factor = span > 0.0 ? static_cast<float>((animationTimeTicks - current.mTime) / span) : 0.0f;
            return glm::normalize(glm::slerp(
                toGlm(current.mValue),
                toGlm(next.mValue),
                glm::clamp(factor, 0.0f, 1.0f)));
        }
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
            const float factor = span > 0.0 ? static_cast<float>((animationTimeTicks - current.mTime) / span) : 0.0f;
            return glm::mix(toGlm(current.mValue), toGlm(next.mValue), glm::clamp(factor, 0.0f, 1.0f));
        }
    }

    return toGlm(nodeAnim->mScalingKeys[nodeAnim->mNumScalingKeys - 1].mValue);
}

glm::mat4 AnimatedCharacter::toGlm(const aiMatrix4x4& matrix)
{
    glm::mat4 result;
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
