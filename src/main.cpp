#include "game/game.h"

int main(int argc, char* argv[]) {
    
    FileManager::SetBasePath(
        std::filesystem::canonical(argv[0]).parent_path().string()
    );
    
    Game game;
    game.init();

    while (!game.shouldClose()) {
        game.update();
        game.render();
    }

    game.stop();

    return 0;
}
