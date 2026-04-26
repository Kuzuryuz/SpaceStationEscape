#include "StaticModel.h"

#include "Shader.h"
#include "Texture.h"

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <glm/common.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <utility>

namespace
{
    constexpr unsigned int kStaticModelFlags =
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality |
        aiProcess_PreTransformVertices;

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

    bool isPuzzlePipePath(const std::string& modelPath)
    {
        const std::string loweredPath = toLowerCopy(modelPath);
        return loweredPath.find("assets/models/puzzle/pipe-up.obj") != std::string::npos ||
            loweredPath.find("assets\\models\\puzzle\\pipe-up.obj") != std::string::npos ||
            loweredPath.find("assets/models/puzzle/pipe-down.obj") != std::string::npos ||
            loweredPath.find("assets\\models\\puzzle\\pipe-down.obj") != std::string::npos;
    }

    int parseObjIndex(const std::string& token)
    {
        if (token.empty())
            return 0;

        const size_t slashPos = token.find('/');
        const std::string indexText = slashPos == std::string::npos ? token : token.substr(0, slashPos);
        return indexText.empty() ? 0 : std::stoi(indexText);
    }

    struct ObjFaceVertex
    {
        int positionIndex = 0;
        int uvIndex = 0;
        int normalIndex = 0;
    };

    ObjFaceVertex parseObjFaceVertex(const std::string& token)
    {
        ObjFaceVertex result;
        std::stringstream stream(token);
        std::string part;

        if (std::getline(stream, part, '/') && !part.empty())
            result.positionIndex = std::stoi(part);
        if (std::getline(stream, part, '/') && !part.empty())
            result.uvIndex = std::stoi(part);
        if (std::getline(stream, part, '/') && !part.empty())
            result.normalIndex = std::stoi(part);

        return result;
    }

    bool appendObjVertex(
        const ObjFaceVertex& faceVertex,
        const std::vector<glm::vec3>& positions,
        const std::vector<glm::vec2>& uvs,
        const std::vector<glm::vec3>& normals,
        std::vector<StaticModel::Vertex>& vertices,
        std::vector<unsigned int>& targetIndices,
        glm::vec3& minBounds,
        glm::vec3& maxBounds)
    {
        if (faceVertex.positionIndex <= 0 || faceVertex.positionIndex > static_cast<int>(positions.size()))
            return false;

        StaticModel::Vertex vertex;
        vertex.position = positions[static_cast<size_t>(faceVertex.positionIndex - 1)];
        if (faceVertex.uvIndex > 0 && faceVertex.uvIndex <= static_cast<int>(uvs.size()))
            vertex.uv = uvs[static_cast<size_t>(faceVertex.uvIndex - 1)];
        if (faceVertex.normalIndex > 0 && faceVertex.normalIndex <= static_cast<int>(normals.size()))
            vertex.normal = normals[static_cast<size_t>(faceVertex.normalIndex - 1)];

        const unsigned int newIndex = static_cast<unsigned int>(vertices.size());
        vertices.push_back(vertex);
        targetIndices.push_back(newIndex);
        minBounds = glm::min(minBounds, vertex.position);
        maxBounds = glm::max(maxBounds, vertex.position);
        return true;
    }

    bool loadPuzzlePipeObjData(
        const std::string& modelPath,
        std::vector<StaticModel::Vertex>& vertices,
        std::vector<unsigned int>& indices,
        GLsizei& pipeIndexCount,
        GLsizei& ringIndexCount,
        glm::vec3& minBounds,
        glm::vec3& maxBounds)
    {
        std::ifstream file(modelPath);
        if (!file.is_open())
            return false;

        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uvs;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> pipeIndices;
        std::vector<unsigned int> ringIndices;
        bool readingRing = false;
        bool sawNamedPipePart = false;
        bool sawNamedRingPart = false;

        std::string line;
        while (std::getline(file, line))
        {
            if (line.rfind("o ", 0) == 0 || line.rfind("g ", 0) == 0)
            {
                const std::string partName = line.size() > 2 ? line.substr(2) : "";
                readingRing = isRingPartName(partName);
                sawNamedRingPart = sawNamedRingPart || readingRing;
                sawNamedPipePart = sawNamedPipePart || toLowerCopy(partName) == "pipe";
                continue;
            }

            std::istringstream stream(line);
            std::string prefix;
            stream >> prefix;
            if (prefix == "v")
            {
                glm::vec3 position(0.0f);
                stream >> position.x >> position.y >> position.z;
                positions.push_back(position);
            }
            else if (prefix == "vt")
            {
                glm::vec2 uv(0.0f);
                stream >> uv.x >> uv.y;
                uv.y = 1.0f - uv.y;
                uvs.push_back(uv);
            }
            else if (prefix == "vn")
            {
                glm::vec3 normal(0.0f, 1.0f, 0.0f);
                stream >> normal.x >> normal.y >> normal.z;
                normals.push_back(glm::normalize(normal));
            }
            else if (prefix == "f")
            {
                std::vector<ObjFaceVertex> faceVertices;
                std::string token;
                while (stream >> token)
                    faceVertices.push_back(parseObjFaceVertex(token));

                if (faceVertices.size() < 3)
                    continue;

                std::vector<unsigned int>& target = readingRing ? ringIndices : pipeIndices;
                for (size_t i = 1; i + 1 < faceVertices.size(); ++i)
                {
                    if (!appendObjVertex(faceVertices[0], positions, uvs, normals, vertices, target, minBounds, maxBounds) ||
                        !appendObjVertex(faceVertices[i], positions, uvs, normals, vertices, target, minBounds, maxBounds) ||
                        !appendObjVertex(faceVertices[i + 1], positions, uvs, normals, vertices, target, minBounds, maxBounds))
                    {
                        return false;
                    }
                }
            }
        }

        if (!sawNamedPipePart || !sawNamedRingPart || pipeIndices.empty() || ringIndices.empty())
            return false;

        pipeIndexCount = static_cast<GLsizei>(pipeIndices.size());
        ringIndexCount = static_cast<GLsizei>(ringIndices.size());
        indices.reserve(pipeIndices.size() + ringIndices.size());
        indices.insert(indices.end(), pipeIndices.begin(), pipeIndices.end());
        indices.insert(indices.end(), ringIndices.begin(), ringIndices.end());
        return true;
    }

    bool splitPuzzlePipeObjIndices(
        const std::string& modelPath,
        std::vector<unsigned int>& pipeIndices,
        std::vector<unsigned int>& ringIndices)
    {
        std::ifstream file(modelPath);
        if (!file.is_open())
            return false;

        bool readingRing = false;
        bool sawNamedPipePart = false;
        bool sawNamedRingPart = false;
        std::string line;
        while (std::getline(file, line))
        {
            if (line.rfind("o ", 0) == 0 || line.rfind("g ", 0) == 0)
            {
                const std::string partName = line.size() > 2 ? line.substr(2) : "";
                readingRing = isRingPartName(partName);
                sawNamedRingPart = sawNamedRingPart || readingRing;
                sawNamedPipePart = sawNamedPipePart || toLowerCopy(partName) == "pipe";
                continue;
            }

            if (line.rfind("f ", 0) != 0)
                continue;

            std::istringstream stream(line.substr(2));
            std::vector<unsigned int> faceIndices;
            std::string token;
            while (stream >> token)
            {
                const int objIndex = parseObjIndex(token);
                if (objIndex > 0)
                    faceIndices.push_back(static_cast<unsigned int>(objIndex - 1));
            }

            if (faceIndices.size() < 3)
                continue;

            std::vector<unsigned int>& target = readingRing ? ringIndices : pipeIndices;
            for (size_t i = 1; i + 1 < faceIndices.size(); ++i)
            {
                target.push_back(faceIndices[0]);
                target.push_back(faceIndices[i]);
                target.push_back(faceIndices[i + 1]);
            }
        }

        return sawNamedPipePart && sawNamedRingPart && !pipeIndices.empty() && !ringIndices.empty();
    }

    void collectMeshData(
        const aiScene* scene,
        const aiNode* node,
        std::vector<StaticModel::Vertex>& vertices,
        std::vector<unsigned int>& pipeIndices,
        std::vector<unsigned int>& ringIndices,
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
            const std::string meshName = mesh->mName.C_Str();
            const std::string nodeName = node->mName.C_Str();
            const float partMask = (isRingPartName(meshName) || isRingPartName(nodeName)) ? 1.0f : 0.0f;
            const bool hasUv = mesh->HasTextureCoords(0);

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

                if (hasUv)
                {
                    const aiVector3D& texCoord = mesh->mTextureCoords[0][i];
                    vertex.uv = glm::vec2(texCoord.x, 1.0f - texCoord.y);
                }

                const bool uvLooksLikeRing = hasUv && vertex.uv.x > 0.0001f && vertex.uv.x < 0.28f;
                vertex.partMask = (partMask > 0.5f || uvLooksLikeRing) ? 1.0f : 0.0f;
                vertices.push_back(vertex);
                minBounds = glm::min(minBounds, vertex.position);
                maxBounds = glm::max(maxBounds, vertex.position);
            }

            for (unsigned int faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex)
            {
                const aiFace& face = mesh->mFaces[faceIndex];
                bool faceIsRing = partMask > 0.5f;
                if (!faceIsRing && hasUv)
                {
                    float uvSum = 0.0f;
                    for (unsigned int i = 0; i < face.mNumIndices; ++i)
                        uvSum += vertices[baseVertex + face.mIndices[i]].uv.x;
                    const float averageUvX = face.mNumIndices > 0 ? uvSum / static_cast<float>(face.mNumIndices) : 1.0f;
                    faceIsRing = averageUvX > 0.0001f && averageUvX < 0.28f;
                }

                std::vector<unsigned int>& targetIndices = faceIsRing ? ringIndices : pipeIndices;
                for (unsigned int i = 0; i < face.mNumIndices; ++i)
                    targetIndices.push_back(baseVertex + face.mIndices[i]);
            }
        }

        for (unsigned int childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
            collectMeshData(scene, node->mChildren[childIndex], vertices, pipeIndices, ringIndices, minBounds, maxBounds);
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
    if (isPuzzlePipePath(modelPath))
    {
        vertices.clear();
        indices.clear();
        minBounds = glm::vec3(std::numeric_limits<float>::max());
        maxBounds = glm::vec3(std::numeric_limits<float>::lowest());
        pipeIndexCount = 0;
        ringIndexCount = 0;

        if (loadPuzzlePipeObjData(modelPath, vertices, indices, pipeIndexCount, ringIndexCount, minBounds, maxBounds))
        {
            upload();

            const glm::vec3 size = getBoundsSize();
            std::cout << "StaticModel raw pipe OBJ loaded: " << modelPath
                << " | vertices=" << vertices.size()
                << " | indices=" << indices.size()
                << " | pipeIndices=" << pipeIndexCount
                << " | ringIndices=" << ringIndexCount
                << " | size=(" << size.x << ", " << size.y << ", " << size.z << ")"
                << "\n";
            return;
        }

        std::cerr << "StaticModel raw pipe OBJ load failed, falling back to Assimp: " << modelPath << "\n";
    }

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(modelPath, kStaticModelFlags);

    if (!scene || !scene->mRootNode || scene->mNumMeshes == 0)
        throw std::runtime_error("Failed to load model '" + modelPath + "': " + importer.GetErrorString());

    vertices.clear();
    indices.clear();
    std::vector<unsigned int> pipeIndices;
    std::vector<unsigned int> ringIndices;
    minBounds = glm::vec3(std::numeric_limits<float>::max());
    maxBounds = glm::vec3(std::numeric_limits<float>::lowest());

    collectMeshData(scene, scene->mRootNode, vertices, pipeIndices, ringIndices, minBounds, maxBounds);
    if (isPuzzlePipePath(modelPath))
    {
        std::vector<unsigned int> objPipeIndices;
        std::vector<unsigned int> objRingIndices;
        if (splitPuzzlePipeObjIndices(modelPath, objPipeIndices, objRingIndices))
        {
            pipeIndices = std::move(objPipeIndices);
            ringIndices = std::move(objRingIndices);
        }
        else
        {
            std::cerr << "StaticModel pipe OBJ split failed, using Assimp split: " << modelPath << "\n";
        }
    }

    pipeIndexCount = static_cast<GLsizei>(pipeIndices.size());
    ringIndexCount = static_cast<GLsizei>(ringIndices.size());
    indices.reserve(pipeIndices.size() + ringIndices.size());
    indices.insert(indices.end(), pipeIndices.begin(), pipeIndices.end());
    indices.insert(indices.end(), ringIndices.begin(), ringIndices.end());

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
        << " | pipeIndices=" << pipeIndexCount
        << " | ringIndices=" << ringIndexCount
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

    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, partMask)));
    glEnableVertexAttribArray(3);

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

void StaticModel::DrawPartColored(Shader& shader, const glm::vec3& pipeColor, const glm::vec3& ringColor) const
{
    if (!loaded)
        return;

    shader.use();
    shader.setInt("useTexture", 0);
    shader.setInt("usePartColors", 0);

    glBindVertexArray(vao);
    if (pipeIndexCount > 0)
    {
        shader.setVec3("tintColor", pipeColor);
        glDrawElements(GL_TRIANGLES, pipeIndexCount, GL_UNSIGNED_INT, nullptr);
    }

    if (ringIndexCount > 0)
    {
        shader.setVec3("tintColor", ringColor);
        const void* offset = reinterpret_cast<const void*>(static_cast<std::uintptr_t>(pipeIndexCount) * sizeof(unsigned int));
        glDrawElements(GL_TRIANGLES, ringIndexCount, GL_UNSIGNED_INT, offset);
    }
    glBindVertexArray(0);
}
