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

#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
using namespace std;

#define NO_OF_ENEMIES 16

#define FIXED_TIMESTEP 0.166666f
#define MAX_TIMESTEPS 6

#define TILE_SIZE 0.1f

#define SPRITE_COUNT_X 30
#define SPRITE_COUNT_Y 30

float gravity = -4.0f;
int **levelData;
bool **isSolid;
int mapWidth, mapHeight;
GLuint sheet;

SDL_Window* displayWindow;

ShaderProgram program1;
glm::mat4 projectionMatrix = glm::mat4(1.0f);
glm::mat4 viewMatrix = glm::mat4(1.0f);
glm::mat4 modelMatrix = glm::mat4(1.0f);
float time = 0.0f;

enum GameMode { STATE_MAIN_MENU, STATE_GAME_LEVEL, STATE_GAME_OVER } mode;

class SheetSprite {
public:
	SheetSprite() {};
	SheetSprite(unsigned int textureID, float u, float v, float width, float height, float size = 1) {
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
		velocity.y += gravity * elapsed;
		position += velocity * elapsed;
	}

	glm::vec3 position;
	glm::vec3 velocity;
	glm::vec3 acceleration;
	glm::vec3 size;
	SheetSprite sprite;
	float health;
	bool isStatic;

	bool collidedTop;
	bool collidedBottom;
	bool collidedLeft;
	bool collidedRight;
	char* name;
} player, enemy;

GLuint LoadTexture(const char *filePath) {
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);

	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
		assert(false);
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

//unsigned int textureID, float u, float v, float width, float height, float size 0.2f
void placeEntity(string type, float placeX, float placeY) {
		SheetSprite playerSprite = SheetSprite(sheet, (float)(19 % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X, (float)(0 % SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y, 1.0f / (float)SPRITE_COUNT_X, 1.0f / (float)SPRITE_COUNT_Y, 1.0f);
		player = Entity(glm::vec3(placeX, placeY, 0.0f), glm::vec3(0.5f, 0.0f, 0.0f), glm::vec3(TILE_SIZE, TILE_SIZE, 0.0f), playerSprite);
		player.name = "player";
		SheetSprite enemySprite = SheetSprite(sheet, (float)(19 % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X, (float)(2 % SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y, 1.0f / (float)SPRITE_COUNT_X, 1.0f / (float)SPRITE_COUNT_Y, 1.0f);
		enemy = Entity(glm::vec3(1.0f, -0.34f, 0.0f), glm::vec3(0.5f, 0.0f, 0.0f), glm::vec3(TILE_SIZE, TILE_SIZE, 0.0f), enemySprite);
}

bool readEntityData(std::ifstream &stream) {
	string line;
	string type;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "type") {
			type = value;
		}
		else if (key == "location") {
			istringstream lineStream(value);
			string xPosition, yPosition;
			getline(lineStream, xPosition, ',');
			getline(lineStream, yPosition, ',');

			//float placeX = atoi(xPosition.c_str())*TILE_SIZE;
			//float placeY = atoi(yPosition.c_str())*-TILE_SIZE + 0.5f;

			float placeX = -14.0f * TILE_SIZE;
			float placeY = -0.34f;

			placeEntity(type, placeX, placeY);
		}
	}
	return true;
}

bool readLayerData(std::ifstream &stream) {
	string line;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "data") {
			for (int y = 0; y < mapHeight; y++) {
				getline(stream, line);
				istringstream lineStream(line);
				string tile;
				for (int x = 0; x < mapWidth; x++) {
					getline(lineStream, tile, ',');
					int val = (int)atoi(tile.c_str());
					if (val > 0) {
						// be careful, the tiles in this format are indexed from 1 not 0
						levelData[y][x] = val - 1;
						if (val == 123 || val == 151 || val == 153 || val == 497 || val == 527 || val == 525)
							isSolid[y][x] = true;
					}
					else {
						levelData[y][x] = 199;
					}
				}
			}
		}
	}
	return true;
}

bool readHeader(std::ifstream &stream) {
	string line;
	mapWidth = -1;
	mapHeight = -1;
	while (getline(stream, line)) {
		if (line == "") { break; }
		istringstream sStream(line);
		string key, value;
		getline(sStream, key, '=');
		getline(sStream, value);
		if (key == "width") {
			mapWidth = atoi(value.c_str());
		}
		else if (key == "height") {
			mapHeight = atoi(value.c_str());
		}
	}
	if (mapWidth == -1 || mapHeight == -1) {
		return false;
	}
	else { // allocate our map data
		levelData = new int*[mapHeight];
		isSolid = new bool*[mapHeight];
		for (int i = 0; i < mapHeight; ++i) {
			levelData[i] = new int[mapWidth];
			isSolid[i] = new bool[mapHeight];
		}
		return true;
	}
}

void worldToTileCoordinates(float worldX, float worldY, int *gridX, int *gridY) {
	*gridX = (int)(worldX / TILE_SIZE);
	*gridY = (int)(worldY / -TILE_SIZE);
}

void collisionCheck() {
	int x, y;
	worldToTileCoordinates(player.position.x, player.position.y, &x, &y);
	if (isSolid[y][x] == true) {}
		//player.collidedBottom = true;
}

void Setup() {
	int resX = 1081;
	int resY = 713;
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Ori 500", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, resX, resY, SDL_WINDOW_OPENGL);
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
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.5f, 1.3f, 0.0f));
	sheet = LoadTexture(RESOURCE_FOLDER"spritesheet_rgba.png");
	mode = STATE_GAME_LEVEL;
	ifstream infile("map.txt");
	string line;
	while (getline(infile, line)) {
		if (line == "[header]") {
			if (!readHeader(infile)) {
				return;
			}
		}
		else if (line == "[layer]") {
			readLayerData(infile);
		}
		else if (line == "[entities]") {
			readEntityData(infile);
		}
	}

	//viewMatrix = glm::translate(viewMatrix, glm::vec3(-player.position.x, -player.position.y, 0.0f));
	program1.SetProjectionMatrix(projectionMatrix);
	program1.SetViewMatrix(viewMatrix);
	program1.SetModelMatrix(modelMatrix);

	glUseProgram(program1.programID);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Render() {
	if (mode == STATE_GAME_OVER) {
			glm::mat4 modelMatrix = glm::mat4(1.0f);
			modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.3f, 0.0f, 0.0f));
			modelMatrix = glm::scale(modelMatrix, glm::vec3(0.5f, 0.5f, 0.0f));
			program1.SetModelMatrix(modelMatrix);
			glClear(GL_COLOR_BUFFER_BIT);
			DrawText(program1, 1, "YOU STEPPED ON A LAND MINE", 0.45f, -0.25f);
	}
	else {
		std::vector<float> vertexData;
		std::vector<float> texCoordData;
		for (int y = 0; y < mapHeight; y++) {
			for (int x = 0; x < mapWidth; x++) {
				float u = (float)(((int)levelData[y][x]) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
				float v = (float)(((int)levelData[y][x]) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;
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
		float* vertices = vertexData.data();
		float* texCoords = texCoordData.data();
		//glm::mat4 modelMatrix = glm::mat4(1.0f);
		//modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.5f, 1.3f, 0.0f));

		//viewMatrix = glm::translate(viewMatrix, glm::vec3(-player.position.x, -player.position.y, 0.0f));

		//program1.SetViewMatrix(viewMatrix);
		//program1.SetModelMatrix(modelMatrix);

		glClear(GL_COLOR_BUFFER_BIT);
		glVertexAttribPointer(program1.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program1.positionAttribute);

		glVertexAttribPointer(program1.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program1.texCoordAttribute);

		glBindTexture(GL_TEXTURE_2D, sheet);
		glDrawArrays(GL_TRIANGLES, 0, vertexData.size() / 2);

		glDisableVertexAttribArray(program1.positionAttribute);
		glDisableVertexAttribArray(program1.texCoordAttribute);

		player.Draw(program1);
	}

	SDL_GL_SwapWindow(displayWindow);
}

void ProcessUpdate(float elapsed, bool &done, SDL_Event &event) {
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
			done = true;
		}

		/*else if (event.type == SDL_KEYDOWN) {
			if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
				player.velocity.y = 2.0f;
			}
		}
	}
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	if (keys[SDL_SCANCODE_LEFT]) {
		player.velocity.x = -1.0f;
	}
	if (keys[SDL_SCANCODE_RIGHT]) {
		player.velocity.x = 1.0f;*/
	}
	}

	void Update(float elapsed) {
		player.collidedBottom = false;
		player.collidedRight = false;
		player.collidedLeft = false;
		player.collidedTop = false;
		collisionCheck();
		if (player.position.y - player.size.y / 2 < -0.34f)
			player.position.y = -0.34f;

		//if (player.position.x + player.size.x - 2.3f <= 0.05f && player.position.y - player.size.y / 2 + 0.34f <= 0.1f)
			//mode = STATE_GAME_OVER;
	
		if (player.collidedBottom) {
			player.position.y += player.size.y / 2;
		}
		player.Update(elapsed);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(-player.velocity.x*0.05, 0.0f, 0.0f));
		program1.SetModelMatrix(modelMatrix);
		player.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
	}

	int main(int argc, char *argv[])
	{
		Setup();

		float lastFrameTicks = 0.0f;
		float accumulator = 0.0f;

		int temp = 5;
		bool done = false;
		SDL_Event e;
		while (!done) {
			float ticks = (float)SDL_GetTicks() / 1000.0f;
			float elapsed = ticks - lastFrameTicks;
			lastFrameTicks = ticks;
			//ProcessUpdate(elapsed, done, event);
			while (SDL_PollEvent(&e)) {
				if (e.type == SDL_QUIT || e.type == SDL_WINDOWEVENT_CLOSE) {
					done = true;
				}
			}

			const Uint8 *keys = SDL_GetKeyboardState(NULL);
			if (keys[SDL_SCANCODE_LEFT]) {
				player.velocity.x = -0.5f;
			}
			if (keys[SDL_SCANCODE_RIGHT]) {
				player.velocity.x = 0.5f;
			}

			if (keys[SDL_SCANCODE_SPACE]) {
				player.velocity.y = 3.0f;
			}

			Update(elapsed);

			/*elapsed += accumulator;
			if (elapsed < FIXED_TIMESTEP) {
				accumulator = elapsed;
				continue;
			}
			while (elapsed >= FIXED_TIMESTEP) {
				Update(FIXED_TIMESTEP);
				elapsed -= FIXED_TIMESTEP;
			}
			accumulator = elapsed;*/

			Render();
		}

		glDisable(GL_BLEND);
		SDL_Quit();
		return 0;
	}