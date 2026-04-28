#pragma once

#include <glad/glad.h>

#include <cstddef>
#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <vector>

class Shader;
class Texture;

class StaticModel
{
public:
    struct Vertex
    {
        glm::vec3 position{ 0.0f };
        glm::vec3 normal{ 0.0f, 1.0f, 0.0f };
        glm::vec2 uv{ 0.0f };
        glm::vec3 materialColor{ 1.0f };
        float partMask = 0.0f;
    };

    explicit StaticModel(const std::string& modelPath);
    ~StaticModel();

    StaticModel(const StaticModel&) = delete;
    StaticModel& operator=(const StaticModel&) = delete;

    bool isLoaded() const { return loaded; }
    const std::string& getError() const { return loadError; }
    glm::vec3 getBoundsSize() const { return maxBounds - minBounds; }

    void Draw(Shader& shader) const;
    void DrawPartColored(Shader& shader, const glm::vec3& pipeColor, const glm::vec3& ringColor) const;

private:
    void load(const std::string& modelPath);
    void upload();

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    GLsizei pipeIndexCount = 0;
    GLsizei ringIndexCount = 0;
    std::unique_ptr<Texture> diffuseTexture;

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;

    glm::vec3 minBounds{ 0.0f };
    glm::vec3 maxBounds{ 0.0f };

    bool loaded = false;
    std::string loadError;
};
