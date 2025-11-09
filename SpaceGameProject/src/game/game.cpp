//
// Created by raphael on 02/11/2025.
//

#include "game.h"

Game::Game():
    width(1440),
    height(1080),
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
    renderer.addTriangle({0.0f,0.0f,0.0f},{100.0f,0.0f,0.0f},{0.0f,100.0f,0.0f},{1.0f,0.0f,0.0f,1.0f});
    renderer.uploadGeometry();
    renderer.draw();
    renderer.present();
}

bool Game::shouldClose() const {
    return renderer.shouldClose();
}

void Game::moveEvent(int x, int y) {
    std::cout << "Mouse at : (" << x << ", " << y << ")" << std::endl;
}

void Game::clickEvent(int x, int y) {
    std::cout << "Click at : (" << x << ", " << y << ")" << std::endl;
}

void Game::stop() {
    std::cout << "Game Stopped" << std::endl;
}




