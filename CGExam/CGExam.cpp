#include <iostream>
#include <glad/glad.h>
#include <SDL.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Vertex Shader source
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 position;
layout (location = 1) in vec2 texCoord;
out vec2 TexCoord;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main() {
    TexCoord = texCoord;
    gl_Position = projection * view * model * vec4(position, 0.0, 1.0);
})";

// Fragment Shader source
const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform sampler2D texture1;
void main() {
    FragColor = texture(texture1, TexCoord);
})";

// Helper function to load a texture from file
GLuint loadTexture(const char* filepath, const glm::vec3& colorKey, bool applyColorKey) {
	int width, height, channels;
	unsigned char* image = stbi_load(filepath, &width, &height, &channels, STBI_rgb_alpha);
	if (!image) {
		std::cerr << "Failed to load image: " << filepath << std::endl;
		return 0;
	}

	if (applyColorKey) {
		// Apply color keying to make specified color transparent
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				int index = (y * width + x) * 4;
				unsigned char r = image[index];
				unsigned char g = image[index + 1];
				unsigned char b = image[index + 2];
				if (r == static_cast<unsigned char>(colorKey.r) &&
					g == static_cast<unsigned char>(colorKey.g) &&
					b == static_cast<unsigned char>(colorKey.b)) {
					image[index + 3] = 0; // Set alpha to 0 (transparent)
				}
			}
		}
	}

	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(image);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return texture;
}

int main(int argc, char* args[]) {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cerr << "SDL couldn't initialize: " << SDL_GetError() << std::endl;
		return 1;
	}

	SDL_Window* window = SDL_CreateWindow("OpenGL Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL);
	if (window == nullptr) {
		std::cerr << "Window couldn't be created: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return 1;
	}

	SDL_GLContext glContext = SDL_GL_CreateContext(window);
	if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
		std::cerr << "Failed to initialize GLAD" << std::endl;
		SDL_DestroyWindow(window);
		SDL_Quit();
		return 1;
	}

	// Compile and link shaders
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	GLint success;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar infoLog[512];
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar infoLog[512];
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}

	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		GLchar infoLog[512];
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// Vertices for a quad with texture coordinates (Coordinates normalized to [-1, 1] range)
	float vertices[] = {
		// Positions   // Texture Coords
		-0.5f, -0.5f,    0.0f, 0.0f,
		 0.5f, -0.5f,    1.0f, 0.0f,
		 0.5f,  0.5f,    1.0f, 1.0f,
		-0.5f,  0.5f,    0.0f, 1.0f,
	};
	unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };

	GLuint VBO, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Vertex positions
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// Texture coordinates
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// Load multiple textures
	GLuint textures[4];
	glm::vec3 colorKey(255, 0, 255); // Pink color to be made transparent

	textures[0] = loadTexture("../Assets/graphics/cloneA.bmp", colorKey, true);
	textures[1] = loadTexture("../Assets/graphics/ShipIdle.bmp", colorKey, true);
	textures[2] = loadTexture("../Assets/graphics/SAster96A.bmp", colorKey, true);
	textures[3] = loadTexture("../Assets/graphics/galaxy2.bmp", colorKey, true);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Rendering loop
	while (true) {
		SDL_Event event;
		if (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				break;
			}
		}

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(shaderProgram);

		glm::mat4 view = glm::mat4(1.0f);
		glm::mat4 projection = glm::ortho(-400.0f, 400.0f, -300.0f, 300.0f, -1.0f, 1.0f);

		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

		glBindVertexArray(VAO);

		// Draw background
		glm::mat4 modelBG = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)); // Center right
		modelBG = glm::scale(modelBG, glm::vec3(800.0f, 600.0f, 1.0f)); // Scale to fixed size
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelBG));
		glBindTexture(GL_TEXTURE_2D, textures[3]);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		// Draw ship (rotate -90 degrees to face left)
		glm::mat4 modelShip = glm::translate(glm::mat4(1.0f), glm::vec3(-350.0f, 0.0f, 0.0f)); // Center left
		modelShip = glm::rotate(modelShip, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // Rotate -90 degrees
		modelShip = glm::scale(modelShip, glm::vec3(100.0f, 100.0f, 1.0f)); // Scale to fixed size
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelShip));
		glBindTexture(GL_TEXTURE_2D, textures[1]);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		// Draw clone (center right)
		glm::mat4 modelClone = glm::translate(glm::mat4(1.0f), glm::vec3(200.0f, 50.0f, 0.0f)); // Center right
		modelClone = glm::scale(modelClone, glm::vec3(100.0f, 100.0f, 1.0f)); // Scale to fixed size
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelClone));
		glBindTexture(GL_TEXTURE_2D, textures[0]);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		// Draw asteroid 1 (center right)
		glm::mat4 modelAsteroid1 = glm::translate(glm::mat4(1.0f), glm::vec3(200.0f, -50.0f, 0.0f)); // Center right
		modelAsteroid1 = glm::scale(modelAsteroid1, glm::vec3(300.0f, 300.0f, 1.0f)); // Scale to fixed size
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelAsteroid1));
		glBindTexture(GL_TEXTURE_2D, textures[2]);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		// Draw asteroid 1 (center right)
		glm::mat4 modelAsteroid2 = glm::translate(glm::mat4(1.0f), glm::vec3(100.0f, -100.0f, 0.0f)); // Center right
		modelAsteroid2 = glm::scale(modelAsteroid2, glm::vec3(300.0f, 300.0f, 1.0f)); // Scale to fixed size
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelAsteroid2));
		glBindTexture(GL_TEXTURE_2D, textures[2]);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);



		SDL_GL_SwapWindow(window);
	}

	// Cleanup resources
	for (GLuint texture : textures) {
		glDeleteTextures(1, &texture);
	}

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);

	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}