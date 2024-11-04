#pragma once

#include <stdint.h>
#include "diagnostics.h"

struct Game
{
    void ProcessTicks(uint64_t numberOfTicks);
};

void Game::ProcessTicks(uint64_t numberOfTicks)
{
    if (!numberOfTicks)
    {
        return;
    }
    // LOG("TODO Game::ProcessTicks %llu\n", numberOfTicks);
}
