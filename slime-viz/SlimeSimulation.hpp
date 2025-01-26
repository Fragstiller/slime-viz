#ifndef SLIME_SIMULATION_HPP
#define SLIME_SIMULATION_HPP

#define _USE_MATH_DEFINES

#include <glad/glad.h>
#include <cstdlib>
#include <math.h>
#include <vector>

#include "expected.hpp"

#include "ApplicationBase.hpp"

using namespace nonstd;

float randomFloat(float lo, float hi) {
	return lo + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (hi - lo)));
}

int randomInt(int lo, int hi) {
	return rand() % (lo - hi + 1) + lo;
}

struct Agent {
	float position[2];
	float angle;
	float padding;
};

class SlimeSimulation : public ApplicationBase {
private:
	unsigned int mainTexture;
	expected<unsigned int, std::string> updateShaderProgram;
	expected<unsigned int, std::string> diffuseShaderProgram;
	expected<unsigned int, std::string> copyShaderProgram;

	int mainTextureWidth;
	int mainTextureHeight;
	int agentCount;
public:
	SlimeSimulation() {
		mainTexture = 0;
		mainTextureWidth = 1000;
		mainTextureHeight = 1000;
		agentCount = 100000;
	}

	int getWindowWidth() override {
		return mainTextureWidth;
	}

	int getWindowHeight() override {
		return mainTextureHeight;
	}

	void setupTextures() override {
		glGenTextures(1, &mainTexture);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mainTexture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, mainTextureWidth, mainTextureHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
		glBindImageTexture(0, mainTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

		unsigned int copyTexture;
		glGenTextures(1, &copyTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, copyTexture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, mainTextureWidth, mainTextureHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
		glBindImageTexture(2, copyTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	}

	unsigned int getMainTexture() override {
		return mainTexture;
	}

	void setupSSBO() override {
		unsigned int SSBO;
		glGenBuffers(1, &SSBO);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);

		std::vector<Agent> agents{};
		for (int i = 0; i < agentCount; i++) {
			Agent agent = { randomInt(0, mainTextureWidth), randomInt(0, mainTextureHeight), randomFloat(0.0f, M_PI * 2) };
			//Agent agent = { randomInt((mainTextureWidth / 2) - mainTextureWidth / 16, (mainTextureWidth / 2) + mainTextureWidth / 16),
		//					randomInt((mainTextureWidth / 2) - mainTextureWidth / 16, (mainTextureWidth / 2) + mainTextureWidth / 16),
		//					randomFloat(0.0f, M_PI * 2) };
			agents.push_back(agent);
		}
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Agent) * agents.size(), agents.data(), GL_DYNAMIC_COPY);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, SSBO);
	}

	expected<void, std::string> setupShaders() override {
		auto updateShaderProgramBuilder = ShaderProgramBuilder();
		auto updateResult = updateShaderProgramBuilder.attachShader(GL_COMPUTE_SHADER, "update.comp");
		if (!updateResult) {
			return updateResult;
		}
		updateShaderProgram = updateShaderProgramBuilder.getShaderProgram();
		if (!updateShaderProgram) {
			return make_unexpected(updateShaderProgram.error());
		}

		auto diffuseShaderProgramBuilder = ShaderProgramBuilder();
		auto diffuseResult = diffuseShaderProgramBuilder.attachShader(GL_COMPUTE_SHADER, "diffuse.comp");
		if (!diffuseResult) {
			return diffuseResult;
		}
		diffuseShaderProgram = diffuseShaderProgramBuilder.getShaderProgram();
		if (!diffuseShaderProgram) {
			return make_unexpected(diffuseShaderProgram.error());
		}

		auto copyShaderProgramBuilder = ShaderProgramBuilder();
		auto copyResult = copyShaderProgramBuilder.attachShader(GL_COMPUTE_SHADER, "copy.comp");
		if (!copyResult) {
			return copyResult;
		}
		copyShaderProgram = copyShaderProgramBuilder.getShaderProgram();
		if (!copyShaderProgram) {
			return make_unexpected(copyShaderProgram.error());
		}

		glUseProgram(*updateShaderProgram);
		glUniform1i(glGetUniformLocation(*updateShaderProgram, "width"), mainTextureWidth);
		glUniform1i(glGetUniformLocation(*updateShaderProgram, "height"), mainTextureHeight);

		glUseProgram(*diffuseShaderProgram);
		glUniform1i(glGetUniformLocation(*diffuseShaderProgram, "width"), mainTextureWidth);
		glUniform1i(glGetUniformLocation(*diffuseShaderProgram, "height"), mainTextureHeight);
	}

	void run(int frame) override {
		glUseProgram(*updateShaderProgram);
		glUniform1ui(glGetUniformLocation(*updateShaderProgram, "time"), frame);
		glDispatchCompute(agentCount, 1, 1);

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glUseProgram(*diffuseShaderProgram);
		glDispatchCompute(mainTextureWidth, mainTextureHeight, 1);

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glUseProgram(*copyShaderProgram);
		glDispatchCompute(mainTextureWidth, mainTextureHeight, 1);

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}
};

#endif