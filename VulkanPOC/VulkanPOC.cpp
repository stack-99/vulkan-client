﻿#include "engine_lib.h"

#include "GameCore.hpp"

int main() {
   GameCore game;

    try {
        game.Initialize();

        game.Run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }


    return EXIT_SUCCESS;
}