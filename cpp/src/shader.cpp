#include "shader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

static std::string readFile(const std::string& path) {
    std::ifstream f(path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

GLuint Shader::compile(GLenum type, const std::string& source) {
    GLuint s = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);

    int ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, 512, nullptr, log);
        std::cerr << "Shader compile error: " << log << std::endl;
        return 0;
    }
    return s;
}

bool Shader::load(const std::string& vertPath, const std::string& fragPath) {
    std::string vertSrc = readFile(vertPath);
    std::string fragSrc = readFile(fragPath);

    GLuint vert = compile(GL_VERTEX_SHADER, vertSrc);
    GLuint frag = compile(GL_FRAGMENT_SHADER, fragSrc);
    if (!vert || !frag) return false;

    program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

    int ok;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(program, 512, nullptr, log);
        std::cerr << "Shader link error: " << log << std::endl;
        return false;
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
    return true;
}

void Shader::use() const {
    glUseProgram(program);
}

void Shader::setMat4(const char* name, const glm::mat4& m) const {
    glUniformMatrix4fv(glGetUniformLocation(program, name), 1, GL_FALSE, glm::value_ptr(m));
}

void Shader::setInt(const char* name, int v) const {
    glUniform1i(glGetUniformLocation(program, name), v);
}

Shader::~Shader() {
    if (program) glDeleteProgram(program);
}
