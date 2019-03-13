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

#include <cmath>
#include <iostream>

SDL_Window* displayWindow;

class Entity {
public:
	Entity(float width = 1.0f, float height = 1.0f, float x = 0.0f, float y = 0.0f, float velocity = 5.0f, float direction_x = 0.0f, float direction_y = 0.0f) {
		this->x = x;
		this->y = y;
		this->width = width;
		this->height = height;
		this->velocity = velocity;
		this->direction_x = direction_x;
		this->direction_y = direction_y;
	}

	float x;
	float y;
	float rotation;

	int textureID;

	float width;
	float height;
	float velocity;
	float direction_x;
	float direction_y;

	void Draw(ShaderProgram &p, float r = 1.0f, float g = 1.0f, float b = 1.0f) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(x, y, 0.0f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(width, height, 1.0f));
		p.SetModelMatrix(modelMatrix);
		p.SetColor(r, g, b, 0.0f);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

};

ShaderProgram program1;
glm::mat4 projectionMatrix = glm::mat4(1.0f);
glm::mat4 viewMatrix = glm::mat4(1.0f);

void Setup() {

	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Rocket League 111", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1080, 720, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
	glewInit();
#endif

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	glViewport(0, 0, 1080, 720);

	program1.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");

	float aspectRatio = (float)1080 / (float)720;
	float projectionHeight = 1.0f;
	float projectionWidth = projectionHeight * aspectRatio;
	float projectionDepth = 1.0f;
	projectionMatrix = glm::ortho(-projectionWidth, projectionWidth, -projectionHeight, projectionHeight, -projectionDepth, projectionDepth);
	//projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);
	glUseProgram(program1.programID);

	program1.SetProjectionMatrix(projectionMatrix);
	program1.SetViewMatrix(viewMatrix);
}

Entity ball(0.05f, 0.05f, 0.0f, 0.0f, 1.8f, 3 / 2, 3 / 2);
Entity leftPaddle(0.05f, 0.4f, -1.4f, 0.0f, 3.0f);
Entity rightPaddle(0.05f, 0.4f, 1.4f, 0.0f, 3.0f);
Entity lowerWall(3.8f, 0.01f, 0.0f, -1.0f, 0.0f);
Entity upperWall(3.8f, 0.01f, 0.0f, 1.0f, 0.0f);

void Update() {

}

bool detectCollision(Entity &first, Entity &second) {
	float p1 = abs(first.x - second.x) - (first.width + second.width) / 2.0f;
	float p2 = abs(first.y - second.y) - (first.height + second.height) / 2.0f;
	return p1 < 0.0f && p2 < 0.0f;
}

void Render() {

}

int main(int argc, char *argv[])
{
	
	Setup();
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::scale(modelMatrix, glm::vec3(0.01f, 2.0f, 1.0f));

	float lastFrameTicks = 0.0f;

	float ball_r, ball_g, ball_b;
	ball_r = ball_g = ball_b = 1.0f;

    SDL_Event event;
    bool done = false;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
        }

		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		glClear(GL_COLOR_BUFFER_BIT);

		float vertices[] = { -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f };
		glVertexAttribPointer(program1.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program1.positionAttribute);

		program1.SetColor(1.0f, 1.0f, 1.0f, 1.0f);
		program1.SetModelMatrix(modelMatrix);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		ball.Draw(program1, ball_r, ball_g, ball_b);

		leftPaddle.Draw(program1, 0.0f, 0.0f, 1.0f);
		rightPaddle.Draw(program1, 1.0f, 0.0f, 0.0f);

		ball.x += elapsed * ball.velocity * ball.direction_x;
		ball.y += elapsed * ball.velocity * ball.direction_y;

		const Uint8 *keys = SDL_GetKeyboardState(NULL);
		if (keys[SDL_SCANCODE_UP]) {
			if (!detectCollision(rightPaddle, upperWall)) {
				rightPaddle.y += elapsed * rightPaddle.velocity;
			}
		}

		else if (keys[SDL_SCANCODE_DOWN]) {
			if (!detectCollision(rightPaddle, lowerWall)) {
				rightPaddle.y += elapsed * -rightPaddle.velocity;
			}
		}

		if (keys[SDL_SCANCODE_W]) {
			if (!detectCollision(leftPaddle, upperWall)) {
				leftPaddle.y += elapsed * leftPaddle.velocity;
			}
		}

		else if (keys[SDL_SCANCODE_S]) {
			if (!detectCollision(leftPaddle, lowerWall)) {
				leftPaddle.y += elapsed * -leftPaddle.velocity;
			}
		}

		Update();

		if ((rightPaddle.x - (ball.x + elapsed * ball.velocity * ball.direction_x) - (ball.width + rightPaddle.width) / 2) < 0 && (abs(rightPaddle.y - (ball.y + elapsed * ball.velocity * ball.direction_y)) - (ball.height + rightPaddle.height) / 2) < 0) {
			ball.direction_x *= -1;
		}

		if ((ball.x + elapsed * ball.velocity * ball.direction_x - leftPaddle.x - (ball.width + leftPaddle.width) / 2) < 0 && (abs(leftPaddle.y - (ball.y + elapsed * ball.velocity * ball.direction_y)) - (ball.height + leftPaddle.height) / 2) < 0) {
			ball.direction_x *= -1;
		}

		/*if (detectCollision(ball, leftPaddle) || detectCollision(ball, rightPaddle)) {
			ball.direction_x *= -1;
		}*/

		if ((ball.y + elapsed * ball.velocity * ball.direction_y) > upperWall.y)
			ball.direction_y *= -1;

		if ((ball.y + elapsed * ball.velocity * ball.direction_y) < lowerWall.y)
			ball.direction_y *= -1;

		else if (ball.x + ball.width < leftPaddle.x) {

			ball.velocity *= -1.0f;
			ball_g = ball_b = 0.0f;
			ball_r = 1.0f;
			ball.x = 0.0f;
			ball.y = 0.0f;
		}

		else if (ball.x > rightPaddle.x + rightPaddle.width) {
			ball.velocity *= -1.0f;
			ball_r = ball_g = 0.0f;
			ball_b = 1.0f;
			ball.x = 0.0f;
			ball.y = 0.0f;
		}

		glDisableVertexAttribArray(program1.positionAttribute);

		SDL_GL_SwapWindow(displayWindow);

    }
	glDisable(GL_BLEND);
    SDL_Quit();
    return 0;
}
