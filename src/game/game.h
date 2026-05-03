//
// Created by raphael on 02/11/2025.
//

#ifndef GAME_H
#define GAME_H

#include "../renderer/renderer.h"
#include "world/worldgen.h"
#include "ECS/ecs.h"

class Game {
public:

    Game();

    void init();
    void update();
    void render();

    bool shouldClose() const;
    void stop();

    void moveEvent(int x,int y);

    void clickEvent(int x,int y);
    void keyPressEvent(int key);
    void keyReleaseEvent(int key);
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

    Renderer& getRenderer() { return this->renderer; }

    float deltaTime;
    float lastFrame;
private:
    int width = 0;
    int height = 0;

    Renderer renderer;
    FileManager fileManager;

    ChunkManager chunkManager; // default seed

    bool keys[1024] = {};  // tracks which keys are held down

    ECSWorld world;
    float    dt = 0.0f;
};


#endif //GAME_H