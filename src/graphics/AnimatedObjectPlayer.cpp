#include "AnimatedObjectPlayer.h"

#include "Shader.h"
#include "Texture.h"

#include <assimp/postprocess.h>

#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace
{
    constexpr unsigned int kAnimatedObjectFlags =
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs;

    std::string toLowerCopy(const std::string& text)
    {
        auto toLower = [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); };

        std::string loweredText(text);
        std::transform(loweredText.begin(), loweredText.end(), loweredText.begin(), toLower);
        return loweredText;
    }

    bool isRingPartName(const std::string& name)
    {
        const std::string loweredName = toLowerCopy(name);
        if (loweredName == "ring" || loweredName.find("ring_mesh") != std::string::npos)
            return true;
        if (loweredName.find(".001") != std::string::npos)
            return true;
        return loweredName.find("ring") != std::string::npos &&
            loweredName.find("pipe") == std::string::npos;
    }
}

AnimatedObjectPlayer::AnimatedObjectPlayer(
    const std::string& modelPath,
    const std::string& texturePath,
    bool shouldLoop,
    const std::string& animationName)
    : looping(shouldLoop)
{
    try
    {
        load(modelPath, texturePath, animationName);
        update(0.0f, true);
        loaded = true;
    }
    catch (const std::exception& ex)
    {
        loadError = ex.what();
        std::cerr << "AnimatedObjectPlayer load failed: " << loadError << "\n";
    }
}

AnimatedObjectPlayer::~AnimatedObjectPlayer()
{
    for (auto& mesh : meshes)
    {
        if (mesh.ebo != 0)
            glDeleteBuffers(1, &mesh.ebo);
        if (mesh.vbo != 0)
            glDeleteBuffers(1, &mesh.vbo);
        if (mesh.vao != 0)
            glDeleteVertexArrays(1, &mesh.vao);
    }
}

void AnimatedObjectPlayer::update(float deltaSeconds, bool restart)
{
    if (!loaded || !animation)
        return;

    if (restart)
    {
        clipTimeSeconds = 0.0;
        finished = false;
        return;
    }

    if (finished)
        return;

    clipTimeSeconds += deltaSeconds;
    if (!looping && clipTimeSeconds >= getAnimationDurationSeconds())
    {
        clipTimeSeconds = getAnimationDurationSeconds();
        finished = true;
    }
}

void AnimatedObjectPlayer::draw(
    Shader& shader,
    const glm::mat4& view,
    const glm::mat4& projection,
    const glm::mat4& baseModel,
    const glm::vec3& tintColor) const
{
    if (!loaded || !scene || !scene->mRootNode)
        return;

    shader.use();
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);
    shader.setVec3("lightDir", glm::normalize(glm::vec3(-0.4f, -1.0f, -0.25f)));
    shader.setVec3("tintColor", tintColor);
    shader.setInt("diffuseTexture", 0);

    if (texture && texture->isLoaded())
        texture->bind(0);

    drawNode(shader, scene->mRootNode, glm::mat4(1.0f), baseModel, tintColor);
    shader.setInt("useSolidColor", 0);
}

void AnimatedObjectPlayer::load(
    const std::string& modelPath,
    const std::string& texturePath,
    const std::string& animationName)
{
    importer = std::make_unique<Assimp::Importer>();
    scene = importer->ReadFile(modelPath, kAnimatedObjectFlags);

    if (!scene || !scene->mRootNode || scene->mNumMeshes == 0)
        throw std::runtime_error("Failed to load animated object: " + std::string(importer->GetErrorString()));
    if (scene->mNumAnimations == 0)
        throw std::runtime_error("Animated object file has no animation clip: " + modelPath);

    animation = nullptr;
    for (unsigned int animationIndex = 0; animationIndex < scene->mNumAnimations; ++animationIndex)
    {
        const aiAnimation* candidateAnimation = scene->mAnimations[animationIndex];
        if (!candidateAnimation)
            continue;

        if (animationName.empty() || animationName == candidateAnimation->mName.C_Str())
        {
            animation = candidateAnimation;
            break;
        }
    }

    if (!animation)
    {
        throw std::runtime_error(
            "Animated object file does not contain requested animation: " +
            modelPath + " | animation='" + animationName + "'");
    }

    rootInverseTransform = glm::inverse(toGlm(scene->mRootNode->mTransformation));
    minBounds = glm::vec3(std::numeric_limits<float>::max());
    maxBounds = glm::vec3(std::numeric_limits<float>::lowest());

    meshes.reserve(scene->mNumMeshes);
    for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
        uploadMesh(scene->mMeshes[meshIndex]);

    minBounds = glm::vec3(std::numeric_limits<float>::max());
    maxBounds = glm::vec3(std::numeric_limits<float>::lowest());
    computeSceneBounds(scene->mRootNode, glm::mat4(1.0f), minBounds, maxBounds);

    const glm::vec3 extents = maxBounds - minBounds;
    const glm::vec3 targetExtents(0.35f, 0.5f, 0.4f);
    const float scaleX = (extents.x > 0.0001f) ? targetExtents.x / extents.x : 1.0f;
    const float scaleY = (extents.y > 0.0001f) ? targetExtents.y / extents.y : 1.0f;
    const float scaleZ = (extents.z > 0.0001f) ? targetExtents.z / extents.z : 1.0f;
    fittedScale = std::min(scaleX, std::min(scaleY, scaleZ));
    fittedBasePivot = glm::vec3(
        (minBounds.x + maxBounds.x) * 0.5f,
        minBounds.y,
        (minBounds.z + maxBounds.z) * 0.5f);
    normalizationTransform = glm::scale(glm::mat4(1.0f), glm::vec3(fittedScale)) *
        glm::translate(glm::mat4(1.0f), -fittedBasePivot);

    texture = std::make_unique<Texture>(texturePath);

    std::cout << "AnimatedObjectPlayer loaded: " << modelPath
        << " | meshes=" << scene->mNumMeshes
        << " | animations=" << scene->mNumAnimations
        << " | selectedAnimation='" << animation->mName.C_Str() << "'"
        << " | channels=" << animation->mNumChannels
        << " | durationSeconds=" << getAnimationDurationSeconds()
        << " | boundsMin=(" << minBounds.x << ", " << minBounds.y << ", " << minBounds.z << ")"
        << " | boundsMax=(" << maxBounds.x << ", " << maxBounds.y << ", " << maxBounds.z << ")"
        << " | size=(" << extents.x << ", " << extents.y << ", " << extents.z << ")"
        << " | fittedScale=" << fittedScale
        << "\n";
}

void AnimatedObjectPlayer::uploadMesh(const aiMesh* mesh)
{
    if (!mesh || mesh->mNumVertices == 0)
        throw std::runtime_error("Animated object mesh is missing");

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(mesh->mNumVertices);
    indices.reserve(mesh->mNumFaces * 3);
    glm::vec3 minBounds(std::numeric_limits<float>::max());
    glm::vec3 maxBounds(std::numeric_limits<float>::lowest());

    for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
    {
        Vertex vertex;
        vertex.position = toGlm(mesh->mVertices[i]);
        if (mesh->HasNormals())
            vertex.normal = glm::normalize(toGlm(mesh->mNormals[i]));
        if (mesh->HasTextureCoords(0))
            vertex.uv = glm::vec2(mesh->mTextureCoords[0][i].x, 1.0f - mesh->mTextureCoords[0][i].y);
        vertices.push_back(vertex);
        minBounds = glm::min(minBounds, vertex.position);
        maxBounds = glm::max(maxBounds, vertex.position);
    }

    for (unsigned int faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex)
    {
        const aiFace& face = mesh->mFaces[faceIndex];
        for (unsigned int i = 0; i < face.mNumIndices; ++i)
            indices.push_back(face.mIndices[i]);
    }

    Mesh uploadedMesh;
    uploadedMesh.name = mesh->mName.C_Str();
    uploadedMesh.indexCount = static_cast<GLsizei>(indices.size());
    uploadedMesh.useTint = isRingPartName(uploadedMesh.name);
    uploadedMesh.solidColor = uploadedMesh.useTint
        ? glm::vec3(1.0f)
        : glm::vec3(0.75f, 0.78f, 0.94f);
    uploadedMesh.minBounds = minBounds;
    uploadedMesh.maxBounds = maxBounds;

    glGenVertexArrays(1, &uploadedMesh.vao);
    glGenBuffers(1, &uploadedMesh.vbo);
    glGenBuffers(1, &uploadedMesh.ebo);

    glBindVertexArray(uploadedMesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, uploadedMesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, uploadedMesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, uv)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    meshes.push_back(uploadedMesh);
}

void AnimatedObjectPlayer::computeSceneBounds(
    const aiNode* node,
    const glm::mat4& parentTransform,
    glm::vec3& outMin,
    glm::vec3& outMax) const
{
    if (!node)
        return;

    const glm::mat4 globalTransform = parentTransform * toGlm(node->mTransformation);

    for (unsigned int meshIndex = 0; meshIndex < node->mNumMeshes; ++meshIndex)
    {
        const unsigned int sceneMeshIndex = node->mMeshes[meshIndex];
        if (sceneMeshIndex < meshes.size())
            includeTransformedBounds(meshes[sceneMeshIndex], globalTransform, outMin, outMax);
    }

    for (unsigned int childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
        computeSceneBounds(node->mChildren[childIndex], globalTransform, outMin, outMax);
}

void AnimatedObjectPlayer::includeTransformedBounds(
    const Mesh& mesh,
    const glm::mat4& transform,
    glm::vec3& outMin,
    glm::vec3& outMax)
{
    const glm::vec3 corners[8] = {
        { mesh.minBounds.x, mesh.minBounds.y, mesh.minBounds.z },
        { mesh.minBounds.x, mesh.minBounds.y, mesh.maxBounds.z },
        { mesh.minBounds.x, mesh.maxBounds.y, mesh.minBounds.z },
        { mesh.minBounds.x, mesh.maxBounds.y, mesh.maxBounds.z },
        { mesh.maxBounds.x, mesh.minBounds.y, mesh.minBounds.z },
        { mesh.maxBounds.x, mesh.minBounds.y, mesh.maxBounds.z },
        { mesh.maxBounds.x, mesh.maxBounds.y, mesh.minBounds.z },
        { mesh.maxBounds.x, mesh.maxBounds.y, mesh.maxBounds.z },
    };

    for (const glm::vec3& corner : corners)
    {
        const glm::vec3 transformedCorner = glm::vec3(transform * glm::vec4(corner, 1.0f));
        outMin = glm::min(outMin, transformedCorner);
        outMax = glm::max(outMax, transformedCorner);
    }
}

void AnimatedObjectPlayer::drawNode(
    Shader& shader,
    const aiNode* node,
    const glm::mat4& parentTransform,
    const glm::mat4& baseModel,
    const glm::vec3& tintColor) const
{
    if (!node)
        return;

    const glm::mat4 nodeTransform = getAnimatedNodeTransform(node);
    const glm::mat4 globalTransform = parentTransform * nodeTransform;

    for (unsigned int meshIndex = 0; meshIndex < node->mNumMeshes; ++meshIndex)
    {
        const unsigned int sceneMeshIndex = node->mMeshes[meshIndex];
        if (sceneMeshIndex >= meshes.size())
            continue;

        const Mesh& mesh = meshes[sceneMeshIndex];
        shader.setMat4("model", baseModel * normalizationTransform * globalTransform);
        shader.setInt("useSolidColor", 1);
        shader.setVec3("solidColor", mesh.useTint ? tintColor : mesh.solidColor);
        shader.setVec3("tintColor", glm::vec3(1.0f));
        glBindVertexArray(mesh.vao);
        glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
    }

    for (unsigned int childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
        drawNode(shader, node->mChildren[childIndex], globalTransform, baseModel, tintColor);
}

glm::mat4 AnimatedObjectPlayer::getAnimatedNodeTransform(const aiNode* node) const
{
    glm::mat4 transform = toGlm(node->mTransformation);
    const aiNodeAnim* nodeAnim = findNodeAnim(node->mName.C_Str());
    if (!nodeAnim)
        return transform;

    const double ticksPerSecond = animation->mTicksPerSecond > 0.0 ? animation->mTicksPerSecond : 30.0;
    const double animationTimeTicks = clipTimeSeconds * ticksPerSecond;
    const glm::vec3 scaling = interpolateScaling(animationTimeTicks, nodeAnim);
    const glm::quat rotation = interpolateRotation(animationTimeTicks, nodeAnim);
    const glm::vec3 translation = interpolatePosition(animationTimeTicks, nodeAnim);

    return glm::translate(glm::mat4(1.0f), translation) *
        glm::toMat4(rotation) *
        glm::scale(glm::mat4(1.0f), scaling);
}

const aiNodeAnim* AnimatedObjectPlayer::findNodeAnim(const std::string& nodeName) const
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

double AnimatedObjectPlayer::getAnimationDurationSeconds() const
{
    if (!animation)
        return 1.0 / 30.0;

    const double ticksPerSecond = animation->mTicksPerSecond > 0.0 ? animation->mTicksPerSecond : 30.0;
    return std::max(animation->mDuration / ticksPerSecond, 1.0 / 30.0);
}

glm::mat4 AnimatedObjectPlayer::toGlm(const aiMatrix4x4& matrix)
{
    glm::mat4 result(1.0f);
    result[0][0] = matrix.a1; result[1][0] = matrix.a2; result[2][0] = matrix.a3; result[3][0] = matrix.a4;
    result[0][1] = matrix.b1; result[1][1] = matrix.b2; result[2][1] = matrix.b3; result[3][1] = matrix.b4;
    result[0][2] = matrix.c1; result[1][2] = matrix.c2; result[2][2] = matrix.c3; result[3][2] = matrix.c4;
    result[0][3] = matrix.d1; result[1][3] = matrix.d2; result[2][3] = matrix.d3; result[3][3] = matrix.d4;
    return result;
}

glm::vec3 AnimatedObjectPlayer::toGlm(const aiVector3D& value)
{
    return glm::vec3(value.x, value.y, value.z);
}

glm::quat AnimatedObjectPlayer::toGlm(const aiQuaternion& value)
{
    return glm::quat(value.w, value.x, value.y, value.z);
}

glm::vec3 AnimatedObjectPlayer::interpolatePosition(double animationTimeTicks, const aiNodeAnim* nodeAnim) const
{
    if (nodeAnim->mNumPositionKeys == 0)
        return glm::vec3(0.0f);
    if (nodeAnim->mNumPositionKeys == 1)
        return toGlm(nodeAnim->mPositionKeys[0].mValue);

    for (unsigned int i = 0; i + 1 < nodeAnim->mNumPositionKeys; ++i)
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

glm::quat AnimatedObjectPlayer::interpolateRotation(double animationTimeTicks, const aiNodeAnim* nodeAnim) const
{
    if (nodeAnim->mNumRotationKeys == 0)
        return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    if (nodeAnim->mNumRotationKeys == 1)
        return glm::normalize(toGlm(nodeAnim->mRotationKeys[0].mValue));

    for (unsigned int i = 0; i + 1 < nodeAnim->mNumRotationKeys; ++i)
    {
        const aiQuatKey& current = nodeAnim->mRotationKeys[i];
        const aiQuatKey& next = nodeAnim->mRotationKeys[i + 1];
        if (animationTimeTicks < next.mTime)
        {
            const double span = next.mTime - current.mTime;
            const float factor = span > 0.0 ? static_cast<float>((animationTimeTicks - current.mTime) / span) : 0.0f;
            return glm::normalize(glm::slerp(toGlm(current.mValue), toGlm(next.mValue), glm::clamp(factor, 0.0f, 1.0f)));
        }
    }

    return glm::normalize(toGlm(nodeAnim->mRotationKeys[nodeAnim->mNumRotationKeys - 1].mValue));
}

glm::vec3 AnimatedObjectPlayer::interpolateScaling(double animationTimeTicks, const aiNodeAnim* nodeAnim) const
{
    if (nodeAnim->mNumScalingKeys == 0)
        return glm::vec3(1.0f);
    if (nodeAnim->mNumScalingKeys == 1)
        return toGlm(nodeAnim->mScalingKeys[0].mValue);

    for (unsigned int i = 0; i + 1 < nodeAnim->mNumScalingKeys; ++i)
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
