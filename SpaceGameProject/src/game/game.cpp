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

    fileManager.init();

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
    renderer.addTriangle({100.0f,100.0f}, {200.0f,100.0f}, {150.0f,150.0f},{1.0f,1.0f,0.05f,1.0f},1.0f);
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

void Game::keyPressEvent(int key) {
    std::cout << "Key Pressed : " << key << std::endl;

    if (key == GLFW_KEY_F11) {
        renderer.fullscreen = !renderer.fullscreen;
        if (renderer.fullscreen) {
            glfwSetWindowMonitor(renderer.getWindow(), glfwGetPrimaryMonitor(), 0, 0, 1920, 1080, GLFW_DONT_CARE);
        } else {
            glfwSetWindowMonitor(renderer.getWindow(), NULL, 0, 0, 800, 600, GLFW_DONT_CARE);
        }
    }
}

void Game::keyReleaseEvent(int key) {
    std::cout << "Key Released : " << key << std::endl;
}

void Game::stop() {
    std::cout << "Game Stopped" << std::endl;
}




