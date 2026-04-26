#pragma once

#include <array>

struct GameState
{
    static constexpr int kOxygenValveCount = 3;

    int currentRoom = -1;

    bool oxygenFixed = false;
    bool powerFixed = false;

    bool storageUnlocked = false;
    bool labUnlocked = false;

    bool foundNote = false;
    bool hasCode = false;

    bool controlUnlocked = false;
    bool gameFinished = false;
    bool playerDied = false;
    bool oxygenPuzzleFailed = false;

    int oxygenValveProgress = 0;
    std::array<bool, kOxygenValveCount> oxygenValvesOpened{ false, false, false };
};
