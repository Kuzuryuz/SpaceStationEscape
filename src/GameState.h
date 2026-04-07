#pragma once

struct GameState
{
    int currentRoom = -1;

    bool oxygenFixed = false;
    bool powerFixed = false;

    bool storageUnlocked = false;
    bool labUnlocked = false;

    bool foundNote = false;
    bool hasCode = false;

    bool controlUnlocked = false;
    bool gameFinished = false;
};