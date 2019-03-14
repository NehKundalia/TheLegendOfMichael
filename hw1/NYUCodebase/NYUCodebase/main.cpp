#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengl.h>
#include <SDL_image.h>

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

#include "ShaderProgram.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

SDL_Window* displayWindow;

GLuint LoadTexture(const char *filePath) {
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);

	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
		//assert(false);
	}
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	stbi_image_free(image);
	return textureID;
}

int main(int argc, char *argv[])
{

    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Injustice 5", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
    glewInit();
#endif

	glClearColor(0.0f, 0.0f, 1.0f, 1.0f);

	glViewport(0, 0, 640, 360);

	ShaderProgram program1;
	program1.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	ShaderProgram program2;
	program2.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");

	GLuint picTexture1 = LoadTexture(RESOURCE_FOLDER"doom.png");
	GLuint picTexture2 = LoadTexture(RESOURCE_FOLDER"vs.png");
	GLuint picTexture3 = LoadTexture(RESOURCE_FOLDER"magikarp.png");
	
	glm::mat4 projectionMatrix = glm::mat4(1.0f);
	glm::mat4 viewMatrix = glm::mat4(1.0f);
	projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);

	glm::mat4 modelMatrix1 = glm::mat4(1.0f);
	modelMatrix1 = glm::translate(modelMatrix1, glm::vec3(-1.2f, 0.0f, 0.0f));

	glm::mat4 modelMatrix2 = glm::mat4(1.0f);
	modelMatrix2 = glm::scale(modelMatrix2, glm::vec3(0.5f, 0.5f, 0.5f));
	
	glm::mat4 modelMatrix3 = glm::mat4(1.0f);
	modelMatrix3 = glm::translate(modelMatrix3, glm::vec3(1.2f, 0.0f, 0.0f));

	glm::mat4 modelMatrix4 = glm::mat4(1.0f);
	modelMatrix4 = glm::translate(modelMatrix4, glm::vec3(-1.2f, 0.7, 0.0f));
	modelMatrix4 = glm::scale(modelMatrix4, glm::vec3(1.0f, 0.1f, 1.0f));

	glm::mat4 modelMatrix5 = glm::mat4(1.0f);
	modelMatrix5 = glm::translate(modelMatrix5, glm::vec3(1.2f, 0.7, 0.0f));
	modelMatrix5 = glm::scale(modelMatrix5, glm::vec3(1.0f, 0.1f, 1.0f));

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glUseProgram(program1.programID);
	glUseProgram(program2.programID);


    SDL_Event event;
    bool done = false;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
        }

		glClearColor(0.5f, 0.0f, 0.5f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		float vertices[] = { -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f };
		glVertexAttribPointer(program1.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program1.positionAttribute);

		float texCoords[] = { 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f };
		glVertexAttribPointer(program1.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program1.texCoordAttribute);

		program1.SetProjectionMatrix(projectionMatrix);
		program1.SetViewMatrix(viewMatrix);

		program1.SetModelMatrix(modelMatrix1);
		glBindTexture(GL_TEXTURE_2D, picTexture1);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		program1.SetModelMatrix(modelMatrix2);
		glBindTexture(GL_TEXTURE_2D, picTexture2);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		program1.SetModelMatrix(modelMatrix3);
		glBindTexture(GL_TEXTURE_2D, picTexture3);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program1.positionAttribute);
		glDisableVertexAttribArray(program1.texCoordAttribute);

		glVertexAttribPointer(program2.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program2.positionAttribute);

		program2.SetColor(0.0f, 1.0f, 0.0f, 1.0f);
		program2.SetProjectionMatrix(projectionMatrix);
		program2.SetViewMatrix(viewMatrix);

		program2.SetModelMatrix(modelMatrix4);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		program2.SetModelMatrix(modelMatrix5);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program2.positionAttribute);

		SDL_GL_SwapWindow(displayWindow);

    }
	glDisable(GL_BLEND);
    SDL_Quit();
    return 0;
}
