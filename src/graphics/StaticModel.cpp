#include "StaticModel.h"

#include "Shader.h"
#include "Texture.h"

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <glm/common.hpp>

#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace
{
    constexpr unsigned int kStaticModelFlags =
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality |
        aiProcess_PreTransformVertices;

    void collectMeshData(
        const aiScene* scene,
        const aiNode* node,
        std::vector<StaticModel::Vertex>& vertices,
        std::vector<unsigned int>& indices,
        glm::vec3& minBounds,
        glm::vec3& maxBounds)
    {
        if (!scene || !node)
            return;

        for (unsigned int meshIndex = 0; meshIndex < node->mNumMeshes; ++meshIndex)
        {
            const aiMesh* mesh = scene->mMeshes[node->mMeshes[meshIndex]];
            if (!mesh || mesh->mNumVertices == 0)
                continue;

            const unsigned int baseVertex = static_cast<unsigned int>(vertices.size());

            for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
            {
                const aiVector3D& position = mesh->mVertices[i];
                StaticModel::Vertex vertex;
                vertex.position = glm::vec3(position.x, position.y, position.z);

                if (mesh->HasNormals())
                {
                    const aiVector3D& normal = mesh->mNormals[i];
                    vertex.normal = glm::normalize(glm::vec3(normal.x, normal.y, normal.z));
                }

                if (mesh->HasTextureCoords(0))
                {
                    const aiVector3D& texCoord = mesh->mTextureCoords[0][i];
                    vertex.uv = glm::vec2(texCoord.x, 1.0f - texCoord.y);
                }

                vertices.push_back(vertex);
                minBounds = glm::min(minBounds, vertex.position);
                maxBounds = glm::max(maxBounds, vertex.position);
            }

            for (unsigned int faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex)
            {
                const aiFace& face = mesh->mFaces[faceIndex];
                for (unsigned int i = 0; i < face.mNumIndices; ++i)
                    indices.push_back(baseVertex + face.mIndices[i]);
            }
        }

        for (unsigned int childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
            collectMeshData(scene, node->mChildren[childIndex], vertices, indices, minBounds, maxBounds);
    }
}

StaticModel::StaticModel(const std::string& modelPath)
{
    try
    {
        load(modelPath);
        loaded = true;
    }
    catch (const std::exception& ex)
    {
        loadError = ex.what();
        std::cerr << "StaticModel load failed: " << loadError << "\n";
    }
}

StaticModel::~StaticModel()
{
    if (ebo != 0)
        glDeleteBuffers(1, &ebo);
    if (vbo != 0)
        glDeleteBuffers(1, &vbo);
    if (vao != 0)
        glDeleteVertexArrays(1, &vao);
}

void StaticModel::load(const std::string& modelPath)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(modelPath, kStaticModelFlags);

    if (!scene || !scene->mRootNode || scene->mNumMeshes == 0)
        throw std::runtime_error("Failed to load model '" + modelPath + "': " + importer.GetErrorString());

    vertices.clear();
    indices.clear();
    minBounds = glm::vec3(std::numeric_limits<float>::max());
    maxBounds = glm::vec3(std::numeric_limits<float>::lowest());

    collectMeshData(scene, scene->mRootNode, vertices, indices, minBounds, maxBounds);

    if (vertices.empty() || indices.empty())
        throw std::runtime_error("Model has no renderable geometry: " + modelPath);

    const std::filesystem::path modelDir = std::filesystem::path(modelPath).parent_path();
    for (unsigned int materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex)
    {
        aiMaterial* material = scene->mMaterials[materialIndex];
        if (!material || material->GetTextureCount(aiTextureType_DIFFUSE) == 0)
            continue;

        aiString texturePath;
        if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) != AI_SUCCESS)
            continue;

        const std::filesystem::path resolvedPath = modelDir / texturePath.C_Str();

        try
        {
            diffuseTexture = std::make_unique<Texture>(resolvedPath.string());
            if (diffuseTexture->isLoaded())
            {
                std::cout << "StaticModel texture loaded: " << resolvedPath.string() << "\n";
                break;
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "StaticModel texture load failed: " << ex.what() << "\n";
        }
    }

    upload();

    const glm::vec3 size = getBoundsSize();
    std::cout << "StaticModel loaded: " << modelPath
        << " | vertices=" << vertices.size()
        << " | indices=" << indices.size()
        << " | size=(" << size.x << ", " << size.y << ", " << size.z << ")"
        << "\n";
}

void StaticModel::upload()
{
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
        vertices.data(),
        GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
        indices.data(),
        GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, uv)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void StaticModel::Draw(Shader& shader) const
{
    if (!loaded)
        return;

    shader.use();
    shader.setInt("texture_diffuse1", 0);
    shader.setInt("useTexture", diffuseTexture && diffuseTexture->isLoaded() ? 1 : 0);
    if (diffuseTexture && diffuseTexture->isLoaded())
        diffuseTexture->bind(0);
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}
