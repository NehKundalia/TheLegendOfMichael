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

#include<vector>

#define MAX_BULLETS 30
#define NO_OF_ENEMIES 16
int bulletIndex = 0;

SDL_Window* displayWindow;

ShaderProgram program1;
ShaderProgram program2;

glm::mat4 projectionMatrix = glm::mat4(1.0f);
glm::mat4 viewMatrix = glm::mat4(1.0f);

enum GameMode { STATE_MAIN_MENU, STATE_GAME_LEVEL, STATE_GAME_OVER };
GameMode mode;

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

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	stbi_image_free(image);
	return textureID;
}

class SheetSprite {
public:
	SheetSprite() {};
	SheetSprite(unsigned int textureID, float u, float v, float width, float height, float size) {
		this->textureID = textureID;
		this->u = u;
		this->v = v;
		this->width = width;
		this->height = height;
		this->size = size;
	}
	float size;
	unsigned int textureID;
	float u;
	float v;
	float width;
	float height;

	void Draw(ShaderProgram &program) {
		glBindTexture(GL_TEXTURE_2D, textureID);
		GLfloat texCoords[] = {
		u, v + height,
		u + width, v,
		u, v,
		u + width, v,
		u, v + height,
		u + width, v + height
		};
		float aspect = width / height;
		float vertices[] = {
		-0.5f * size * aspect, -0.5f * size,
		0.5f * size * aspect, 0.5f * size,
		-0.5f * size * aspect, 0.5f * size,
		0.5f * size * aspect, 0.5f * size,
		-0.5f * size * aspect, -0.5f * size ,
		0.5f * size * aspect, -0.5f * size };

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program.positionAttribute);

		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

	}
};

class Entity {
public:
	Entity() {
		position = glm::vec3(0.0f, 0.0f, 0.0f);
		velocity = glm::vec3(0.0f, 0.0f, 0.0f);
		size = glm::vec3(1.0f, 1.0f, 1.0f);
	};
	Entity(glm::vec3& position, glm::vec3& velocity, glm::vec3& size, SheetSprite& sprite) {
		this->position = position;
		this->velocity = velocity;
		this->size = size;
		this->sprite = sprite;
	}
	void Draw(ShaderProgram &program) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, position);
		modelMatrix = glm::scale(modelMatrix, size);
		program1.SetModelMatrix(modelMatrix);
		sprite.Draw(program1);
	}

	void Update(float elapsed) {
		position += velocity * elapsed;
	}

	glm::vec3 position;
	glm::vec3 velocity;
	glm::vec3 size;
	SheetSprite sprite;
	float health;
	//bool isALive = true;
};

struct GameState {
	Entity player;
	Entity enemies[NO_OF_ENEMIES];
	Entity bullets[MAX_BULLETS];
	int enemiesAlive = 16;
} gameState;

void DrawText(ShaderProgram &program, int fontTexture, std::string text, float size, float spacing) {
	float character_size = 1.0 / 16.0f;
	std::vector<float> vertexData;
	std::vector<float> texCoordData;
	for (int i = 0; i < text.size(); i++) {
		int spriteIndex = (int)text[i];
		float texture_x = (float)(spriteIndex % 16) / 16.0f;
		float texture_y = (float)(spriteIndex / 16) / 16.0f;
		vertexData.insert(vertexData.end(), {
		((size + spacing) * i) + (-0.5f * size), 0.5f * size,
		((size + spacing) * i) + (-0.5f * size), -0.5f * size,
		((size + spacing) * i) + (0.5f * size), 0.5f * size,
		((size + spacing) * i) + (0.5f * size), -0.5f * size,
		((size + spacing) * i) + (0.5f * size), 0.5f * size,
		((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			});
		texCoordData.insert(texCoordData.end(), {
		texture_x, texture_y,
		texture_x, texture_y + character_size,
		texture_x + character_size, texture_y,
		texture_x + character_size, texture_y + character_size,
		texture_x + character_size, texture_y,
		texture_x, texture_y + character_size,
			});
	}
	glBindTexture(GL_TEXTURE_2D, fontTexture);

	float* vertices = vertexData.data();
	float* texCoords = texCoordData.data();

	GLuint picTexture1 = LoadTexture(RESOURCE_FOLDER"font1.png");
	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program.positionAttribute);

	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program.texCoordAttribute);

	glBindTexture(GL_TEXTURE_2D, picTexture1);
	glDrawArrays(GL_TRIANGLES, 0, text.size()*6);

	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);
}

void shootBullet() {
	gameState.bullets[bulletIndex].position.x = gameState.player.position.x;
	gameState.bullets[bulletIndex].position.y = gameState.player.position.y + 0.3f;
	bulletIndex++;
	if (bulletIndex > MAX_BULLETS - 1) {
		bulletIndex = 0;
	}
}

class MainMenu {
public:
	void Render() {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.85f, 0.0f, 0.0f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.5f, 0.5f, 0.0f));
		program1.SetModelMatrix(modelMatrix);
		glClear(GL_COLOR_BUFFER_BIT);
		DrawText(program1, 1, "SPACE INVADERS!", 0.5f, -0.25f);

		glm::mat4 modelMatrix2 = glm::mat4(1.0f);
		modelMatrix2 = glm::translate(modelMatrix2, glm::vec3(-0.5f, -0.1f, 0.0f));
		modelMatrix2 = glm::scale(modelMatrix2, glm::vec3(0.2f, 0.2f, 0.0f));
		program1.SetModelMatrix(modelMatrix2);
		DrawText(program1, 1, "(Spacebar to shoot)", 0.5f, -0.30f);
	}

	void ProcessUpdate(float elapsed, bool &done, SDL_Event &event) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			else if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
					mode = STATE_GAME_LEVEL;
				}
			}
		}
	}
};

float time = 0.0f;
class GameLevel {
public:
	void Render() {
		glClear(GL_COLOR_BUFFER_BIT);
		gameState.player.Draw(program1);
		for(int i = 0; i < NO_OF_ENEMIES; i++)
			gameState.enemies[i].Draw(program1);
		for (int i = 0; i < MAX_BULLETS; i++) {
			gameState.bullets[i].Draw(program1);
		}
	}

	void ProcessUpdate(float elapsed, bool &done, SDL_Event &event) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}

			else if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
					shootBullet();
				}
			}
		}
		const Uint8 *keys = SDL_GetKeyboardState(NULL);
		if (keys[SDL_SCANCODE_LEFT]) {
			if(gameState.player.position.x - gameState.player.size.x/2 > -1.77f)
				gameState.player.velocity.x = -3.0f;
		}
		if (keys[SDL_SCANCODE_RIGHT]) {
			if (gameState.player.position.x + gameState.player.size.x / 2 < 1.77f)
				gameState.player.velocity.x = 3.0f;
		}
	}

	void Update(float elapsed) {
		gameState.player.Update(elapsed);
		gameState.player.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
		for (int i = 0; i < MAX_BULLETS; i++)
			gameState.bullets[i].Update(elapsed);

		if (gameState.enemies[3].position.x + gameState.enemies[3].size.x/2 >= 1.77f)
			for (int i = 0; i < NO_OF_ENEMIES; i++)
				gameState.enemies[i].velocity.x = -1.0f;
	
		if (gameState.enemies[0].position.x - gameState.enemies[0].size.x / 2 <= -1.77f)
			for (int i = 0; i < NO_OF_ENEMIES; i++)
				gameState.enemies[i].velocity.x = 1.0f;

		for (int i = 0; i < NO_OF_ENEMIES; i++) {
			gameState.enemies[i].Update(elapsed);
			if (gameState.enemies[i].position.y <= gameState.player.position.y + 0.3f)
				mode = STATE_GAME_OVER;
		}


		time += elapsed;
		if (time > 3.0f) {
			for (int i = 0; i < NO_OF_ENEMIES; i++)
				gameState.enemies[i].position.y -= 0.3f;
			time = 0.0f;
		}

		for (int i = 0; i < MAX_BULLETS; i++) {
			for (int j = 0; j < NO_OF_ENEMIES; j++)
				if (gameState.bullets[i].position.y >= gameState.enemies[j].position.y - 0.1f && gameState.bullets[i].position.y <= gameState.enemies[j].position.y + 0.1f && (gameState.bullets[i].position.x > gameState.enemies[j].position.x - 0.1f) && (gameState.bullets[i].position.x < gameState.enemies[j].position.x + 0.1f)) {
					gameState.bullets[i].position.x = -2000.0f;
					gameState.enemies[j].position.y = 2000.0f;
					gameState.enemiesAlive -= 1;
				}
		}

		if (gameState.enemiesAlive == 0)
			mode = STATE_GAME_OVER;
	}
};

MainMenu mainMenu;
GameLevel gameLevel;

void Setup() {
	int resX = 1080;
	int resY = 720;
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Space Invaders", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, resX, resY, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
	glewInit();
#endif

	glViewport(0, 0, resX, resY);

	program1.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	program2.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");

	float aspectRatio = (float)resX / (float)resY;
	float projectionHeight = 1.0f;
	float projectionWidth = projectionHeight * aspectRatio;
	float projectionDepth = 1.0f;
	projectionMatrix = glm::ortho(-projectionWidth, projectionWidth, -projectionHeight, projectionHeight, -projectionDepth, projectionDepth);

	mode = STATE_MAIN_MENU;
	//height="40" width="16" y="125" x="827" name="fire00.png
	//height = "84" width = "93" y = "552" x = "425" name = "enemyGreen1.png"
	GLuint sheet = LoadTexture(RESOURCE_FOLDER"space_invaders.png");

	SheetSprite playerSprite = SheetSprite(sheet, 211.0f / 1024.0f, 941.0f / 1024.0f, 99.0f / 1024.0f, 75.0f / 1024.0f, 0.2f);
	SheetSprite bulletSprite = SheetSprite(sheet, 827.0f / 1024.0f, 125.0f / 1024.0f, 16.0f / 1024.0f, 40.0f / 1024.0f, 0.1f);
	SheetSprite enemySprite = SheetSprite(sheet, 425.0f / 1024.0f, 552.0f / 1024.0f, 93.0f / 1024.0f, 84.0f / 1024.0f, 0.2f);

	gameState.player = Entity(glm::vec3(0.0f, -0.85f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f), playerSprite);

	float posX = -0.9f;
	float posY = 0.985f;
	for (int i = 0; i < NO_OF_ENEMIES; i++) {
		if (i % 4 == 0) {
			posX = -0.9f;
			posY -= 0.2f;
		}
		gameState.enemies[i].sprite = enemySprite;
		gameState.enemies[i].position.x = posX;
		gameState.enemies[i].position.y = posY;
		gameState.enemies[i].velocity.x = 1.0f;
		posX += 0.4f;
	}
	for (int i = 0; i < MAX_BULLETS; i++) {
		gameState.bullets[i].sprite = bulletSprite;
		gameState.bullets[i].position.x = -2000.0f;
		gameState.bullets[i].velocity.y = 1.0f;
	}

	glUseProgram(program1.programID);
	glUseProgram(program2.programID);

	program1.SetProjectionMatrix(projectionMatrix);
	program1.SetViewMatrix(viewMatrix);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

class GameOver {
public:
	void Render() {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		if (gameState.enemiesAlive == 0) {
			modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.45f, 0.0f, 0.0f));
			modelMatrix = glm::scale(modelMatrix, glm::vec3(0.5f, 0.5f, 0.0f));
			program1.SetModelMatrix(modelMatrix);
			DrawText(program1, 1, "YOU WON!", 0.5f, -0.25f);
		}

		else if (gameState.enemiesAlive != 0) {
			modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.55f, 0.0f, 0.0f));
			modelMatrix = glm::scale(modelMatrix, glm::vec3(0.5f, 0.5f, 0.0f));
			program1.SetModelMatrix(modelMatrix);
			DrawText(program1, 1, "GAME OVER!", 0.5f, -0.25f);
		}
	}

	void ProcessUpdate(float elapsed, bool &done, SDL_Event &event) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
	}
};

GameOver gameOver;

void Render() {
	switch (mode) {
		case STATE_MAIN_MENU:
			mainMenu.Render();
			break;

		case STATE_GAME_LEVEL:
			gameLevel.Render();
			break;

		case STATE_GAME_OVER:
			gameOver.Render();
			break;
	}
	SDL_GL_SwapWindow(displayWindow);
}

void ProcessUpdate(float elapsed, bool &done, SDL_Event &event) {
	switch (mode) {
		case STATE_MAIN_MENU:
			mainMenu.ProcessUpdate(elapsed, done, event);
			break;

		case STATE_GAME_LEVEL:
			gameLevel.ProcessUpdate(elapsed, done, event);
			break;

		case STATE_GAME_OVER:
			gameOver.ProcessUpdate(elapsed, done, event);
			break;
	}
}

void Update(float elapsed) {
	if (mode == STATE_GAME_LEVEL)
		gameLevel.Update(elapsed);
}

int main(int argc, char *argv[])
{
	Setup();

	float lastFrameTicks = 0.0f;

    SDL_Event event;
    bool done = false;
    while (!done) {
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		ProcessUpdate(elapsed, done, event);

		Update(elapsed);

		Render();
    }

	glDisable(GL_BLEND);
    SDL_Quit();
    return 0;
}