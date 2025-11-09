//
// Created by raphael on 02/11/2025.
//

#ifndef GAME_H
#define GAME_H

#include "../renderer/renderer.h"

class Game {
public:

    Game();

    void init();
    void update();
    void render();

    bool shouldClose() const;
    void stop();

    static void moveEvent(int x,int y);

    static void clickEvent(int x,int y);

    Renderer& getRenderer() { return this->renderer; }

    float deltaTime;
    float lastFrame;
private:
    int width = 0;
    int height = 0;

    Renderer renderer;
};


#endif //GAME_H