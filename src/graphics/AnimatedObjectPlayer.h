#pragma once

#include <glad/glad.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <vector>

class Shader;
class Texture;

class AnimatedObjectPlayer
{
public:
    AnimatedObjectPlayer(
        const std::string& modelPath,
        const std::string& texturePath,
        bool looping = false,
        const std::string& animationName = "");
    ~AnimatedObjectPlayer();

    AnimatedObjectPlayer(const AnimatedObjectPlayer&) = delete;
    AnimatedObjectPlayer& operator=(const AnimatedObjectPlayer&) = delete;

    void update(float deltaSeconds, bool restart = false);
    void draw(
        Shader& shader,
        const glm::mat4& view,
        const glm::mat4& projection,
        const glm::mat4& baseModel,
        const glm::vec3& tintColor) const;

    bool isLoaded() const { return loaded; }
    const std::string& getError() const { return loadError; }
    bool isFinished() const { return finished; }

private:
    struct Vertex
    {
        glm::vec3 position{ 0.0f };
        glm::vec3 normal{ 0.0f, 1.0f, 0.0f };
        glm::vec2 uv{ 0.0f };
    };

    struct Mesh
    {
        std::string name;
        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint ebo = 0;
        GLsizei indexCount = 0;
        bool useTint = true;
        glm::vec3 solidColor{ 0.78f, 0.76f, 1.0f };
        glm::vec3 minBounds{ 0.0f };
        glm::vec3 maxBounds{ 0.0f };
    };

    void load(const std::string& modelPath, const std::string& texturePath, const std::string& animationName);
    void uploadMesh(const aiMesh* mesh);
    void computeSceneBounds(
        const aiNode* node,
        const glm::mat4& parentTransform,
        glm::vec3& outMin,
        glm::vec3& outMax) const;
    static void includeTransformedBounds(
        const Mesh& mesh,
        const glm::mat4& transform,
        glm::vec3& outMin,
        glm::vec3& outMax);
    void drawNode(
        Shader& shader,
        const aiNode* node,
        const glm::mat4& parentTransform,
        const glm::mat4& baseModel,
        const glm::vec3& tintColor) const;
    glm::mat4 getAnimatedNodeTransform(const aiNode* node) const;
    const aiNodeAnim* findNodeAnim(const std::string& nodeName) const;
    double getAnimationDurationSeconds() const;

    static glm::mat4 toGlm(const aiMatrix4x4& matrix);
    static glm::vec3 toGlm(const aiVector3D& value);
    static glm::quat toGlm(const aiQuaternion& value);

    glm::vec3 interpolatePosition(double animationTimeTicks, const aiNodeAnim* nodeAnim) const;
    glm::quat interpolateRotation(double animationTimeTicks, const aiNodeAnim* nodeAnim) const;
    glm::vec3 interpolateScaling(double animationTimeTicks, const aiNodeAnim* nodeAnim) const;

    std::unique_ptr<Assimp::Importer> importer;
    const aiScene* scene = nullptr;
    const aiAnimation* animation = nullptr;
    std::unique_ptr<Texture> texture;
    std::vector<Mesh> meshes;
    glm::mat4 rootInverseTransform{ 1.0f };
    glm::mat4 normalizationTransform{ 1.0f };
    glm::vec3 minBounds{ 0.0f };
    glm::vec3 maxBounds{ 0.0f };
    glm::vec3 fittedBasePivot{ 0.0f };
    float fittedScale = 1.0f;

    double clipTimeSeconds = 0.0;
    bool looping = false;
    bool finished = false;
    bool loaded = false;
    std::string loadError;
};
