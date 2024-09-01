#include <iostream>
#include <glad/glad.h>
#include <SDL.h>
#include "stb_image.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

// Camera settings
glm::vec3 cameraPos = glm::vec3(0.0f, 1.0f, 1.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.5f, 0.0f);

float deltaTime = 0.0f;
float lastFrameTime = SDL_GetTicks();

float lastX = 400, lastY = 300;
float pitch = 0.f;
float yaw = -90.f;
float fov = 45.0f;

float screenWidth = 800;
float screenHeight = 600;

SDL_Window* window;

void processKeyboard(float deltaTime)
{
	float cameraSpeed = 5.0f * deltaTime;
	glm::vec3 movement(0.0f);

	const Uint8* keyState = SDL_GetKeyboardState(NULL);
	if (keyState[SDL_SCANCODE_W] || keyState[SDL_SCANCODE_UP])
		movement += cameraSpeed * cameraFront;
	if (keyState[SDL_SCANCODE_S] || keyState[SDL_SCANCODE_DOWN])
		movement -= cameraSpeed * cameraFront;
	if (keyState[SDL_SCANCODE_A] || keyState[SDL_SCANCODE_LEFT])
		movement -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (keyState[SDL_SCANCODE_D] || keyState[SDL_SCANCODE_RIGHT])
		movement += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

	movement.y = 0.0f;
	cameraPos += movement;
}

void processMouse(SDL_Event ev)
{
	if (ev.type == SDL_MOUSEMOTION)
	{
		float xpos = ev.button.x;
		float ypos = ev.button.y;

		static bool firstMouse = true;
		if (firstMouse)
		{
			firstMouse = false;
			lastX = xpos;
			lastY = ypos;
		}

		float xoffset = xpos - lastX;
		float yoffset = lastY - ypos;
		SDL_WarpMouseInWindow(window, 400, 300);
		lastX = 400;
		lastY = 300;

		float sensitivity = 0.05f;
		xoffset *= sensitivity;
		yoffset *= sensitivity;

		yaw += xoffset;
		pitch += yoffset;

		pitch = glm::clamp(pitch, -89.0f, 89.0f);

		glm::vec3 front;
		front.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
		front.y = sin(glm::radians(pitch));
		front.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
		cameraFront = glm::normalize(front);
	}

	if (ev.type == SDL_MOUSEWHEEL)
	{
		fov -= ev.wheel.y;
		fov = glm::clamp(fov, 1.0f, 45.0f);
	}
}

void load_obj(const char* filename, std::vector<glm::vec4>& vertices, std::vector<glm::vec3>& normals, std::vector<glm::vec2>& texCoords, std::vector<GLushort>& elements)
{
	std::ifstream in(filename, std::ios::in);
	if (!in.is_open())
	{
		std::cerr << "Error Opening file " << filename << std::endl;
		exit(1);
	}

	std::string line;
	std::vector<glm::vec2> tempTexCoords;

	while (getline(in, line))
	{
		if (line.substr(0, 2) == "v ")
		{
			std::istringstream s(line.substr(2));
			glm::vec4 v;
			s >> v.x >> v.y >> v.z;
			v.w = 1.0f;
			vertices.push_back(v);
		}
		else if (line.substr(0, 3) == "vt ")
		{
			std::istringstream s(line.substr(3));
			glm::vec2 uv;
			s >> uv.x >> uv.y;
			tempTexCoords.push_back(uv);
		}
		else if (line.substr(0, 2) == "f ")
		{
			std::istringstream s(line.substr(2));
			GLushort vertexIndex[3], uvIndex[3], normalIndex[3];
			char slash;

			for (int i = 0; i < 3; i++)
			{
				s >> vertexIndex[i] >> slash >> uvIndex[i] >> slash >> normalIndex[i];
				vertexIndex[i]--;
				uvIndex[i]--;
				normalIndex[i]--;
			}

			elements.push_back(vertexIndex[0]);
			elements.push_back(vertexIndex[1]);
			elements.push_back(vertexIndex[2]);

			texCoords.push_back(tempTexCoords[uvIndex[0]]);
			texCoords.push_back(tempTexCoords[uvIndex[1]]);
			texCoords.push_back(tempTexCoords[uvIndex[2]]);
		}
	}

	normals.resize(vertices.size(), glm::vec3(0.0f, 0.0f, 0.0f));
	for (int i = 0; i < elements.size(); i += 3)
	{
		GLushort ia = elements[i];
		GLushort ib = elements[i + 1];
		GLushort ic = elements[i + 2];

		glm::vec3 normal = glm::normalize(glm::cross(glm::vec3(vertices[ib]) - glm::vec3(vertices[ia]), glm::vec3(vertices[ic]) - glm::vec3(vertices[ia])));
		normals[ia] = normals[ib] = normals[ic] = normal;
	}
}

GLuint loadTexture(const char* path)
{
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture: " << path << std::endl;
	}
	stbi_image_free(data);

	return textureID;
}

int main(int argc, char** argv)
{
	SDL_Init(SDL_INIT_VIDEO);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	window = SDL_CreateWindow("Computer Graphics Project 1", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenWidth, screenHeight, SDL_WINDOW_OPENGL);
	if (!window)
	{
		std::cerr << "Failed to create SDL Window" << std::endl;
		SDL_Quit();
		return -1;
	}

	SDL_GLContext context = SDL_GL_CreateContext(window);

	if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
	{
		std::cerr << "Failed to initialize GLAD" << std::endl;
		SDL_Quit();
		return -2;
	}

	SDL_SetRelativeMouseMode(SDL_TRUE);

	std::vector<glm::vec4> vertices;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> texCoords;
	std::vector<GLushort> elements;
	load_obj("suzanne.obj", vertices, normals, texCoords, elements);

	std::vector<float> vertexData;
	for (size_t i = 0; i < vertices.size(); ++i)
	{
		vertexData.push_back(vertices[i].x);
		vertexData.push_back(vertices[i].y);
		vertexData.push_back(vertices[i].z);
		vertexData.push_back(normals[i].x);
		vertexData.push_back(normals[i].y);
		vertexData.push_back(normals[i].z);
		vertexData.push_back(texCoords[i].x);
		vertexData.push_back(texCoords[i].y);
	}

	GLuint vbo, ebo, vao;
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);
	glGenVertexArrays(1, &vao);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements.size() * sizeof(GLushort), elements.data(), GL_STATIC_DRAW);

	const char* vertexShaderSource = R"glsl(
        #version 330 core
        in vec3 position;
        in vec3 normal;
        in vec2 texCoord;

        out vec3 FragPos;
        out vec3 Normal;
        out vec2 TexCoord;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        void main()
        {
            gl_Position = projection * view * model * vec4(position, 1.0);
            FragPos = vec3(model * vec4(position, 1.0));
            Normal = mat3(transpose(inverse(model))) * normal;  
            TexCoord = texCoord;
        }
    )glsl";

	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);

	GLint success;
	char infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}

	const char* fragmentShaderSource = R"glsl(
        #version 330 core
        in vec2 TexCoord;
        out vec4 outColor;

        uniform sampler2D ourTexture;

        void main()
        {
            outColor = texture(ourTexture, TexCoord);
        }
    )glsl";

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);

	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}

	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cerr << "ERROR::SHADER::PROGRAM::COMPILATION_FAILED\n" << infoLog << std::endl;
	}

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	GLuint texture = loadTexture("container.jpg");
	GLuint floorTexture = loadTexture("bricks.jpg");

	glUseProgram(shaderProgram);
	glUniform1i(glGetUniformLocation(shaderProgram, "ourTexture"), 0);

	GLuint modelLocation = glGetUniformLocation(shaderProgram, "model");
	GLuint viewLocation = glGetUniformLocation(shaderProgram, "view");
	GLuint projectionLocation = glGetUniformLocation(shaderProgram, "projection");

	glm::mat4 projection = glm::perspective(glm::radians(fov), screenWidth / screenHeight, 0.1f, 100.0f);
	glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));

	GLuint wallVBO, wallVAO, wallEBO;
	glGenBuffers(1, &wallVBO);
	glGenVertexArrays(1, &wallVAO);
	glGenBuffers(1, &wallEBO);

	glBindVertexArray(wallVAO);

	std::vector<float> wallVertices = {
		// First Wall (Left)
		-1.5f,  0.0f, -15.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
		-1.5f,  2.0f, -15.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,
		-1.5f,  0.0f,  5.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,
		-1.5f,  2.0f,  5.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,

		// Second Wall (Right)
		 1.5f,  0.0f, -15.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
		 1.5f,  2.0f, -15.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
		 1.5f,  0.0f,  5.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
		 1.5f,  2.0f,  5.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f
	};

	std::vector<GLuint> wallIndices = {
		0, 1, 2,
		1, 3, 2,
		4, 5, 6,
		5, 7, 6
	};

	glBindBuffer(GL_ARRAY_BUFFER, wallVBO);
	glBufferData(GL_ARRAY_BUFFER, wallVertices.size() * sizeof(float), wallVertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wallEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, wallIndices.size() * sizeof(GLuint), wallIndices.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	GLuint floorVBO, floorVAO, floorEBO;
	glGenBuffers(1, &floorVBO);
	glGenVertexArrays(1, &floorVAO);
	glGenBuffers(1, &floorEBO);

	glBindVertexArray(floorVAO);

	std::vector<float> floorVertices = {
		-1.5f,  0.0f, -15.0f, 0.0f, 0.0f, 0.0f,  0.0f, 0.0f,
		 1.5f,  0.0f, -15.0f, 0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
		-1.5f,  0.0f,   5.0f, 0.0f, 1.0f, 0.0f,  0.0f, 1.0f,
		 1.5f,  0.0f,   5.0f, 0.0f, 1.0f, 0.0f,  1.0f, 1.0f
	};

	std::vector<GLuint> floorIndices = {
		0, 1, 2,
		1, 3, 2
	};

	glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
	glBufferData(GL_ARRAY_BUFFER, floorVertices.size() * sizeof(float), floorVertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, floorEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, floorIndices.size() * sizeof(GLuint), floorIndices.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glClearColor(0.2f, 0.5f, 0.3f, 1.0f);
	glEnable(GL_DEPTH_TEST);

	bool gameIsRunning = true;
	SDL_Event windowEvent;
	while (gameIsRunning)
	{
		int now = SDL_GetTicks();
		deltaTime = (now - lastFrameTime) / 1000.0f;
		lastFrameTime = now;

		while (SDL_PollEvent(&windowEvent) != 0)
		{
			if (windowEvent.type == SDL_QUIT)
				gameIsRunning = false;

			processMouse(windowEvent);
		}
		processKeyboard(deltaTime);

		glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
		glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view));

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(shaderProgram);

		// Render Suzanne
		glBindVertexArray(vao);
		glBindTexture(GL_TEXTURE_2D, texture);
		glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.5f, -15.0f));
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
		glDrawElements(GL_TRIANGLES, elements.size(), GL_UNSIGNED_SHORT, 0);

		// Render walls
		glBindVertexArray(wallVAO);
		glBindTexture(GL_TEXTURE_2D, texture);
		model = glm::mat4(1.0f);
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
		glDrawElements(GL_TRIANGLES, wallIndices.size(), GL_UNSIGNED_INT, 0);

		// Render floor
		glBindVertexArray(floorVAO);
		glBindTexture(GL_TEXTURE_2D, floorTexture);
		model = glm::mat4(1.0f);
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
		glDrawElements(GL_TRIANGLES, floorIndices.size(), GL_UNSIGNED_INT, 0);

		SDL_GL_SwapWindow(window);
	}

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}