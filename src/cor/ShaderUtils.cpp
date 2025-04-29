#include "cor/ShaderUtils.h"
#include <fstream>
#include <sstream>
#include <iostream>

static std::string ReadFile(const char* path) {
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in) {
        std::cerr << "Error opening " << path << "\n";
        return "";
    }
    std::ostringstream contents;
    contents << in.rdbuf();
    return contents.str();
}

static bool CheckCompileErrors(GLuint shader, const char* type) {
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_TRUE) return true;

    GLint logLen;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
    std::string log(logLen, ' ');
    glGetShaderInfoLog(shader, logLen, nullptr, &log[0]);
    std::cerr << "Shader compile error (" << type << "):\n" << log << "\n";
    return false;
}

static bool CheckLinkErrors(GLuint prog) {
    GLint status;
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (status == GL_TRUE) return true;

    GLint logLen;
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLen);
    std::string log(logLen, ' ');
    glGetProgramInfoLog(prog, logLen, nullptr, &log[0]);
    std::cerr << "Program link error:\n" << log << "\n";
    return false;
}

GLuint LoadShaders(const char* vertPath, const char* fragPath) {
    // 1) Read sources
    std::string vertCode = ReadFile(vertPath);
    std::cerr << "[DEBUG] vertex shader (" << vertPath << ") length = " << vertCode.size() << "\n";
    std::string fragCode = ReadFile(fragPath);
    std::cerr << "[DEBUG] fragment shader (" << fragPath << ") length = " << fragCode.size() << "\n";
    if (vertCode.empty() || fragCode.empty()) return 0;

    const char* vSrc = vertCode.c_str();
    const char* fSrc = fragCode.c_str();

    // 2) Create & compile vertex shader
    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vSrc, nullptr);
    glCompileShader(vert);
    if (!CheckCompileErrors(vert, "VERTEX")) {
        glDeleteShader(vert);
        return 0;
    }

    // 3) Create & compile fragment shader
    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &fSrc, nullptr);
    glCompileShader(frag);
    if (!CheckCompileErrors(frag, "FRAGMENT")) {
        glDeleteShader(vert);
        glDeleteShader(frag);
        return 0;
    }

    // 4) Link into a program
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    if (!CheckLinkErrors(prog)) {
        glDeleteProgram(prog);
        glDeleteShader(vert);
        glDeleteShader(frag);
        return 0;
    }

    // 5) Clean up shader objects (no longer needed once linked)
    glDetachShader(prog, vert);
    glDetachShader(prog, frag);
    glDeleteShader(vert);
    glDeleteShader(frag);

    return prog;
}
