#include <iostream>
#include <glad/glad.h>
#include <SDL.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>



// Structure to hold information about an animated sprite
struct SpriteAnimation {
	GLuint textureID;
	int rows;
	int columns;
	int frameCount;
	int currentFrame;
	float frameDuration;
	float elapsedTime;
	float width;
	float height;

	SpriteAnimation(GLuint texID, int r, int c, float duration, float frameWidth, float frameHeight)
		: textureID(texID), rows(r), columns(c), frameCount(r* c), currentFrame(0),
		frameDuration(duration), elapsedTime(0.0f), width(frameWidth), height(frameHeight) {}
};

// Vertex Shader source
const char* vertexShaderSource = R"(#version 330 core
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
const char* fragmentShaderSource = R"(#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform sampler2D texture1;

void main() {
    FragColor = texture(texture1, TexCoord);
})";

// Helper function to load a texture from file with alpha keying and flip it vertically
GLuint loadTexture(const char* filepath, const glm::vec3& colorKey, bool applyColorKey) {
	// Flip the image vertically when loading
	stbi_set_flip_vertically_on_load(true);

	int width, height, channels;
	unsigned char* image = stbi_load(filepath, &width, &height, &channels, STBI_rgb_alpha);
	if (image == nullptr) {
		std::cerr << "Failed to load texture: " << filepath << ". Reason: " << stbi_failure_reason() << std::endl;
		return 0;
	}

	if (applyColorKey) {
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				int index = (y * width + x) * 4;
				unsigned char r = image[index];
				unsigned char g = image[index + 1];
				unsigned char b = image[index + 2];

				if (r == static_cast<unsigned char>(colorKey.r) &&
					g == static_cast<unsigned char>(colorKey.g) &&
					b == static_cast<unsigned char>(colorKey.b)) {
					image[index + 3] = 0; // Make the color transparent
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



void updateSpriteAnimation(SpriteAnimation& animation, float deltaTime, float* vertices) {
	animation.elapsedTime += deltaTime;

	if (animation.elapsedTime >= animation.frameDuration) {
		animation.currentFrame = (animation.currentFrame + 1) % animation.frameCount;
		animation.elapsedTime = 0.0f;
	}

	// Calculate the current frame's row and column
	int frameRow = animation.currentFrame / animation.columns;
	int frameCol = animation.currentFrame % animation.columns;

	float uSize = 1.0f / animation.columns;  // Width of each frame
	float vSize = 1.0f / animation.rows;     // Height of each frame

	// Calculate the U, V coordinates for the top-left corner of the current frame
	float frameU = frameCol * uSize;           // Horizontal (U) position of the frame
	float frameV = 1.0f - ((frameRow + 1) * vSize);  // Correctly calculate the V position for top-left

	// Update texture coordinates directly in the vertices array
	vertices[2] = frameU;           vertices[3] = frameV;           // Bottom-Left
	vertices[6] = frameU + uSize;   vertices[7] = frameV;           // Bottom-Right
	vertices[10] = frameU + uSize;  vertices[11] = frameV + vSize;  // Top-Right
	vertices[14] = frameU;          vertices[15] = frameV + vSize;  // Top-Left

	std::cout << "Current Frame: " << animation.currentFrame
		<< " | Row: " << frameRow
		<< " | Column: " << frameCol
		<< " | U: " << frameU
		<< " | V: " << frameV << std::endl;
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

	float vertices[] = {
		// Positions       // Texture Coords
		-0.5f, -0.5f,     0.0f, 0.0f,  // Bottom-Left
		 0.5f, -0.5f,     1.0f, 0.0f,  // Bottom-Right
		 0.5f,  0.5f,     1.0f, 1.0f,  // Top-Right
		-0.5f,  0.5f,     0.0f, 1.0f   // Top-Left
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

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glm::vec3 colorKey(255, 0, 255);
	GLuint spriteSheetTexture = loadTexture("../Assets/graphics/LonerA.bmp", colorKey, true);

	SpriteAnimation shipAnimation(spriteSheetTexture, 4, 4, 0.1f, 64.0f, 64.0f);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Enable wireframe mode
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Use this to return to normal mode



	float lastFrameTime = 0.0f;
	while (true) {
		float currentFrameTime = SDL_GetTicks() / 1000.0f;
		float deltaTime = currentFrameTime - lastFrameTime;
		lastFrameTime = currentFrameTime;

		SDL_Event event;
		if (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				break;
			}
		}

		// Update animation and vertices (texture coordinates)
		updateSpriteAnimation(shipAnimation, deltaTime, vertices);



		// Bind the VBO and update the entire vertex data
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		// Render the frame
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(shaderProgram);

		glm::mat4 view = glm::mat4(1.0f);
		glm::mat4 projection = glm::ortho(-400.0f, 400.0f, -300.0f, 300.0f, -1.0f, 1.0f);
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

		glm::mat4 modelShip = glm::translate(glm::mat4(1.0f), glm::vec3(-350.0f, 0.0f, 0.0f));
		modelShip = glm::scale(modelShip, glm::vec3(shipAnimation.width, shipAnimation.height, 1.0f));
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelShip));

		glBindVertexArray(VAO);
		glBindTexture(GL_TEXTURE_2D, shipAnimation.textureID);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		SDL_GL_SwapWindow(window);
	}

	glDeleteTextures(1, &spriteSheetTexture);
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}