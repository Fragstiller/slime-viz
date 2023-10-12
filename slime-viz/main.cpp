#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cstdio>
#include <fstream>
#include <sstream>

#include "expected.hpp";

#include "ShaderProgramBuilder.hpp"

constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 600;
constexpr bool WINDOW_RESIZEABLE = false;

using namespace nonstd;

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
}

int main() {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, (int)WINDOW_RESIZEABLE);

	const auto window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "slime-viz", nullptr, nullptr);
	if (window == 0) {
		printf("GLFW init failed\n");
		return -1;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		printf("GLAD init failed\n");
		return -1;
	}

	int workGroupCount[3];
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &workGroupCount[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &workGroupCount[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &workGroupCount[2]);
	printf("max global (total) work group counts x:%i y:%i z%i\n",
			workGroupCount[0], workGroupCount[1], workGroupCount[2]);

	int workGroupSize[3];
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &workGroupSize[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &workGroupSize[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &workGroupSize[2]);
	printf("max local (in one shader) work group sizes x:%i y:%i z%i\n",
			workGroupSize[0], workGroupSize[1], workGroupSize[2]);

	int workGroupInv;
	glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &workGroupInv);
	printf("max local work group invocations %i\n", workGroupInv);

	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	unsigned int texture;
	glGenTextures(1, &texture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 512, 512, 0, GL_RGBA, GL_FLOAT, nullptr);
	glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	float vertices[] = {
		 0.5f,  0.5f, 0.0f,  1.0f, 1.0f,
		 0.5f, -0.5f, 0.0f,  1.0f, 0.0f,
		-0.5f, -0.5f, 0.0f,  0.0f, 0.0f,
		-0.5f,  0.5f, 0.0f,  0.0f, 1.0f
	};
	unsigned int indices[] = {
		0, 1, 3,
		1, 2, 3
	};

	unsigned int VBO;
	glGenBuffers(1, &VBO);

	unsigned int VAO;
	glGenVertexArrays(1, &VAO);

	unsigned int EBO;
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	auto shaderProgramBuilder = ShaderProgramBuilder();
	auto vertexResult = shaderProgramBuilder.attachShader(GL_VERTEX_SHADER, "shader.vert");
	if (!vertexResult) {
		printf(vertexResult.error().c_str());
		return -1;
	}
	auto fragmentResult = shaderProgramBuilder.attachShader(GL_FRAGMENT_SHADER, "shader.frag");
	if (!fragmentResult) {
		printf(fragmentResult.error().c_str());
		return -1;
	}
	auto shaderProgram = shaderProgramBuilder.getShaderProgram();
	if (!shaderProgram) {
		printf(shaderProgram.error().c_str());
		return -1;
	}

	auto computeShaderProgramBuilder = ShaderProgramBuilder();
	auto computeResult = computeShaderProgramBuilder.attachShader(GL_COMPUTE_SHADER, "shader.comp");
	if (!computeResult) {
		printf(computeResult.error().c_str());
		return -1;
	}
	auto computeShaderProgram = computeShaderProgramBuilder.getShaderProgram();
	if (!computeShaderProgram) {
		printf(computeShaderProgram.error().c_str());
	}

	glUseProgram(*shaderProgram);
	glUniform1i(glGetUniformLocation(*shaderProgram, "texture0"), 0);

	while (!glfwWindowShouldClose(window)) {
		processInput(window);

		glUseProgram(*computeShaderProgram);
		glDispatchCompute(512, 512, 1);

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(*shaderProgram);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteProgram(*shaderProgram);
	
	glfwTerminate();
	return 0;
}