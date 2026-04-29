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
    glfwSetScrollCallback(renderer.getWindow(), Game::scroll_callback);
}

void Game::update() {
    deltaTime = (float)glfwGetTime() - lastFrame;
    lastFrame = (float)glfwGetTime();

    // Camera speed in world units per second
    const float speed = 100.0f;

    if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP])
        renderer.camera.position[1] += speed * deltaTime;
    if (keys[GLFW_KEY_A] || keys[GLFW_KEY_DOWN])
        renderer.camera.position[1] -= speed * deltaTime;
    if (keys[GLFW_KEY_S] || keys[GLFW_KEY_LEFT])
        renderer.camera.position[0] -= speed * deltaTime;
    if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT])
        renderer.camera.position[0] += speed * deltaTime;
}

void Game::render() {
    renderer.clear();
    // In Game::render()
    renderer.addTriangle(
        Vec2(-0.5f,  0.5f),
        Vec2(-0.5f, -0.5f),
        Vec2( 0.5f, -0.5f),
        Vec4(1.0f, 0.0f, 0.0f, 1.0f),
        0.0f
    );

    renderer.draw();
    // centered on screen, scale 1.0 = native pixel size relative to 320x180
    renderer.drawImage("debug/debug.png", Vec2(0.0f, 0.0f), 1.0f);

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
    std::cerr << "[key] code: " << key << " name: " << (glfwGetKeyName(key, 0) ? glfwGetKeyName(key, 0) : "unknown") << std::endl;
    
    if (key >= 0 && key < 1024)
        keys[key] = true;

    if (key == GLFW_KEY_F11) {
        renderer.fullscreen = !renderer.fullscreen;
        if (renderer.fullscreen)
            glfwSetWindowMonitor(renderer.getWindow(), glfwGetPrimaryMonitor(), 0, 0, 1920, 1080, GLFW_DONT_CARE);
        else
            glfwSetWindowMonitor(renderer.getWindow(), NULL, 100, 100, 1280, 720, GLFW_DONT_CARE);
    }
}

void Game::keyReleaseEvent(int key) {
    if (key >= 0 && key < 1024)
        keys[key] = false;
}

// game.cpp
void Game::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    const auto game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (!game) return;

    game->renderer.camera.zoom += (float)yoffset * 0.1f;
    game->renderer.camera.zoom  = std::max(0.1f, std::min(game->renderer.camera.zoom, 10.0f)); // clamp 0.1 - 10
}

void Game::stop() {
    std::cout << "Game Stopped" << std::endl;
}




