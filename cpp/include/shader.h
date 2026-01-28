#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>

class Shader {
public:
    GLuint program = 0;

    Shader() = default;
    bool load(const std::string& vertPath, const std::string& fragPath);
    void use() const;
    void setMat4(const char* name, const glm::mat4& m) const;
    void setInt(const char* name, int v) const;
    ~Shader();

private:
    GLuint compile(GLenum type, const std::string& source);
};
