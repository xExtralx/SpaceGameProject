//
// Created by raphael on 02/11/2025.
//

#include "game.h"

Game::Game():
    width(1280),
    height(720),
    renderer(width,height)
{
}

void Game::init() {
    if (!renderer.init()) {
        std::cout << "failed to Initialize Renderer" << std::endl;
        return;
    }

    std::cout << "Game Initialized" << std::endl;

    glfwSetWindowUserPointer(renderer.getWindow(), this);
    glfwSetMouseButtonCallback(renderer.getWindow(), Renderer::mouse_button_callback);
}

void Game::update() {
    deltaTime = (float)glfwGetTime() - lastFrame;
    lastFrame = glfwGetTime();
}

void Game::render() {
    renderer.clear();
    renderer.addTriangle({0.0f,0.0f}, {.0f,0.0f}, {0.0f,1.0f},{0.0f,1.0f,0.05f,1.0f},1.0f);
    renderer.uploadGeometry();
    renderer.draw();
    renderer.present();
    renderer.update();
}

bool Game::shouldClose() const {
    return renderer.shouldClose();
}

void Game::moveEvent(int x, int y) {
    // Logique de déplacement
}

void Game::clickEvent(int x, int y) {
    std::cout << "Click at : (" << x << ", " << y << ")" << std::endl;
}

void Game::stop() {
    std::cout << "Game Stopped" << std::endl;
}




