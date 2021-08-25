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
#include "FlareMap.cpp"
#include <iostream>

#include <SDL_mixer.h>
#define MAX_BULLETS 30
#define NO_OF_ENEMIES 16

#define TILE_SIZE 0.2f
#define SPRITE_COUNT_X 27
#define SPRITE_COUNT_Y 18

#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6

GLuint sheet;
GLuint playerSheet;
GLuint player2Sheet;
GLuint ghostSheet;

bool invincible = false;
bool killed = false;

int one = 1;

SDL_Window* displayWindow;

ShaderProgram program1;
glm::mat4 projectionMatrix = glm::mat4(1.0f);
glm::mat4 viewMatrix = glm::mat4(1.0f);
glm::mat4 modelMatrix = glm::mat4(1.0f);

float time2 = 0.0f;

FlareMap map;

int obstacles[] = {0, 1, 2, 8, 9, 10, 27, 29, 35, 37, 54, 55, 56, 62, 63, 64, 82, 110, 225, 171, 199, 292, 346, 373, 435, 108, 81, 82, 83, 136, 137, 135, 172, 170, 226, 224};
int death[] = { 198 };
int upAnimation[] = { 2, 6, 10, 14 };
int downAnimation[] = { 0, 4, 8, 12 };
int leftAnimation[] = { 1, 5, 9, 13 };
int rightAnimation[] = { 3, 7, 11, 15 };

int numFrames = 4;
float animationElapsed = 0.0f;
float framesPerSecond = 480.0f;
int currentIndex = 0;

int goalIndex = 283;
int goalIndex2 = 266;
int goalIndex3 = 339;

float lerp(float v0, float v1, float t) {
	return (1.0 - t)*v0 + t * v1;
}

enum GameMode { STATE_MAIN_MENU, STATE_GAME_LEVEL, STATE_GAME_LEVEL2, STATE_GAME_LEVEL3, STATE_GAME_OVER, STATE_GAME_OVER2} mode;

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
} animation[16], animation2[16];

class Entity {
public:
	Entity() {
		position = glm::vec3(0.0f, 0.0f, 0.0f);
		velocity = glm::vec3(0.0f, 0.0f, 0.0f);
		size = glm::vec3(1.0f, 1.0f, 1.0f);
		acceleration = glm::vec3(2.0f, -4.0f, 0.0f);
	};
	Entity(glm::vec3& position, glm::vec3& velocity, glm::vec3& size, glm::vec3& acceleration, SheetSprite& sprite) {
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
		//velocity.x = lerp(velocity.x, 0.0f, elapsed * friction_x);
		//velocity.y = lerp(velocity.y, 0.0f, elapsed * friction_y);
		//velocity.x += acceleration.x * elapsed;

		position += velocity * elapsed;
	}

	glm::vec3 position;
	glm::vec3 velocity;
	glm::vec3 size;
	glm::vec3 acceleration;

	bool isStatic;
	bool collidedTop;
	bool collidedBottom;
	bool collidedLeft;
	bool collidedRight;

	bool invincible = false;
	SheetSprite sprite;
	//bool isALive = true;
};

struct GameState {
	Entity player, player2;
	Entity enemies[NO_OF_ENEMIES];
	Entity bullets[MAX_BULLETS];
	int enemiesAlive = 16;
	bool success = true;
	int bulletIndex = 0;
} gameState, gameState2, gameState3;

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
	glDrawArrays(GL_TRIANGLES, 0, text.size() * 6);

	glDisableVertexAttribArray(program.positionAttribute);
	glDisableVertexAttribArray(program.texCoordAttribute);
}

void shootBullet() {
	gameState.bullets[gameState.bulletIndex].position.x = gameState.player.position.x;
	gameState.bullets[gameState.bulletIndex].position.y = gameState.player.position.y + 0.3f;
	gameState.bulletIndex++;
	if (gameState.bulletIndex > MAX_BULLETS - 1) {
		gameState.bulletIndex = 0;
	}
}

class MainMenu {
public:
	void Render() {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.9f, 0.0f, 0.0f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.5f, 0.5f, 0.0f));
		program1.SetModelMatrix(modelMatrix);
		glClear(GL_COLOR_BUFFER_BIT);
		DrawText(program1, 1, "Legend of Michael", 0.5f, -0.25f);

		glm::mat4 modelMatrix2 = glm::mat4(1.0f);
		modelMatrix2 = glm::translate(modelMatrix2, glm::vec3(-0.5f, -0.1f, 0.0f));
		modelMatrix2 = glm::scale(modelMatrix2, glm::vec3(0.2f, 0.2f, 0.0f));
		program1.SetModelMatrix(modelMatrix2);
		DrawText(program1, 1, "(Spacebar to continue)", 0.5f, -0.30f);
	}

	void ProcessUpdate(float elapsed, bool &done, SDL_Event &event) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			else if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
					mode = STATE_GAME_LEVEL;
					Mix_Chunk *someSound;
					someSound = Mix_LoadWAV("jingles2.ogg");
					Mix_PlayChannel(-1, someSound, 0);
				}

				else if (event.type == SDL_KEYDOWN) {
					if ((event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)) {
						done = true;
					}
				}
			}
		}
	}
} mainMenu;

void worldToTileCoordinates(float worldX, float worldY, int *gridX, int *gridY) {
	*gridX = (int)(worldX / TILE_SIZE);
	*gridY = (int)(worldY / -TILE_SIZE);
}

void SetupLevel2() {
	int index = 479;

	SheetSprite carSprite(sheet, (float)(((int)index) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X, (float)(((int)index) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y, 1.0 / (float)SPRITE_COUNT_X, 1.0 / (float)SPRITE_COUNT_Y, 1.0f);
	map.entities.clear();
	map.Load("map.txt");
	for (int i = 0; i <= 5; i++) {
		float placeX = map.entities[i].x *TILE_SIZE + TILE_SIZE / 2;
		float placeY = map.entities[i].y* -TILE_SIZE + TILE_SIZE / 2;

		gameState.enemies[i] = Entity(glm::vec3(placeX - TILE_SIZE, placeY - TILE_SIZE, 0.0f), glm::vec3(0.0f, -2.0f, 0.0f), glm::vec3(TILE_SIZE, TILE_SIZE, 0.0f), glm::vec3(2.0f, -4.0f, 0.0f), carSprite);
	}

	gameState.enemies[0].position.x += TILE_SIZE;
	gameState.enemies[1].position.x += TILE_SIZE;
	gameState.enemies[1].position.y += 3*TILE_SIZE;
	gameState.enemies[2].position.x += 2*TILE_SIZE;
	gameState.enemies[3].position.x += 2 * TILE_SIZE;
	gameState.enemies[4].position.x += 3 * TILE_SIZE;

	float placeX = map.entities[6].x * TILE_SIZE + TILE_SIZE / 2;
	float placeY = map.entities[6].y * -TILE_SIZE + TILE_SIZE / 2;
	gameState.player.position = glm::vec3(placeX, placeY, 0.0f);
	glm::mat4 viewMatrix = glm::mat4(1.0f);
	//program1.SetViewMatrix(viewMatrix);

}

void SetupLevel3() {
	int index = 446;
	ghostSheet = LoadTexture(RESOURCE_FOLDER"spritesheet_rgba.png");
	SheetSprite ghostSprite(ghostSheet, (float)(((int)index) % 30) / (float)30, (float)(((int)index) / 30) / (float)30, 1.0 / (float)30, 1.0 / (float)30, 1.0f);
	map.entities.clear();
	map.Load("map3.txt");
	for (int i = 0; i <= 1; i++) {
		float placeX = map.entities[i].x * TILE_SIZE + TILE_SIZE / 2;
		float placeY = map.entities[i].y * -TILE_SIZE + TILE_SIZE / 2;
		gameState.player2.position = glm::vec3(placeX - TILE_SIZE, placeY - TILE_SIZE, 0.0f);
		//gameState.enemies[i] = Entity(glm::vec3(placeX - TILE_SIZE, placeY - TILE_SIZE, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(TILE_SIZE, TILE_SIZE, 0.0f), glm::vec3(2.0f, -4.0f, 0.0f), ghostSprite);
	}

	float placeX = map.entities[2].x * TILE_SIZE + TILE_SIZE / 2;
	float placeY = map.entities[2].y * -TILE_SIZE + TILE_SIZE / 2;
	gameState.player.position = glm::vec3(placeX, placeY, 0.0f);
	gameState.enemies[0].velocity.x = 1.0f;
	gameState.enemies[1].velocity.y = 1.0f;
	glm::mat4 viewMatrix = glm::mat4(1.0f);
	viewMatrix = glm::translate(viewMatrix, glm::vec3(-1.5f, 1.4f, 0.0f));

	program1.SetViewMatrix(viewMatrix);
}

class GameLevel {
public:
	void Render() {
		std::vector<float> vertexData;
		std::vector<float> texCoordData;
		for (int y = 0; y < map.mapHeight; y++) {
			for (int x = 0; x < map.mapWidth; x++) {
				if (true) {
					float u = (float)(((int)map.mapData[y][x]) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
					float v = (float)(((int)map.mapData[y][x]) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;
					float spriteWidth = 1.0f / (float)SPRITE_COUNT_X;
					float spriteHeight = 1.0f / (float)SPRITE_COUNT_Y;
					vertexData.insert(vertexData.end(), {
					TILE_SIZE * x, -TILE_SIZE * y,
					TILE_SIZE * x, (-TILE_SIZE * y) - TILE_SIZE,
					(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
					TILE_SIZE * x, -TILE_SIZE * y,
					(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
					(TILE_SIZE * x) + TILE_SIZE, -TILE_SIZE * y
						});
					texCoordData.insert(texCoordData.end(), {
					u, v,
					u, v + (spriteHeight),
					u + spriteWidth, v + (spriteHeight),
					u, v,
					u + spriteWidth, v + (spriteHeight),
					u + spriteWidth, v
						});
				}
			}
		}
		float* vertices = vertexData.data();
		float* texCoords = texCoordData.data();

		glm::mat4 modelMatrix = glm::mat4(1.0f);
		glm::mat4 viewMatrix = glm::mat4(1.0f);
		viewMatrix = glm::translate(viewMatrix, -gameState.player.position);

		program1.SetViewMatrix(viewMatrix);
		program1.SetModelMatrix(modelMatrix);

		glClear(GL_COLOR_BUFFER_BIT);
		glVertexAttribPointer(program1.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program1.positionAttribute);

		glVertexAttribPointer(program1.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program1.texCoordAttribute);

		glBindTexture(GL_TEXTURE_2D, sheet);
		glDrawArrays(GL_TRIANGLES, 0, vertexData.size() / 2);

		glDisableVertexAttribArray(program1.positionAttribute);
		glDisableVertexAttribArray(program1.texCoordAttribute);

		gameState.player.Draw(program1);
		//gameState.player2.Draw(program1);
		gameState.enemies[0].Draw(program1);
	}

	void ProcessUpdate(float elapsed, bool &done, SDL_Event &event) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}

			else if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
					shootBullet();
					//gameState.player.velocity.y = 5.0f;
				}
				else if ((event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)) {
					done = true;
				}
			}
		}
		const Uint8 *keys = SDL_GetKeyboardState(NULL);
		if (keys[SDL_SCANCODE_LEFT]) {
			int x, y;
			worldToTileCoordinates(gameState.player.position.x + TILE_SIZE/2, gameState.player.position.y, &x, &y);
			bool flag = false;
			for (int i : obstacles) {
				if (map.mapData[y][x-1] == i) {
					flag = true;
					break;
				}
			}
			if (!flag) {
				gameState.player.velocity.x = -1.0f;
				gameState.player2.velocity.x = -1.0f;
			}

			else {
				gameState.player.velocity.x = 0.0f;
				gameState.player2.velocity.x = 0.0f;
			}

			animationElapsed += elapsed;
			if (animationElapsed > 1.0 / framesPerSecond) {
				currentIndex++;
				animationElapsed = 0.0;
				if (currentIndex > numFrames - 1) {
					currentIndex = 0;
				}
			}
			gameState.player.sprite = animation[leftAnimation[currentIndex]];
			gameState.player2.sprite = animation2[leftAnimation[currentIndex]];

			Mix_Chunk *someSound;
			someSound = Mix_LoadWAV("footstep00.ogg");
			Mix_PlayChannel(-1, someSound, 0);
		}

		if (keys[SDL_SCANCODE_RIGHT]) {
			int x, y;
			worldToTileCoordinates(gameState.player.position.x - TILE_SIZE/2, gameState.player.position.y, &x, &y);
			bool flag = false;
			for (int i : obstacles) {
				if (map.mapData[y][x+1] == i) {
					flag = true;
					break;
				}
			}
			if (!flag) {
				gameState.player.velocity.x = 1.0f;
				gameState.player2.velocity.x = 1.0f;
			}

			else {
				gameState.player.velocity.x = 0.0f;
				gameState.player2.velocity.x = 0.0f;
			}

			animationElapsed += elapsed;
			if (animationElapsed > 1.0 / framesPerSecond) {
				currentIndex++;
				animationElapsed = 0.0;
				if (currentIndex > numFrames - 1) {
					currentIndex = 0;
				}
			}
			gameState.player.sprite = animation[rightAnimation[currentIndex]];
			gameState.player2.sprite = animation2[rightAnimation[currentIndex]];

			Mix_Chunk *someSound;
			someSound = Mix_LoadWAV("footstep00.ogg");
			Mix_PlayChannel(-1, someSound, 0);
		}

		if (keys[SDL_SCANCODE_UP]) {
			int x, y;
			worldToTileCoordinates(gameState.player.position.x, gameState.player.position.y + TILE_SIZE/2, &x, &y);
			bool flag = false;
			for (int i : obstacles) {
				if (map.mapData[y][x] == i) {
					flag = true;
					break;
				}
			}
			if (!flag) {
				gameState.player.velocity.y = 1.0f;
				gameState.player2.velocity.y = 1.0f;
			}

			else {
				gameState.player.velocity.y = 0.0f;
				gameState.player2.velocity.y = 0.0f;


			}

			animationElapsed += elapsed;
			if (animationElapsed > 1.0 / framesPerSecond) {
				currentIndex++;
				animationElapsed = 0.0;
				if (currentIndex > numFrames - 1) {
					currentIndex = 0;
				}
			}
			gameState.player.sprite = animation[upAnimation[currentIndex]];
			gameState.player2.sprite = animation2[upAnimation[currentIndex]];

			Mix_Chunk *someSound;
			someSound = Mix_LoadWAV("footstep00.ogg");
			Mix_PlayChannel(-1, someSound, 0);
		}

		if (keys[SDL_SCANCODE_DOWN]) {
			int x, y;
			worldToTileCoordinates(gameState.player.position.x, gameState.player.position.y - TILE_SIZE / 2, &x, &y);
			bool flag = false;
			for (int i : obstacles) {
				if (map.mapData[y][x] == i) {
					flag = true;
					break;
				}
			}
			if (!flag) {
				gameState.player.velocity.y = -1.0f;
				gameState.player2.velocity.y = -1.0f;
			}

			else {
				gameState.player.velocity.y = 0.0f;
				gameState.player2.velocity.y = 0.0f;
			}
			animationElapsed += elapsed;
			if (animationElapsed > 1.0 / framesPerSecond) {
				currentIndex++;
				animationElapsed = 0.0;
				if (currentIndex > numFrames - 1) {
					currentIndex = 0;
				}
			}
			gameState.player.sprite = animation[downAnimation[currentIndex]];
			gameState.player2.sprite = animation2[downAnimation[currentIndex]];

			Mix_Chunk *someSound;
			someSound = Mix_LoadWAV("footstep00.ogg");
			Mix_PlayChannel(-1, someSound, 0);
		}
	}

	void Update(float elapsed) {
		gameState.player.Update(elapsed);
		gameState.player2.Update(elapsed);
		gameState.enemies[0].Update(elapsed);
		gameState.player.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
		gameState.player2.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
		gameState.player.acceleration = glm::vec3(0.0f, 0.0f, 0.0f);

		int x, y;
		worldToTileCoordinates(gameState.player.position.x, gameState.player.position.y, &x, &y);

		if (map.mapData[y][x] == goalIndex) {
			mode = STATE_GAME_LEVEL2;
			Mix_Chunk *someSound;
			someSound = Mix_LoadWAV("jingles2.ogg");
			Mix_PlayChannel(-1, someSound, 0);
			SetupLevel2();
			gameState.success = true;
		}

		bool flag = false;
		for (int i : death)
			if (map.mapData[y][x] == i)
				flag = true;
		if (flag) {
			mode = STATE_GAME_OVER;
			gameState.success = false;
		}

		worldToTileCoordinates(gameState.enemies[0].position.x, gameState.enemies[0].position.y, &x, &y);
		flag = false;
		for (int i : obstacles) {
			if (map.mapData[y][x] == i) {
				flag = true;
				break;
			}
		}
		if (flag)
			gameState.enemies[0].velocity.x *= -1;

		int px, py;
		worldToTileCoordinates(gameState.player.position.x, gameState.player.position.y, &px, &py);
		if (px == x && py == y) {
			mode = STATE_GAME_OVER;
			gameState.success = false;
		}

		/*for (int i = 0; i < MAX_BULLETS; i++)
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
		}*/

		/*time += elapsed;
		if (time > 3.0f) {
			for (int i = 0; i < NO_OF_ENEMIES; i++)
				gameState.enemies[i].position.y -= 0.3f;
			time = 0.0f;
		}*/

		/*for (int i = 0; i < MAX_BULLETS; i++) {
			for (int j = 0; j < NO_OF_ENEMIES; j++)
				if (gameState.bullets[i].position.y >= gameState.enemies[j].position.y - 0.1f && gameState.bullets[i].position.y <= gameState.enemies[j].position.y + 0.1f && (gameState.bullets[i].position.x > gameState.enemies[j].position.x - 0.1f) && (gameState.bullets[i].position.x < gameState.enemies[j].position.x + 0.1f)) {
					gameState.bullets[i].position.x = -2000.0f;
					gameState.enemies[j].position.y = 2000.0f;
					gameState.enemiesAlive -= 1;
				}
		}*/
	}
} gameLevel;

class GameLevel2 {
public:
	void Render() {
		std::vector<float> vertexData;
		std::vector<float> texCoordData;
		for (int y = 0; y < map.mapHeight; y++) {
			for (int x = 0; x < map.mapWidth; x++) {
				if (true) {
					float u = (float)(((int)map.mapData[y][x]) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
					float v = (float)(((int)map.mapData[y][x]) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;
					float spriteWidth = 1.0f / (float)SPRITE_COUNT_X;
					float spriteHeight = 1.0f / (float)SPRITE_COUNT_Y;
					vertexData.insert(vertexData.end(), {
					TILE_SIZE * x, -TILE_SIZE * y,
					TILE_SIZE * x, (-TILE_SIZE * y) - TILE_SIZE,
					(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
					TILE_SIZE * x, -TILE_SIZE * y,
					(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
					(TILE_SIZE * x) + TILE_SIZE, -TILE_SIZE * y
						});
					texCoordData.insert(texCoordData.end(), {
					u, v,
					u, v + (spriteHeight),
					u + spriteWidth, v + (spriteHeight),
					u, v,
					u + spriteWidth, v + (spriteHeight),
					u + spriteWidth, v
						});
				}
			}
		}
		float* vertices = vertexData.data();
		float* texCoords = texCoordData.data();

		glm::mat4 modelMatrix = glm::mat4(1.0f);
		glm::mat4 viewMatrix = glm::mat4(1.0f);
		viewMatrix = glm::translate(viewMatrix, -gameState.player.position);

		program1.SetViewMatrix(viewMatrix);
		program1.SetModelMatrix(modelMatrix);

		glClear(GL_COLOR_BUFFER_BIT);
		glVertexAttribPointer(program1.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program1.positionAttribute);

		glVertexAttribPointer(program1.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program1.texCoordAttribute);

		glBindTexture(GL_TEXTURE_2D, sheet);
		glDrawArrays(GL_TRIANGLES, 0, vertexData.size() / 2);

		glDisableVertexAttribArray(program1.positionAttribute);
		glDisableVertexAttribArray(program1.texCoordAttribute);

		gameState.player.Draw(program1);
		//gameState.player2.Draw(program1);
		for(int i = 0; i < 6; i++)
			gameState.enemies[i].Draw(program1);
	}

	void ProcessUpdate(float elapsed, bool &done, SDL_Event &event) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}

			else if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
					shootBullet();
					//gameState.player.velocity.y = 5.0f;
				}
				else if ((event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)) {
					done = true;
				}
			}
		}
		const Uint8 *keys = SDL_GetKeyboardState(NULL);
		if (keys[SDL_SCANCODE_LEFT]) {
			int x, y;
			worldToTileCoordinates(gameState.player.position.x + TILE_SIZE / 2, gameState.player.position.y, &x, &y);
			bool flag = false;
			for (int i : obstacles) {
				if (map.mapData[y][x-1] == i) {
					flag = true;
					break;
				}
			}
			if (!flag) {
				gameState.player.velocity.x = -1.0f;
				gameState.player2.velocity.x = -1.0f;
			}

			else {
				gameState.player.velocity.x = 0.0f;
				gameState.player2.velocity.x = 0.0f;
			}

			animationElapsed += elapsed;
			if (animationElapsed > 1.0 / framesPerSecond) {
				currentIndex++;
				animationElapsed = 0.0;
				if (currentIndex > numFrames - 1) {
					currentIndex = 0;
				}
			}
			gameState.player.sprite = animation[leftAnimation[currentIndex]];
			gameState.player2.sprite = animation2[leftAnimation[currentIndex]];

			Mix_Chunk *someSound;
			someSound = Mix_LoadWAV("footstep02.ogg");
			Mix_PlayChannel(-1, someSound, 0);
		}

		if (keys[SDL_SCANCODE_RIGHT]) {
			int x, y;
			worldToTileCoordinates(gameState.player.position.x - TILE_SIZE / 2, gameState.player.position.y, &x, &y);
			bool flag = false;
			for (int i : obstacles) {
				if (map.mapData[y][x+1] == i) {
					flag = true;
					break;
				}
			}
			if (!flag) {
				gameState.player.velocity.x = 1.0f;
				gameState.player2.velocity.x = 1.0f;
			}

			else {
				gameState.player.velocity.x = 0.0f;
				gameState.player2.velocity.x = 0.0f;
			}

			animationElapsed += elapsed;
			if (animationElapsed > 1.0 / framesPerSecond) {
				currentIndex++;
				animationElapsed = 0.0;
				if (currentIndex > numFrames - 1) {
					currentIndex = 0;
				}
			}
			gameState.player.sprite = animation[rightAnimation[currentIndex]];
			gameState.player2.sprite = animation2[rightAnimation[currentIndex]];

			Mix_Chunk *someSound;
			someSound = Mix_LoadWAV("footstep02.ogg");
			Mix_PlayChannel(-1, someSound, 0);
		}

		if (keys[SDL_SCANCODE_UP]) {
			int x, y;
			worldToTileCoordinates(gameState.player.position.x, gameState.player.position.y + TILE_SIZE / 2, &x, &y);
			bool flag = false;
			for (int i : obstacles) {
				if (map.mapData[y][x] == i) {
					flag = true;
					break;
				}
			}
			if (!flag) {
				gameState.player.velocity.y = 1.0f;
				gameState.player2.velocity.y = 1.0f;
			}

			else {
				gameState.player.velocity.y = 0.0f;
				gameState.player2.velocity.y = 0.0f;


			}

			animationElapsed += elapsed;
			if (animationElapsed > 1.0 / framesPerSecond) {
				currentIndex++;
				animationElapsed = 0.0;
				if (currentIndex > numFrames - 1) {
					currentIndex = 0;
				}
			}
			gameState.player.sprite = animation[upAnimation[currentIndex]];
			gameState.player2.sprite = animation2[upAnimation[currentIndex]];

			Mix_Chunk *someSound;
			someSound = Mix_LoadWAV("footstep02.ogg");
			Mix_PlayChannel(-1, someSound, 0);
		}

		if (keys[SDL_SCANCODE_DOWN]) {
			int x, y;
			worldToTileCoordinates(gameState.player.position.x, gameState.player.position.y - TILE_SIZE / 2, &x, &y);
			bool flag = false;
			for (int i : obstacles) {
				if (map.mapData[y][x] == i) {
					flag = true;
					break;
				}
			}
			if (!flag) {
				gameState.player.velocity.y = -1.0f;
				gameState.player2.velocity.y = -1.0f;
			}

			else {
				gameState.player.velocity.y = 0.0f;
				gameState.player2.velocity.y = 0.0f;
			}
			animationElapsed += elapsed;
			if (animationElapsed > 1.0 / framesPerSecond) {
				currentIndex++;
				animationElapsed = 0.0;
				if (currentIndex > numFrames - 1) {
					currentIndex = 0;
				}
			}
			gameState.player.sprite = animation[downAnimation[currentIndex]];
			gameState.player2.sprite = animation2[downAnimation[currentIndex]];

			Mix_Chunk *someSound;
			someSound = Mix_LoadWAV("footstep02.ogg");
			Mix_PlayChannel(-1, someSound, 0);
		}
	}

	void Update(float elapsed) {
		gameState.player.Update(elapsed);
		gameState.player2.Update(elapsed);
		for (int i = 0; i < 6; i++) {
			gameState.enemies[i].Update(elapsed);
		}
		
		gameState.player.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
		gameState.player2.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
		gameState.player.acceleration = glm::vec3(0.0f, 0.0f, 0.0f);

		int x, y;
		worldToTileCoordinates(gameState.player.position.x, gameState.player.position.y, &x, &y);

		bool flag = false;
		for (int i : death)
			if (map.mapData[y][x] == i)
				flag = true;

		/*if (map.mapData[y][x+1] == goalIndex2) {
			glm::mat4 modelMatrix = glm::mat4(1.0f);
			modelMatrix = glm::translate(modelMatrix, gameState.player.position);
			modelMatrix = glm::scale(modelMatrix, glm::vec3(0.1f, 0.1f, 0.0f));
			program1.SetModelMatrix(modelMatrix);
			//glClear(GL_COLOR_BUFFER_BIT);
			DrawText(program1, 1, "Go Away", 0.1f, 0.25f);
		}*/

		if (map.mapData[y][x] == goalIndex2) {
			mode = STATE_GAME_LEVEL3;
			gameState.success = true;
			Mix_Chunk *someSound;
			someSound = Mix_LoadWAV("jingles2.ogg");
			Mix_PlayChannel(-1, someSound, 0);
			SetupLevel3();
		}

		if (flag) {
			mode = STATE_GAME_OVER;
			gameState.success = false;
		}

		int px, py;
		worldToTileCoordinates(gameState.player.position.x, gameState.player.position.y, &px, &py);

		for (int i = 0;i < 6;i++) {
			worldToTileCoordinates(gameState.enemies[i].position.x, gameState.enemies[i].position.y, &x, &y);
			flag = false;
			for (int j : obstacles) {
				if (map.mapData[y][x] == j) {
					flag = true;
					break;
				}
			}
			if (flag)
				gameState.enemies[i].velocity.y *= -1;

			if (px == x && py == y) {
				mode = STATE_GAME_OVER;
				gameState.success = false;
			}
		}
	}
} gameLevel2;

class GameLevel3 {
public:
	void Render() {
		std::vector<float> vertexData;
		std::vector<float> texCoordData;
		for (int y = 0; y < map.mapHeight; y++) {
			for (int x = 0; x < map.mapWidth; x++) {
				if (true) {
					float u = (float)(((int)map.mapData[y][x]) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
					float v = (float)(((int)map.mapData[y][x]) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;
					float spriteWidth = 1.0f / (float)SPRITE_COUNT_X;
					float spriteHeight = 1.0f / (float)SPRITE_COUNT_Y;
					vertexData.insert(vertexData.end(), {
					TILE_SIZE * x, -TILE_SIZE * y,
					TILE_SIZE * x, (-TILE_SIZE * y) - TILE_SIZE,
					(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
					TILE_SIZE * x, -TILE_SIZE * y,
					(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
					(TILE_SIZE * x) + TILE_SIZE, -TILE_SIZE * y
						});
					texCoordData.insert(texCoordData.end(), {
					u, v,
					u, v + (spriteHeight),
					u + spriteWidth, v + (spriteHeight),
					u, v,
					u + spriteWidth, v + (spriteHeight),
					u + spriteWidth, v
						});
				}
			}
		}
		float* vertices = vertexData.data();
		float* texCoords = texCoordData.data();

		glm::mat4 modelMatrix = glm::mat4(1.0f);
		glm::mat4 viewMatrix = glm::mat4(1.0f);
		//viewMatrix = glm::translate(viewMatrix, -gameState.player.position);

		//program1.SetViewMatrix(viewMatrix);
		program1.SetModelMatrix(modelMatrix);

		glClear(GL_COLOR_BUFFER_BIT);
		glVertexAttribPointer(program1.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program1.positionAttribute);

		glVertexAttribPointer(program1.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program1.texCoordAttribute);

		glBindTexture(GL_TEXTURE_2D, sheet);
		glDrawArrays(GL_TRIANGLES, 0, vertexData.size() / 2);

		glDisableVertexAttribArray(program1.positionAttribute);
		glDisableVertexAttribArray(program1.texCoordAttribute);

		gameState.player.Draw(program1);
		gameState.player2.Draw(program1);
		//for (int i = 0; i <= 1; i++)
			//gameState.enemies[i].Draw(program1);
	}

	void ProcessUpdate(float elapsed, bool &done, SDL_Event &event) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}

			else if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
					shootBullet();
					//gameState.player.velocity.y = 5.0f;
				}
				else if ((event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)) {
					done = true;
				}
			}
		}
		const Uint8 *keys = SDL_GetKeyboardState(NULL);
		if (keys[SDL_SCANCODE_LEFT]) {
			int x, y;
			worldToTileCoordinates(gameState.player.position.x + TILE_SIZE / 2, gameState.player.position.y, &x, &y);
			bool flag = false;
			for (int i : obstacles) {
				if (map.mapData[y][x - 1] == i) {
					flag = true;
					break;
				}
			}
			if (!flag) {
				gameState.player.velocity.x = -1.0f;
			}

			else {
				gameState.player.velocity.x = 0.0f;
			}

			animationElapsed += elapsed;
			if (animationElapsed > 1.0 / framesPerSecond) {
				currentIndex++;
				animationElapsed = 0.0;
				if (currentIndex > numFrames - 1) {
					currentIndex = 0;
				}
			}
			gameState.player.sprite = animation[leftAnimation[currentIndex]];

			Mix_Chunk *someSound;
			someSound = Mix_LoadWAV("footstep04.ogg");
			Mix_PlayChannel(-1, someSound, 0);
		}

		if (keys[SDL_SCANCODE_RIGHT]) {
			int x, y;
			worldToTileCoordinates(gameState.player.position.x - TILE_SIZE / 2, gameState.player.position.y, &x, &y);
			bool flag = false;
			for (int i : obstacles) {
				if (map.mapData[y][x + 1] == i) {
					flag = true;
					break;
				}
			}
			if (!flag) {
				gameState.player.velocity.x = 1.0f;
			}

			else {
				gameState.player.velocity.x = 0.0f;
			}

			animationElapsed += elapsed;
			if (animationElapsed > 1.0 / framesPerSecond) {
				currentIndex++;
				animationElapsed = 0.0;
				if (currentIndex > numFrames - 1) {
					currentIndex = 0;
				}
			}
			gameState.player.sprite = animation[rightAnimation[currentIndex]];

			Mix_Chunk *someSound;
			someSound = Mix_LoadWAV("footstep04.ogg");
			Mix_PlayChannel(-1, someSound, 0);
		}

		if (keys[SDL_SCANCODE_UP]) {
			int x, y;
			worldToTileCoordinates(gameState.player.position.x, gameState.player.position.y + TILE_SIZE / 2, &x, &y);
			bool flag = false;
			for (int i : obstacles) {
				if (map.mapData[y][x] == i) {
					flag = true;
					break;
				}
			}
			if (!flag) {
				gameState.player.velocity.y = 1.0f;
			}

			else {
				gameState.player.velocity.y = 0.0f;


			}

			animationElapsed += elapsed;
			if (animationElapsed > 1.0 / framesPerSecond) {
				currentIndex++;
				animationElapsed = 0.0;
				if (currentIndex > numFrames - 1) {
					currentIndex = 0;
				}
			}
			gameState.player.sprite = animation[upAnimation[currentIndex]];

			Mix_Chunk *someSound;
			someSound = Mix_LoadWAV("footstep04.ogg");
			Mix_PlayChannel(-1, someSound, 0);
		}

		if (keys[SDL_SCANCODE_DOWN]) {
			int x, y;
			worldToTileCoordinates(gameState.player.position.x, gameState.player.position.y - TILE_SIZE / 2, &x, &y);
			bool flag = false;
			for (int i : obstacles) {
				if (map.mapData[y][x] == i) {
					flag = true;
					break;
				}
			}
			if (!flag) {
				gameState.player.velocity.y = -1.0f;
			}

			else {
				gameState.player.velocity.y = 0.0f;
			}
			animationElapsed += elapsed;
			if (animationElapsed > 1.0 / framesPerSecond) {
				currentIndex++;
				animationElapsed = 0.0;
				if (currentIndex > numFrames - 1) {
					currentIndex = 0;
				}
			}
			gameState.player.sprite = animation[downAnimation[currentIndex]];

			Mix_Chunk *someSound;
			someSound = Mix_LoadWAV("footstep04.ogg");
			Mix_PlayChannel(-1, someSound, 0);
		}


		if (keys[SDL_SCANCODE_A]) {
			int x, y;
			worldToTileCoordinates(gameState.player2.position.x + TILE_SIZE / 2, gameState.player2.position.y, &x, &y);
			bool flag = false;
			for (int i : obstacles) {
				if (map.mapData[y][x - 1] == i) {
					flag = true;
					break;
				}
			}
			if (!flag) {
				gameState.player2.velocity.x = -1.0f;
			}

			else {
				gameState.player2.velocity.x = 0.0f;
			}

			animationElapsed += elapsed;
			if (animationElapsed > 1.0 / framesPerSecond) {
				currentIndex++;
				animationElapsed = 0.0;
				if (currentIndex > numFrames - 1) {
					currentIndex = 0;
				}
			}
			gameState.player2.sprite = animation2[leftAnimation[currentIndex]];
		}

		if (keys[SDL_SCANCODE_D]) {
			int x, y;
			worldToTileCoordinates(gameState.player2.position.x - TILE_SIZE / 2, gameState.player2.position.y, &x, &y);
			bool flag = false;
			for (int i : obstacles) {
				if (map.mapData[y][x + 1] == i) {
					flag = true;
					break;
				}
			}
			if (!flag) {
				gameState.player2.velocity.x = 1.0f;
			}

			else {
				gameState.player2.velocity.x = 0.0f;
			}

			animationElapsed += elapsed;
			if (animationElapsed > 1.0 / framesPerSecond) {
				currentIndex++;
				animationElapsed = 0.0;
				if (currentIndex > numFrames - 1) {
					currentIndex = 0;
				}
			}
			gameState.player2.sprite = animation2[rightAnimation[currentIndex]];

			Mix_Chunk *someSound;
			someSound = Mix_LoadWAV("footstep04.ogg");
			Mix_PlayChannel(-1, someSound, 0);
		}

		if (keys[SDL_SCANCODE_W]) {
			int x, y;
			worldToTileCoordinates(gameState.player2.position.x, gameState.player2.position.y + TILE_SIZE / 2, &x, &y);
			bool flag = false;
			for (int i : obstacles) {
				if (map.mapData[y][x] == i) {
					flag = true;
					break;
				}
			}
			if (!flag) {
				gameState.player2.velocity.y = 1.0f;
			}

			else {
				gameState.player2.velocity.y = 0.0f;


			}

			animationElapsed += elapsed;
			if (animationElapsed > 1.0 / framesPerSecond) {
				currentIndex++;
				animationElapsed = 0.0;
				if (currentIndex > numFrames - 1) {
					currentIndex = 0;
				}
			}
			gameState.player2.sprite = animation2[upAnimation[currentIndex]];

			Mix_Chunk *someSound;
			someSound = Mix_LoadWAV("footstep04.ogg");
			Mix_PlayChannel(-1, someSound, 0);
		}

		if (keys[SDL_SCANCODE_S]) {
			int x, y;
			worldToTileCoordinates(gameState.player2.position.x, gameState.player2.position.y - TILE_SIZE / 2, &x, &y);
			bool flag = false;
			for (int i : obstacles) {
				if (map.mapData[y][x] == i) {
					flag = true;
					break;
				}
			}
			if (!flag) {
				gameState.player2.velocity.y = -1.0f;
			}

			else {
				gameState.player2.velocity.y = 0.0f;
			}
			animationElapsed += elapsed;
			if (animationElapsed > 1.0 / framesPerSecond) {
				currentIndex++;
				animationElapsed = 0.0;
				if (currentIndex > numFrames - 1) {
					currentIndex = 0;
				}
			}
			gameState.player2.sprite = animation2[downAnimation[currentIndex]];

			Mix_Chunk *someSound;
			someSound = Mix_LoadWAV("footstep04.ogg");
			Mix_PlayChannel(-1, someSound, 0);
		}


	}

	void Update(float elapsed) {
		int key = 307;
		bool keyGone = true;
		gameState.player.Update(elapsed);
		gameState.player2.Update(elapsed);
		/*for (int i = 0; i < 2; i++) {
			gameState.enemies[i].Update(elapsed);
		}*/

		gameState.player.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
		gameState.player2.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
		gameState.player.acceleration = glm::vec3(0.0f, 0.0f, 0.0f);

		int x, y;
		worldToTileCoordinates(gameState.player.position.x, gameState.player.position.y, &x, &y);

		bool flag = false;
		for (int i : death)
			if (map.mapData[y][x] == i)
					flag = true;

		if (flag) {
			mode = STATE_GAME_OVER2;
			gameState.success = false;
		}

		int p1x, p1y;
		worldToTileCoordinates(gameState.player.position.x, gameState.player.position.y, &p1x, &p1y);

		int p2x, p2y;
		worldToTileCoordinates(gameState.player2.position.x, gameState.player2.position.y, &p2x, &p2y);

		flag = false;
		for (int i : death)
			if (map.mapData[p2y][p2x] == i)
				flag = true;
		if (flag) {
			mode = STATE_GAME_OVER2;
			gameState.success = true;
		}

		for (int i = 0; i < map.mapHeight; i++) {
			for (int j = 0; j < map.mapWidth; j++) {
				if (map.mapData[i][j] == key && p1x == j && p1y == i) {
					gameState.player.invincible = true;
					map.mapData[i][j] = 109;
					keyGone = true;
					time2 = 0.0f;
				}
				if (map.mapData[i][j] == key && p2x == j && p2y == i) {
					gameState.player2.invincible = true;
					map.mapData[i][j] = 109;
					keyGone = true;
					time2 = 0.0f;
				}

			}
		}

		time2 += elapsed;
		if (time2 > 5.0f) {
			map.mapData[7][7] = 307;
			gameState.player.invincible = false;
			gameState.player.invincible = false;
			keyGone = false;
			time2 = 0.0f;
		}


		if (p1x == p2x && p1y == p2y) {
			if (gameState.player.invincible) {
				mode = STATE_GAME_OVER2;
				gameState.success = true;
			}

			if (gameState.player2.invincible) {
				mode = STATE_GAME_OVER2;
				gameState.success = false;
			}
		}
		/*for (int i = 0;i < 2;i++) {
			worldToTileCoordinates(gameState.enemies[i].position.x, gameState.enemies[i].position.y, &x, &y);
			flag = false;
			for (int j : obstacles) {
				if (map.mapData[y][x] == j) {
						flag = true;
						break;
					}
				}

			if (flag)
				gameState.enemies[i].velocity.y *= -1;

			if (px == x && py == y) {
				if (invincible) {
					int index = 447;
					killed = true;
					SheetSprite ghostSprite(ghostSheet, (float)(((int)index) % 30) / (float)30, (float)(((int)index) / 30) / (float)30, 1.0 / (float)30, 1.0 / (float)30, 1.0f);
					gameState.enemies[1].sprite = ghostSprite;
					gameState.enemies[1].velocity.y = 0.0f;
				}
				else {
					mode = STATE_GAME_OVER;
					gameState.success = false;
				}
			}
		}*/
	}
} gameLevel3;

bool play = true;
class GameOver {
public:
	void Render() {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		glm::mat4 viewMatrix = glm::mat4(1.0f);
		program1.SetViewMatrix(viewMatrix);
		glClear(GL_COLOR_BUFFER_BIT);

		if (gameState.success) {
			modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.45f, 0.0f, 0.0f));
			modelMatrix = glm::scale(modelMatrix, glm::vec3(0.5f, 0.5f, 0.0f));
			program1.SetModelMatrix(modelMatrix);
			DrawText(program1, 1, "YOU WON!", 0.5f, -0.25f);
		}

		else {
			modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.55f, 0.0f, 0.0f));
			modelMatrix = glm::scale(modelMatrix, glm::vec3(0.5f, 0.5f, 0.0f));
			program1.SetModelMatrix(modelMatrix);
			DrawText(program1, 1, "YOU LOST!   ", 0.5f, -0.25f);
			if (play) {
				Mix_Chunk *someSound;
				someSound = Mix_LoadWAV("jingles.ogg");
				Mix_PlayChannel(-1, someSound, 0);
				play = false;
			}
		}
	}

	void ProcessUpdate(float elapsed, bool &done, SDL_Event &event) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}

			else if (event.type == SDL_KEYDOWN) {
				if ((event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)) {
					done = true;
				}
			}
		}
	}
} gameOver;

bool play2 = true;
class GameOver2 {
public:
	void Render() {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		glm::mat4 viewMatrix = glm::mat4(1.0f);
		program1.SetViewMatrix(viewMatrix);
		glClear(GL_COLOR_BUFFER_BIT);

		if (gameState.success) {
			modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.45f, 0.0f, 0.0f));
			modelMatrix = glm::scale(modelMatrix, glm::vec3(0.5f, 0.5f, 0.0f));
			program1.SetModelMatrix(modelMatrix);
			DrawText(program1, 1, "MICHAEL WON!", 0.5f, -0.25f);
			
			if (play) {
				Mix_Chunk *someSound;
				someSound = Mix_LoadWAV("jingles2.ogg");
				Mix_PlayChannel(-1, someSound, 0);
				play = false;
			}
		}

		else {
			modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.55f, 0.0f, 0.0f));
			modelMatrix = glm::scale(modelMatrix, glm::vec3(0.5f, 0.5f, 0.0f));
			program1.SetModelMatrix(modelMatrix);
			DrawText(program1, 1, "BETTY WON!   ", 0.5f, -0.25f);
			if (play) {
				Mix_Chunk *someSound;
				someSound = Mix_LoadWAV("jingles2.ogg");
				Mix_PlayChannel(-1, someSound, 0);
				play = false;
			}
		}
	}

	void ProcessUpdate(float elapsed, bool &done, SDL_Event &event) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}

			else if (event.type == SDL_KEYDOWN) {
				if ((event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)) {
					done = true;
				}
			}
		}
	}
} gameOver2;

void Setup() {
	int resX = 1081;
	int resY = 713;
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Legend of Michael", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, resX, resY, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
	glewInit();
#endif

	glViewport(0, 0, resX, resY);

	program1.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	float aspectRatio = (float)resX / (float)resY;
	float projectionHeight = 1.0f;
	float projectionWidth = projectionHeight * aspectRatio;
	float projectionDepth = 1.0f;
	projectionMatrix = glm::ortho(-projectionWidth, projectionWidth, -projectionHeight, projectionHeight, -projectionDepth, projectionDepth);

	sheet = LoadTexture(RESOURCE_FOLDER"tilemap.png");
	playerSheet = LoadTexture(RESOURCE_FOLDER"george.png");
	player2Sheet = LoadTexture(RESOURCE_FOLDER"betty.png");
	mode = STATE_MAIN_MENU;

	int index = 446;
	GLuint ghostSheet = LoadTexture(RESOURCE_FOLDER"spritesheet_rgba.png");

	map.Load("map2.txt");

	float placeX = map.entities[1].x * TILE_SIZE + TILE_SIZE / 2;
	float placeY = map.entities[1].y * -TILE_SIZE + TILE_SIZE / 2;
	SheetSprite ghostSprite(ghostSheet, (float)(((int)index) % 30) / (float)30, (float)(((int)index) / 30) / (float)30, 1.0 / (float)30, 1.0 / (float)30, 1.0f);
	gameState.enemies[0] = Entity(glm::vec3(placeX - TILE_SIZE, placeY - TILE_SIZE, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(TILE_SIZE, TILE_SIZE, 0.0f), glm::vec3(2.0f, -4.0f, 0.0f), ghostSprite);

	placeX = map.entities[0].x * TILE_SIZE + TILE_SIZE / 2;
	placeY = map.entities[0].y * -TILE_SIZE + TILE_SIZE / 2;


	index = 2;
	SheetSprite playerSprite = SheetSprite(playerSheet, (float)(((int)index) % 4) / (float)4, (float)(((int)index) / 4) / (float)4, 1.0 / (float)4, 1.0 / (float)4, 1.0f);
	gameState.player = Entity(glm::vec3(placeX, placeY, 0.0f), glm::vec3(0.0f), glm::vec3(TILE_SIZE, TILE_SIZE, 0.0f), glm::vec3(2.0f, -4.0f, 0.0f), playerSprite);

	SheetSprite player2Sprite = SheetSprite(player2Sheet, (float)(((int)index) % 4) / (float)4, (float)(((int)index) / 4) / (float)4, 1.0 / (float)4, 1.0 / (float)4, 1.0f);
	gameState.player2 = Entity(glm::vec3(placeX-TILE_SIZE, placeY-TILE_SIZE, 0.0f), glm::vec3(0.0f), glm::vec3(TILE_SIZE, TILE_SIZE, 0.0f), glm::vec3(2.0f, -4.0f, 0.0f), player2Sprite);

	for (int index = 0; index < 16;index++) {
		animation[index] = SheetSprite(playerSheet, (float)(((int)index) % 4) / (float)4, (float)(((int)index) / 4) / (float)4, 1.0 / (float)4, 1.0 / (float)4, 1.0f);
		animation2[index] = SheetSprite(player2Sheet, (float)(((int)index) % 4) / (float)4, (float)(((int)index) / 4) / (float)4, 1.0 / (float)4, 1.0 / (float)4, 1.0f);
	}

	glUseProgram(program1.programID);

	program1.SetModelMatrix(modelMatrix);
	program1.SetProjectionMatrix(projectionMatrix);
	program1.SetViewMatrix(viewMatrix);

	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);

	Mix_Music *music;
	music = Mix_LoadMUS("session.mp3");
	Mix_PlayMusic(music, -1);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

}

void Render() {
	switch (mode) {
	case STATE_MAIN_MENU:
		mainMenu.Render();
		break;

	case STATE_GAME_LEVEL:
		gameLevel.Render();
		break;

	case STATE_GAME_LEVEL2:
		gameLevel2.Render();
		break;

	case STATE_GAME_LEVEL3:
		gameLevel3.Render();
		break;

	case STATE_GAME_OVER:
		gameOver.Render();
		break;

	case STATE_GAME_OVER2:
		gameOver2.Render();
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

	case STATE_GAME_LEVEL2:
		gameLevel2.ProcessUpdate(elapsed, done, event);
		break;

	case STATE_GAME_LEVEL3:
		gameLevel3.ProcessUpdate(elapsed, done, event);
		break;

	case STATE_GAME_OVER:
		gameOver.ProcessUpdate(elapsed, done, event);
		break;

	case STATE_GAME_OVER2:
		gameOver2.ProcessUpdate(elapsed, done, event);
		break;
	}
}

void Update(float elapsed) {
	if (mode == STATE_GAME_LEVEL)
		gameLevel.Update(elapsed);

	if (mode == STATE_GAME_LEVEL2)
		gameLevel2.Update(elapsed);

	if (mode == STATE_GAME_LEVEL3)
		gameLevel3.Update(elapsed);
}

int main(int argc, char *argv[])
{
	Setup();

	float lastFrameTicks = 0.0f;
	float accumulator = 0.0f;

	SDL_Event event;
	bool done = false;
	while (!done) {
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		elapsed += accumulator;
		if (elapsed < FIXED_TIMESTEP) {
			accumulator = elapsed;
			continue;
		}
		while (elapsed >= FIXED_TIMESTEP) {
			Update(FIXED_TIMESTEP);
			elapsed -= FIXED_TIMESTEP;
		}
		accumulator = elapsed;

		//Update(elapsed);

		ProcessUpdate(elapsed, done, event);
		
		Render();
	}

	glDisable(GL_BLEND);
	SDL_Quit();
	return 0;
}