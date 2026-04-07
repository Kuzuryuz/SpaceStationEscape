#pragma once

#include <string>
#include <glm/glm.hpp>

class Shader
{
public:
    unsigned int ID = 0;

    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();

    void use() const;

    void setMat4(const std::string& name, const glm::mat4& value) const;
    void setVec2(const std::string& name, const glm::vec2& value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;

private:
    static std::string readFile(const std::string& path);
    static unsigned int compileShader(unsigned int type, const std::string& source);
};