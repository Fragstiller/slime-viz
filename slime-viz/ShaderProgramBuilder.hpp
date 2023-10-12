#ifndef SHADER_PROGRAM_BUILDER_HPP
#define SHADER_PROGRAM_BUILDER_HPP

#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <array>

#include "expected.hpp"

using namespace nonstd;

constexpr int SHADER_IDS_ARRAY_SIZE = 64;

class ShaderProgramBuilder {
private:
	unsigned int programID;
	bool linked;

	std::array<unsigned int, SHADER_IDS_ARRAY_SIZE> shaderIDs;
	int lastShaderIndex;

public:
	ShaderProgramBuilder() {
		programID = 0;
		linked = false;

		shaderIDs = std::array<unsigned int, SHADER_IDS_ARRAY_SIZE>{};
		lastShaderIndex = 0;
	}

	expected<void, std::string> attachShader(GLenum shaderType, const std::string& path) {
		linked = false;

		std::ifstream shaderStream (path);
		std::stringstream shaderStringStream;
		std::string shaderString;
		if (shaderStream.is_open()) {
			shaderStringStream << shaderStream.rdbuf();
			shaderString = shaderStringStream.str();
		}
		else return make_unexpected("failed to open shader file\n");
		shaderStream.close();
		auto shaderCString = shaderString.c_str();

		int success;
		char infoLog[512];

		unsigned int shaderID;
		shaderID = glCreateShader(shaderType);
		glShaderSource(shaderID, 1, &shaderCString, nullptr);
		glCompileShader(shaderID);

		glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(shaderID, 512, nullptr, infoLog);
			return make_unexpected(infoLog);
		}

		if (lastShaderIndex >= shaderIDs.size()) {
			make_unexpected("shaders IDs array is full");
		}
		shaderIDs[lastShaderIndex] = shaderID;
		lastShaderIndex++;
	}

	expected<unsigned int, std::string> getShaderProgram() {
		if (linked) return programID;

		programID = glCreateProgram();

		for (auto const &shaderID : shaderIDs) {
			if (shaderID == 0) {
				break;
			}
			glAttachShader(programID, shaderID);
		}

		glLinkProgram(programID);

		int success;
		char infoLog[512];
		glGetProgramiv(programID, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(programID, 512, NULL, infoLog);
			return make_unexpected(infoLog);
		}

		for (auto const &shaderID : shaderIDs) {
			if (shaderID == 0) {
				break;
			}
			glDeleteShader(shaderID);
		}
		linked = true;

		return programID;
	}
};
#endif