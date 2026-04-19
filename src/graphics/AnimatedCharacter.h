#pragma once

#include <glad/glad.h>

#include <assimp/matrix4x4.h>
#include <assimp/quaternion.h>
#include <assimp/scene.h>
#include <assimp/vector3.h>

#include <glm/glm.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Shader;
class Texture;
namespace Assimp { class Importer; }

class AnimatedCharacter
{
public:
    AnimatedCharacter(const std::string& modelPath, const std::string& texturePath);
    ~AnimatedCharacter();

    AnimatedCharacter(const AnimatedCharacter&) = delete;
    AnimatedCharacter& operator=(const AnimatedCharacter&) = delete;

    void update(float deltaSeconds, bool isWalking);
    void draw(
        Shader& shader,
        const glm::mat4& view,
        const glm::mat4& projection,
        const glm::vec3& worldPosition,
        float worldYawDegrees) const;

    bool isLoaded() const { return loaded; }
    std::string getError() const { return loadError; }

private:
    static constexpr int kMaxWeightsPerVertex = 4;

    struct Vertex
    {
        glm::vec3 position{0.0f};
        glm::vec3 normal{0.0f, 1.0f, 0.0f};
        glm::vec2 uv{0.0f};
        int boneIds[kMaxWeightsPerVertex]{ 0, 0, 0, 0 };
        float boneWeights[kMaxWeightsPerVertex]{ 0.0f, 0.0f, 0.0f, 0.0f };
    };

    struct RenderVertex
    {
        glm::vec3 position{0.0f};
        glm::vec3 normal{0.0f, 1.0f, 0.0f};
        glm::vec2 uv{0.0f};
    };

    struct BoneInfo
    {
        glm::mat4 offset{1.0f};
    };

    struct Clip
    {
        double startFrame = 0.0;
        double endFrame = 0.0;
    };

    void loadModel(const std::string& modelPath, const std::string& texturePath);
    void setupMesh();
    void extractMesh(aiMesh* mesh);
    void readBoneWeights(aiMesh* mesh);
    bool findNodeForMesh(const aiNode* node, unsigned int meshIndex, const glm::mat4& parentTransform, glm::mat4& outGlobalTransform) const;
    bool findNodeByName(const aiNode* node, const std::string& nodeName, const glm::mat4& parentTransform, glm::mat4& outGlobalTransform) const;

    void updateAnimationPose();
    void readNodeHierarchy(double animationTimeTicks, const aiNode* node, const glm::mat4& parentTransform);
    const aiNodeAnim* findNodeAnim(const aiAnimation* animation, const std::string& nodeName) const;

    glm::vec3 interpolatePosition(double animationTimeTicks, const aiNodeAnim* nodeAnim) const;
    glm::quat interpolateRotation(double animationTimeTicks, const aiNodeAnim* nodeAnim) const;
    glm::vec3 interpolateScaling(double animationTimeTicks, const aiNodeAnim* nodeAnim) const;

    static glm::mat4 toGlm(const aiMatrix4x4& matrix);
    static glm::vec3 toGlm(const aiVector3D& value);
    static glm::quat toGlm(const aiQuaternion& value);

    void setVertexBoneData(Vertex& vertex, int boneId, float weight);
    void normalizeVertexWeights();
    void updateFittedBounds();

    std::unique_ptr<Assimp::Importer> importer;
    const aiScene* scene = nullptr;
    std::unique_ptr<Texture> texture;

    std::vector<Vertex> baseVertices;
    std::vector<RenderVertex> renderVertices;
    std::vector<unsigned int> indices;

    std::unordered_map<std::string, int> boneMapping;
    std::vector<BoneInfo> boneInfos;
    std::vector<glm::mat4> finalBoneTransforms;

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;

    glm::mat4 skeletonInverseTransform{1.0f};
    glm::vec3 fittedBasePivot{0.0f};
    float fittedScale = 50.0f;
    Clip idleClip{30.0, 150.0};
    Clip walkClip{151.0, 180.0};
    double clipTimeSeconds = 0.0;
    bool walking = false;
    bool loaded = false;
    std::string loadError;
};
