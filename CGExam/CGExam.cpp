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
	float x;  // X position on the screen
	float y;  // Y position on the screen

	SpriteAnimation(GLuint texID, int r, int c, float duration, float frameWidth, float frameHeight, float posX, float posY)
		: textureID(texID), rows(r), columns(c), frameCount(r* c), currentFrame(0),
		frameDuration(duration), elapsedTime(0.0f), width(frameWidth), height(frameHeight),
		x(posX), y(posY) {}
};

// Shader compilation helper
GLuint compileShader(const char* source, GLenum shaderType) {
	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar infoLog[512];
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	return shader;
}

// Shader linking helper
GLuint linkShaderProgram(GLuint vertexShader, GLuint fragmentShader) {
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	GLint success;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		GLchar infoLog[512];
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}

	return shaderProgram;
}

// Helper function to load a texture from file
GLuint loadTexture(const char* filepath, const glm::vec3& colorKey, bool applyColorKey) {
	stbi_set_flip_vertically_on_load(true);
	int width, height, channels;
	unsigned char* image = stbi_load(filepath, &width, &height, &channels, STBI_rgb_alpha);
	if (!image) {
		std::cerr << "Failed to load texture: " << filepath << ". Reason: " << stbi_failure_reason() << std::endl;
		return 0;
	}

	if (applyColorKey) {
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				int index = (y * width + x) * 4;
				if (image[index] == static_cast<unsigned char>(colorKey.r) &&
					image[index + 1] == static_cast<unsigned char>(colorKey.g) &&
					image[index + 2] == static_cast<unsigned char>(colorKey.b)) {
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

// Function to update sprite animation frame
void updateSpriteAnimation(SpriteAnimation& animation, float deltaTime) {
	animation.elapsedTime += deltaTime;
	if (animation.elapsedTime >= animation.frameDuration) {
		animation.currentFrame = (animation.currentFrame + 1) % animation.frameCount;
		animation.elapsedTime = 0.0f;
	}
}

// Function to update texture coordinates
void updateTextureCoords(SpriteAnimation& animation, float* vertices) {
	int frameRow = animation.currentFrame / animation.columns;
	int frameCol = animation.currentFrame % animation.columns;

	float uSize = 1.0f / animation.columns;
	float vSize = 1.0f / animation.rows;

	float frameU = frameCol * uSize;
	float frameV = 1.0f - ((frameRow + 1) * vSize);

	vertices[2] = frameU;           vertices[3] = frameV;           // Bottom-Left
	vertices[6] = frameU + uSize;   vertices[7] = frameV;           // Bottom-Right
	vertices[10] = frameU + uSize;  vertices[11] = frameV + vSize;  // Top-Right
	vertices[14] = frameU;          vertices[15] = frameV + vSize;  // Top-Left
}

// General render function for any textured quad (background or sprite)
void renderObject(GLuint VAO, GLuint texture, const glm::mat4& model, GLuint shaderProgram, const glm::mat4& view, const glm::mat4& projection) {
	glUseProgram(shaderProgram);

	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	glBindTexture(GL_TEXTURE_2D, texture);
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
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

	const char* fragmentShaderSource = R"(#version 330 core
    out vec4 FragColor;
    in vec2 TexCoord;
    uniform sampler2D texture1;
    void main() {
        FragColor = texture(texture1, TexCoord);
    })";

	GLuint vertexShader = compileShader(vertexShaderSource, GL_VERTEX_SHADER);
	GLuint fragmentShader = compileShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
	GLuint shaderProgram = linkShaderProgram(vertexShader, fragmentShader);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// Vertices for a quad
	float vertices[] = {
		// Positions       // Texture Coords
		-0.5f, -0.5f,     0.0f, 0.0f,  // Bottom-Left
		 0.5f, -0.5f,     1.0f, 0.0f,  // Bottom-Right
		 0.5f,  0.5f,      1.0f, 1.0f,  // Top-Right
		-0.5f,  0.5f,     0.0f, 1.0f   // Top-Left
	};

	unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };

	// Setup VAO/VBO for sprites
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

	// Background quad setup (separate VAO/VBO)
	float backgroundVertices[] = {
		// Positions       // Texture Coords
		-400.0f, -300.0f,  0.0f, 0.0f,  // Bottom-Left
		 400.0f, -300.0f,  1.0f, 0.0f,  // Bottom-Right
		 400.0f,  300.0f,  1.0f, 1.0f,  // Top-Right
		-400.0f,  300.0f,  0.0f, 1.0f   // Top-Left
	};

	GLuint backgroundVAO, backgroundVBO, backgroundEBO;
	glGenVertexArrays(1, &backgroundVAO);
	glGenBuffers(1, &backgroundVBO);
	glGenBuffers(1, &backgroundEBO);

	glBindVertexArray(backgroundVAO);
	glBindBuffer(GL_ARRAY_BUFFER, backgroundVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(backgroundVertices), backgroundVertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, backgroundEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

#pragma region LoadTextures
	// Load textures
	glm::vec3 colorKey(255, 0, 255);
	GLuint backgroundTexture = loadTexture("../Assets/graphics/galaxy2.bmp", colorKey, false);

	GLuint bgRockL = loadTexture("../Assets/graphics/BlocksB.bmp", colorKey, true);
	GLuint bgRockR = loadTexture("../Assets/graphics/BlocksA.bmp", colorKey, true);

	GLuint loner = loadTexture("../Assets/graphics/LonerA.bmp", colorKey, true);
	GLuint loner2 = loadTexture("../Assets/graphics/LonerC.bmp", colorKey, true);
	GLuint drone = loadTexture("../Assets/graphics/drone.bmp", colorKey, true);
	GLuint rusher = loadTexture("../Assets/graphics/rusher.bmp", colorKey, true);

	GLuint steelAsteroid = loadTexture("../Assets/graphics/MAster96.bmp", colorKey, true);
	GLuint steelAsteroid2 = loadTexture("../Assets/graphics/MAster64.bmp", colorKey, true);
	GLuint rockAsteroid = loadTexture("../Assets/graphics/SAster96.bmp", colorKey, true);
	GLuint rockAsteroid2 = loadTexture("../Assets/graphics/GAster96.bmp", colorKey, true);
	
	GLuint ship = loadTexture("../Assets/graphics/ShipIdle.bmp", colorKey, true);
	GLuint clone = loadTexture("../Assets/graphics/clone.bmp", colorKey, true);
	GLuint shipJet = loadTexture("../Assets/graphics/Burner1.bmp", colorKey, true);
	GLuint missile = loadTexture("../Assets/graphics/missileA.bmp", colorKey, true);
	GLuint missile2 = loadTexture("../Assets/graphics/missileB.bmp", colorKey, true);

	GLuint life = loadTexture("../Assets/graphics/PULife.bmp", colorKey, true);



#pragma endregion

#pragma region CreateAnimations
	// Create animations
	SpriteAnimation bgRockAnim(bgRockL, 1, 1, 1.f, 1024.0f, 1024.0f, -150.0f, -290.0f);
	SpriteAnimation bgRockAnim2(bgRockR, 1, 1, 1.f, 1024.0f, 1024.0f, 500.0f, -500.0f);

	SpriteAnimation lonerAnim(loner, 4, 4, 0.1f, 64.0f, 64.0f, 0.0f, 150.0f); 
	SpriteAnimation lonerAnim2(loner, 4, 4, 0.1f, 64.0f, 64.0f, -60.f, 200.0f); 
	SpriteAnimation lonerAnim3(loner, 4, 4, 0.1f, 64.0f, 64.0f, 60.f, 200.0f); 
	SpriteAnimation lonerAnim4(loner2, 4, 4, 0.1f, 64.0f, 64.0f, -190.f, -100.0f); 


	SpriteAnimation droneAnim(drone, 2, 8, 0.1f, 32.0f, 32.0f, 200.f, -120.0f);
	SpriteAnimation droneAnim2(drone, 2, 8, 0.1f, 32.0f, 32.0f, 240.f, -100.0f);
	SpriteAnimation droneAnim3(drone, 2, 8, 0.1f, 32.0f, 32.0f, 280.f, -120.0f);
	SpriteAnimation droneAnim4(drone, 2, 8, 0.1f, 32.0f, 32.0f, 240.f, -140.0f);


	SpriteAnimation rusherAnim(rusher, 6, 4, 0.1f, 32.0f, 32.0f, -350.f, 200.0f);
	SpriteAnimation rusherAnim2(rusher, 6, 4, 0.1f, 32.0f, 32.0f, -310.f, 170.0f);
	SpriteAnimation rusherAnim3(rusher, 6, 4, 0.1f, 32.0f, 32.0f, -350.f, 140.0f);
	SpriteAnimation rusherAnim4(rusher, 6, 4, 0.1f, 32.0f, 32.0f, -310.f, 110.0f);
	SpriteAnimation rusherAnim5(rusher, 6, 4, 0.1f, 32.0f, 32.0f, -350.f, 80.0f);
	SpriteAnimation rusherAnim6(rusher, 6, 4, 0.1f, 32.0f, 32.0f, -310.f, 50.0f);

	SpriteAnimation sAsteroidAnim(steelAsteroid, 5, 5, 0.2f, 64.0f, 64.0f, 150.0f, 50.0f);
	SpriteAnimation sAsteroidAnim2(steelAsteroid2, 3, 8, 0.2f, 64.0f, 64.0f, 300.0f, 100.0f);

	SpriteAnimation rAsteroidAnim(rockAsteroid, 5, 5, 0.2f, 64.0f, 64.0f, 200.0f, 150.0f);
	SpriteAnimation rAsteroidAnim2(rockAsteroid, 5, 5, 0.2f, 64.0f, 64.0f, 300.0f, 220.0f);
	SpriteAnimation rAsteroidAnim3(rockAsteroid2, 5, 5, 0.2f, 64.0f, 64.0f, -100.0f, 25.0f);
	
	SpriteAnimation cloneAnim(clone, 4, 4, 0.1f, 32.0f, 32.0f, -50.0f, -200.0f);
	SpriteAnimation cloneAnim2(clone, 4, 4, 0.1f, 32.0f, 32.0f, 50.0f, -200.0f);

	SpriteAnimation shipAnim(ship, 1, 1, 1.f, 64.0f, 64.0f, 0.0f, -230.0f);

	SpriteAnimation shipJetAnim(shipJet, 1, 1, 1.f, 12.0f, 12.0f, -10.0f, -268.0f);
	SpriteAnimation shipJetAnim2(shipJet, 1, 1, 1.f, 12.0f, 12.0f, 10.0f, -268.0f);

	SpriteAnimation missileAnim(missile, 1, 1, 0.1f, 65.0f, 64.0f, -35.0f, -150.0f);
	SpriteAnimation missileAnim2(missile, 1, 1, 0.1f, 65.0f, 64.0f, 65.0f, -150.0f);
	SpriteAnimation missileAnim3(missile2, 1, 1, 0.1f, 65.0f, 64.0f, 17.0f, -180.0f);


	SpriteAnimation lifeAnim(life, 1, 1, 1.f, 32.0f, 32.0f, -380.0f, -280.0f);
	SpriteAnimation lifeAnim2(life, 1, 1, 1.f, 32.0f, 32.0f, -340.0f, -280.0f);
	SpriteAnimation lifeAnim3(life, 1, 1, 1.f, 32.0f, 32.0f, -300.0f, -280.0f);

#pragma endregion 

	
	//load struct vector with the animations
	std::vector<SpriteAnimation> animations = { bgRockAnim, bgRockAnim2, lonerAnim, lonerAnim2, lonerAnim3, lonerAnim4, droneAnim, droneAnim2, droneAnim3, droneAnim4, rusherAnim, rusherAnim2,
												rusherAnim3, rusherAnim4, rusherAnim5, rusherAnim6, sAsteroidAnim, sAsteroidAnim2, rAsteroidAnim, rAsteroidAnim2 ,
												rAsteroidAnim3, shipAnim, shipJetAnim, shipJetAnim2, cloneAnim, cloneAnim2, missileAnim, missileAnim2, missileAnim3,
												lifeAnim, lifeAnim2, lifeAnim3 };

	// Projection and view matrices
	glm::mat4 projection = glm::ortho(-400.0f, 400.0f, -300.0f, 300.0f, -1.0f, 1.0f);
	glm::mat4 view = glm::mat4(1.0f);

	// Background model matrix
	glm::mat4 backgroundModel = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 0.0f));


	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

		glClear(GL_COLOR_BUFFER_BIT);

		// Render static background (only once, no VBO updates)
		renderObject(backgroundVAO, backgroundTexture, backgroundModel, shaderProgram, view, projection);

		//renderObject(backgroundVAO, ship1, shipModel, shaderProgram, view, projection);

		// Render animations
		for (auto& anim : animations) {
			updateSpriteAnimation(anim, deltaTime);
			updateTextureCoords(anim, vertices);

			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

			glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(anim.x, anim.y, 0.0f));
			model = glm::scale(model, glm::vec3(anim.width, anim.height, 1.0f));
		

			renderObject(VAO, anim.textureID, model, shaderProgram, view, projection);
		}

		SDL_GL_SwapWindow(window);
	}

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}