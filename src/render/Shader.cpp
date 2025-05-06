#include "render/Shader.h"
#include <fstream>
#include <sstream>
#include <iostream>

Shader::Shader(): skinprogram_(0){}
Shader::~Shader() {
	if (skinprogram_ != 0) {
		glDeleteProgram(skinprogram_);
	}
}

bool Shader::LoadShaders(const char* vertPath, const char* fragPath)
{
    std::string vertCode = readFile(vertPath);
    std::string fragCode = readFile(fragPath);
    if (vertCode.empty() || fragCode.empty()) return 0;

    const char* vSrc = vertCode.c_str();
    const char* fSrc = fragCode.c_str();

    // 2) Create & compile vertex shader
    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vSrc, nullptr);
    glCompileShader(vert);
    if (!checkCompileErrors(vert, "VERTEX")) {
        glDeleteShader(vert);
        return false;
    }

    // 3) Create & compile fragment shader
    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &fSrc, nullptr);
    glCompileShader(frag);
    if (!checkCompileErrors(frag, "FRAGMENT")) {
        glDeleteShader(vert);
        glDeleteShader(frag);
        return false;
    }

    // 4) Link into program
    skinprogram_ = glCreateProgram();
    glAttachShader(skinprogram_, vert);
    glAttachShader(skinprogram_, frag);/*

    glBindAttribLocation(skinprogram_, 4, "SkeletonBoneIndices");
    glBindAttribLocation(skinprogram_, 5, "SkeletonBoneWeights");
    glBindAttribLocation(skinprogram_, 6, "centerOfRotation");*/

    glLinkProgram(skinprogram_);
    if (!checkLinkErrors(skinprogram_)) {
        glDeleteProgram(skinprogram_);
        glDeleteShader(vert);
        glDeleteShader(frag);
        return false;
    }

    // 5) Clean up shader objects (no longer needed once linked)
    glDetachShader(skinprogram_, vert);
    glDetachShader(skinprogram_, frag);
    glDeleteShader(vert);
    glDeleteShader(frag);

    return true;
}

void Shader::UseShaderProg() const
{
    if (skinprogram_ != 0) {
        glUseProgram(skinprogram_);
    }
}

GLuint Shader::GetProgID() const 
{
    return skinprogram_;
}

std::string Shader::readFile(const char* path)
{
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in) {
        std::cerr << "Error opening " << path << "\n";
        return std::string();
    }
    std::ostringstream contents;
    contents << in.rdbuf();
    return contents.str();
}

bool Shader::checkCompileErrors(GLuint shader, const char* type)
{
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

bool Shader::checkLinkErrors(GLuint prog)
{
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


