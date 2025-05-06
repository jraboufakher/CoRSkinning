#pragma once
#include <string>
#include <GL/glew.h>

class Shader
{
public:
	Shader();
	~Shader();

	bool LoadShaders(const char* vertPath, const char* fragPath);

	void UseShaderProg() const;

	GLuint GetProgID() const;

private:
	GLuint skinprogram_;
	std::string readFile(const char* path);
	bool checkCompileErrors(GLuint shader, const char* type);
	bool checkLinkErrors(GLuint prog);
};

