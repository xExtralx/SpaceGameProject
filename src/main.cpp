#include "game/game.h"

int main(void) {
    Game game;
    game.init();

    while (!game.shouldClose()) {
        game.update();
        game.render();
    }

    game.stop();

    return 0;
}
