#include "World.h"

#include <glm/common.hpp>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

namespace
{
    struct WallMetrics
    {
        float floorY = -0.55f;
        float floorThickness = 0.18f;
        float wallY = 0.95f;
        float wallHeight = 2.9f;
        float wallThickness = 0.26f;
        float wallModuleLength = 2.4f;
        float cornerFootprint = 0.6f;
    };

    constexpr WallMetrics kWallMetrics{};

    glm::vec3 kWallColor(0.48f, 0.52f, 0.60f);
    glm::vec3 kCorridorColor(0.18f, 0.20f, 0.24f);
    glm::vec3 kRoomFloorColor(0.13f, 0.15f, 0.19f);
}

void World::setGameState(GameState* state)
{
    gameState = state;
}

void World::addObject(const std::string& id, glm::vec3 pos, glm::vec3 scale, glm::vec3 color, bool hasCollision, float rotationY)
{
    staticObjects.push_back({ id, pos, scale, color, rotationY, hasCollision });
}

void World::addRoomFloor(const std::string& id, float minX, float maxX, float minZ, float maxZ, const glm::vec3& color)
{
    addObject(
        id,
        glm::vec3((minX + maxX) * 0.5f, kWallMetrics.floorY, (minZ + maxZ) * 0.5f),
        glm::vec3(maxX - minX, kWallMetrics.floorThickness, maxZ - minZ),
        color,
        false
    );
}

void World::addCornerPiece(const std::string& id, float x, float z, const glm::vec3& color)
{
    addObject(
        id,
        glm::vec3(x, kWallMetrics.wallY, z),
        glm::vec3(kWallMetrics.cornerFootprint, kWallMetrics.wallHeight, kWallMetrics.cornerFootprint),
        color,
        true
    );
}

void World::addHorizontalWallRun(const std::string& idPrefix, float startX, float endX, float z, const glm::vec3& color)
{
    float clampedStart = startX + kWallMetrics.cornerFootprint * 0.5f;
    float clampedEnd = endX - kWallMetrics.cornerFootprint * 0.5f;

    if (clampedEnd <= clampedStart)
        return;

    float cursor = clampedStart;
    int partIndex = 0;

    while (cursor < clampedEnd - 0.001f)
    {
        float remaining = clampedEnd - cursor;
        float segmentLength = remaining > kWallMetrics.wallModuleLength
            ? kWallMetrics.wallModuleLength
            : remaining;

        float centerX = cursor + segmentLength * 0.5f;

        addObject(
            idPrefix + "_" + std::to_string(partIndex),
            glm::vec3(centerX, kWallMetrics.wallY, z),
            glm::vec3(segmentLength, kWallMetrics.wallHeight, kWallMetrics.wallThickness),
            color,
            true
        );

        cursor += segmentLength;
        ++partIndex;
    }
}

void World::addVerticalWallRun(const std::string& idPrefix, float x, float startZ, float endZ, const glm::vec3& color)
{
    float clampedStart = startZ + kWallMetrics.cornerFootprint * 0.5f;
    float clampedEnd = endZ - kWallMetrics.cornerFootprint * 0.5f;

    if (clampedEnd <= clampedStart)
        return;

    float cursor = clampedStart;
    int partIndex = 0;

    while (cursor < clampedEnd - 0.001f)
    {
        float remaining = clampedEnd - cursor;
        float segmentLength = remaining > kWallMetrics.wallModuleLength
            ? kWallMetrics.wallModuleLength
            : remaining;

        float centerZ = cursor + segmentLength * 0.5f;

        addObject(
            idPrefix + "_" + std::to_string(partIndex),
            glm::vec3(x, kWallMetrics.wallY, centerZ),
            glm::vec3(kWallMetrics.wallThickness, kWallMetrics.wallHeight, segmentLength),
            color,
            true
        );

        cursor += segmentLength;
        ++partIndex;
    }
}

void World::addDoor(
    const std::string& name,
    const glm::vec3& center,
    const glm::vec3& halfSize,
    float rotationY,
    bool open,
    const glm::vec3& closedColor,
    const glm::vec3& openColor)
{
    doors.push_back({ name, center, halfSize, closedColor, openColor, rotationY, open });
}

Door* World::findDoor(const std::string& name)
{
    for (auto& door : doors)
    {
        if (door.name == name)
            return &door;
    }

    return nullptr;
}

void World::buildDefaultRoom()
{
    staticObjects.clear();
    colliders.clear();
    rooms.clear();
    interactables.clear();
    doors.clear();

    const float bedroomMinX = -4.0f;
    const float bedroomMaxX = 4.0f;
    const float bedroomMinZ = -4.0f;
    const float bedroomMaxZ = 4.0f;

    const float storageMinX = -4.0f;
    const float storageMaxX = 4.0f;
    const float storageMinZ = 4.0f;
    const float storageMaxZ = 12.0f;

    const float controlMinX = -12.0f;
    const float controlMaxX = -4.0f;
    const float controlMinZ = -4.0f;
    const float controlMaxZ = 4.0f;

    const float powerMinX = 4.0f;
    const float powerMaxX = 12.0f;
    const float powerMinZ = -4.0f;
    const float powerMaxZ = 4.0f;

    const float oxygenMinX = -4.0f;
    const float oxygenMaxX = 4.0f;
    const float oxygenMinZ = -12.0f;
    const float oxygenMaxZ = -4.0f;

    const float labMinX = -4.0f;
    const float labMaxX = 4.0f;
    const float labMinZ = -20.0f;
    const float labMaxZ = -12.0f;

    addRoomFloor("floor_bedroom", bedroomMinX, bedroomMaxX, bedroomMinZ, bedroomMaxZ, kRoomFloorColor);
    addRoomFloor("floor_control", controlMinX, controlMaxX, controlMinZ, controlMaxZ, glm::vec3(0.18f, 0.16f, 0.24f));
    addRoomFloor("floor_storage", storageMinX, storageMaxX, storageMinZ, storageMaxZ, kRoomFloorColor);
    addRoomFloor("floor_power", powerMinX, powerMaxX, powerMinZ, powerMaxZ, kRoomFloorColor);
    addRoomFloor("floor_oxygen", oxygenMinX, oxygenMaxX, oxygenMinZ, oxygenMaxZ, kRoomFloorColor);
    addRoomFloor("floor_lab", labMinX, labMaxX, labMinZ, labMaxZ, kRoomFloorColor);

    addHorizontalWallRun("wall_control_top", controlMinX, controlMaxX, controlMinZ, kWallColor);
    addHorizontalWallRun("wall_control_bottom", controlMinX, controlMaxX, controlMaxZ, kWallColor);
    addVerticalWallRun("wall_control_left", controlMinX, controlMinZ, controlMaxZ, kWallColor);
    addVerticalWallRun("wall_bedroom_control_left", controlMaxX, controlMinZ, -1.1f, kWallColor);
    addVerticalWallRun("wall_bedroom_control_right", controlMaxX, 1.1f, controlMaxZ, kWallColor);

    addHorizontalWallRun("wall_storage_bottom", storageMinX, storageMaxX, storageMaxZ, kWallColor);
    addVerticalWallRun("wall_storage_left", storageMinX, storageMinZ, storageMaxZ, kWallColor);
    addVerticalWallRun("wall_storage_right", storageMaxX, storageMinZ, storageMaxZ, kWallColor);
    addHorizontalWallRun("wall_bedroom_storage_left", bedroomMinX, -1.1f, bedroomMaxZ, kWallColor);
    addHorizontalWallRun("wall_bedroom_storage_right", 1.1f, bedroomMaxX, bedroomMaxZ, kWallColor);

    addHorizontalWallRun("wall_power_top", powerMinX, powerMaxX, powerMinZ, kWallColor);
    addHorizontalWallRun("wall_power_bottom", powerMinX, powerMaxX, powerMaxZ, kWallColor);
    addVerticalWallRun("wall_power_right", powerMaxX, powerMinZ, powerMaxZ, kWallColor);
    addVerticalWallRun("wall_bedroom_power_left", bedroomMaxX, bedroomMinZ, -1.1f, kWallColor);
    addVerticalWallRun("wall_bedroom_power_right", bedroomMaxX, 1.1f, bedroomMaxZ, kWallColor);

    addVerticalWallRun("wall_oxygen_left", oxygenMinX, oxygenMinZ, oxygenMaxZ, kWallColor);
    addVerticalWallRun("wall_oxygen_right", oxygenMaxX, oxygenMinZ, oxygenMaxZ, kWallColor);
    addHorizontalWallRun("wall_bedroom_oxygen_left", bedroomMinX, -1.1f, bedroomMinZ, kWallColor);
    addHorizontalWallRun("wall_bedroom_oxygen_right", 1.1f, bedroomMaxX, bedroomMinZ, kWallColor);
    addHorizontalWallRun("wall_oxygen_lab_left", oxygenMinX, -1.1f, oxygenMinZ, kWallColor);
    addHorizontalWallRun("wall_oxygen_lab_right", 1.1f, oxygenMaxX, oxygenMinZ, kWallColor);

    addHorizontalWallRun("wall_lab_top", labMinX, labMaxX, labMinZ, kWallColor);
    addVerticalWallRun("wall_lab_left", labMinX, labMinZ, labMaxZ, kWallColor);
    addVerticalWallRun("wall_lab_right", labMaxX, labMinZ, labMaxZ, kWallColor);

    addCornerPiece("corner_bedroom_nw", bedroomMinX, bedroomMinZ, kWallColor);
    addCornerPiece("corner_bedroom_sw", bedroomMinX, bedroomMaxZ, kWallColor);
    addCornerPiece("corner_bedroom_ne", bedroomMaxX, bedroomMinZ, kWallColor);
    addCornerPiece("corner_bedroom_se", bedroomMaxX, bedroomMaxZ, kWallColor);
    addCornerPiece("corner_control_nw", controlMinX, controlMinZ, kWallColor);
    addCornerPiece("corner_control_sw", controlMinX, controlMaxZ, kWallColor);
    addCornerPiece("corner_storage_sw", storageMinX, storageMaxZ, kWallColor);
    addCornerPiece("corner_storage_se", storageMaxX, storageMaxZ, kWallColor);
    addCornerPiece("corner_power_ne", powerMaxX, powerMinZ, kWallColor);
    addCornerPiece("corner_power_se", powerMaxX, powerMaxZ, kWallColor);
    addCornerPiece("corner_oxygen_nw", oxygenMinX, oxygenMinZ, kWallColor);
    addCornerPiece("corner_oxygen_ne", oxygenMaxX, oxygenMinZ, kWallColor);
    addCornerPiece("corner_lab_nw", labMinX, labMinZ, kWallColor);
    addCornerPiece("corner_lab_ne", labMaxX, labMinZ, kWallColor);

    addDoor(
        "Control Door",
        glm::vec3(-4.0f, 1.0f, 0.0f),
        glm::vec3(0.12f, 1.1f, 1.15f),
        90.0f,
        false,
        glm::vec3(0.85f, 0.20f, 0.35f),
        glm::vec3(0.35f, 0.95f, 0.65f)
    );
    addDoor(
        "Power Door",
        glm::vec3(4.0f, 1.0f, 0.0f),
        glm::vec3(0.12f, 1.1f, 1.15f),
        90.0f,
        true,
        glm::vec3(0.65f, 0.15f, 0.15f),
        glm::vec3(0.15f, 0.85f, 0.35f)
    );
    addDoor(
        "Storage Door",
        glm::vec3(0.0f, 1.0f, 4.0f),
        glm::vec3(1.1f, 1.1f, 0.12f),
        0.0f,
        false,
        glm::vec3(0.75f, 0.25f, 0.15f),
        glm::vec3(0.15f, 0.85f, 0.35f)
    );
    addDoor(
        "Oxygen Door",
        glm::vec3(0.0f, 1.0f, -4.0f),
        glm::vec3(1.1f, 1.1f, 0.12f),
        0.0f,
        true,
        glm::vec3(0.65f, 0.15f, 0.15f),
        glm::vec3(0.15f, 0.85f, 0.35f)
    );
    addDoor(
        "Lab Door",
        glm::vec3(0.0f, 1.0f, -12.0f),
        glm::vec3(1.1f, 1.1f, 0.12f),
        0.0f,
        false,
        glm::vec3(0.75f, 0.25f, 0.15f),
        glm::vec3(0.15f, 0.85f, 0.35f)
    );

    addObject("bed", glm::vec3(-1.3f, 0.15f, 1.6f), glm::vec3(2.2f, 0.5f, 1.3f), glm::vec3(0.55f, 0.55f, 0.65f), true);
    addObject("locker", glm::vec3(2.0f, 1.0f, 2.2f), glm::vec3(1.0f, 2.0f, 0.9f), glm::vec3(0.30f, 0.40f, 0.55f), true);
    addObject("oxygen_console", glm::vec3(0.0f, 0.75f, -8.0f), glm::vec3(1.4f, 1.5f, 0.9f), glm::vec3(0.78f, 0.78f, 0.20f), true);
    addObject("power_console", glm::vec3(8.0f, 0.75f, 0.0f), glm::vec3(1.4f, 1.5f, 0.9f), glm::vec3(0.20f, 0.55f, 0.95f), true);
    addObject("storage_note", glm::vec3(0.0f, 0.45f, 8.0f), glm::vec3(0.9f, 0.9f, 0.9f), glm::vec3(0.80f, 0.48f, 0.20f), true);
    addObject("lab_decoder", glm::vec3(0.0f, 0.75f, -16.0f), glm::vec3(1.6f, 1.4f, 1.0f), glm::vec3(0.25f, 0.78f, 0.88f), true);
    addObject("control_terminal", glm::vec3(-8.0f, 0.85f, 0.0f), glm::vec3(2.1f, 1.8f, 1.0f), glm::vec3(0.88f, 0.22f, 0.78f), true);

    addObject("control_beacon_left", glm::vec3(-10.5f, 1.4f, 2.0f), glm::vec3(0.4f, 2.8f, 0.4f), glm::vec3(0.75f, 0.18f, 0.35f), false);
    addObject("control_beacon_right", glm::vec3(-10.5f, 1.4f, -2.0f), glm::vec3(0.4f, 2.8f, 0.4f), glm::vec3(0.75f, 0.18f, 0.35f), false);

    rooms.push_back({ "Bedroom", glm::vec3(bedroomMinX, -100.0f, bedroomMinZ), glm::vec3(bedroomMaxX, 100.0f, bedroomMaxZ) });
    rooms.push_back({ "Storage", glm::vec3(storageMinX, -100.0f, storageMinZ), glm::vec3(storageMaxX, 100.0f, storageMaxZ) });
    rooms.push_back({ "Control Room", glm::vec3(controlMinX, -100.0f, controlMinZ), glm::vec3(controlMaxX, 100.0f, controlMaxZ) });
    rooms.push_back({ "Power Room", glm::vec3(powerMinX, -100.0f, powerMinZ), glm::vec3(powerMaxX, 100.0f, powerMaxZ) });
    rooms.push_back({ "Oxygen Room", glm::vec3(oxygenMinX, -100.0f, oxygenMinZ), glm::vec3(oxygenMaxX, 100.0f, oxygenMaxZ) });
    rooms.push_back({ "Lab", glm::vec3(labMinX, -100.0f, labMinZ), glm::vec3(labMaxX, 100.0f, labMaxZ) });

    interactables.push_back({
        "oxygen_console",
        glm::vec3(0.0f, 0.0f, -8.0f),
        1.9f,
        [this]()
        {
            if (!gameState || gameState->oxygenFixed)
            {
                std::cout << "Oxygen already stable\n";
                return;
            }

            gameState->oxygenFixed = true;
            std::cout << "Oxygen fixed\n";
        }
    });

    interactables.push_back({
        "power_console",
        glm::vec3(8.0f, 0.0f, 0.0f),
        1.9f,
        [this]()
        {
            if (!gameState)
                return;

            if (!gameState->oxygenFixed)
            {
                std::cout << "Power locked: fix oxygen first\n";
                return;
            }

            if (gameState->powerFixed)
            {
                std::cout << "Power already restored\n";
                return;
            }

            gameState->powerFixed = true;
            gameState->storageUnlocked = true;
            gameState->labUnlocked = true;

            if (Door* storageDoor = findDoor("Storage Door"))
                storageDoor->open = true;
            if (Door* labDoor = findDoor("Lab Door"))
                labDoor->open = true;

            std::cout << "Power restored\n";
            std::cout << "Storage and Lab unlocked\n";
        }
    });

    interactables.push_back({
        "storage_note",
        glm::vec3(0.0f, 0.0f, 8.0f),
        1.7f,
        [this]()
        {
            if (!gameState)
                return;

            if (!gameState->storageUnlocked)
            {
                std::cout << "Storage locked\n";
                return;
            }

            if (gameState->foundNote)
            {
                std::cout << "Nothing else here\n";
                return;
            }

            gameState->foundNote = true;
            std::cout << "You found a note\n";
        }
    });

    interactables.push_back({
        "lab_decoder",
        glm::vec3(0.0f, 0.0f, -16.0f),
        1.9f,
        [this]()
        {
            if (!gameState)
                return;

            if (!gameState->labUnlocked)
            {
                std::cout << "Lab locked\n";
                return;
            }

            if (!gameState->foundNote)
            {
                std::cout << "Need hint from storage first\n";
                return;
            }

            if (gameState->hasCode)
            {
                std::cout << "Code already obtained\n";
                return;
            }

            gameState->hasCode = true;
            std::cout << "You decoded the code\n";
            std::cout << "Return to the control door\n";
        }
    });

    interactables.push_back({
        "control_door",
        glm::vec3(-5.0f, 0.0f, 0.0f),
        1.8f,
        [this]()
        {
            if (!gameState)
                return;

            if (!gameState->hasCode)
            {
                std::cout << "Control locked: need code\n";
                return;
            }

            if (gameState->controlUnlocked)
            {
                std::cout << "Control room already unlocked\n";
                return;
            }

            gameState->controlUnlocked = true;

            if (Door* controlDoor = findDoor("Control Door"))
                controlDoor->open = true;

            std::cout << "Control room unlocked\n";
        }
    });

    interactables.push_back({
        "control_terminal",
        glm::vec3(-8.0f, 0.0f, 0.0f),
        2.0f,
        [this]()
        {
            if (!gameState)
                return;

            if (!gameState->controlUnlocked)
            {
                std::cout << "Control room locked\n";
                return;
            }

            if (gameState->gameFinished)
            {
                std::cout << "Mission already complete\n";
                return;
            }

            gameState->gameFinished = true;
            std::cout << "Escape successful\n";
        }
    });

    rebuildColliders();
}

void World::rebuildColliders()
{
    colliders.clear();

    for (const auto& obj : staticObjects)
    {
        if (!obj.hasCollision)
            continue;

        colliders.push_back({ obj.pos, obj.scale * 0.5f });
    }
}

bool World::intersectsCircleBoxXZ(const glm::vec3& circleCenter, float radius, const BoxCollider& box)
{
    float minX = box.center.x - box.halfSize.x;
    float maxX = box.center.x + box.halfSize.x;
    float minZ = box.center.z - box.halfSize.z;
    float maxZ = box.center.z + box.halfSize.z;

    float closestX = glm::clamp(circleCenter.x, minX, maxX);
    float closestZ = glm::clamp(circleCenter.z, minZ, maxZ);

    float dx = circleCenter.x - closestX;
    float dz = circleCenter.z - closestZ;

    return (dx * dx + dz * dz) < (radius * radius);
}

bool World::collidesWithWorld(const glm::vec3& testPos, float playerRadius) const
{
    for (const auto& box : colliders)
    {
        if (intersectsCircleBoxXZ(testPos, playerRadius, box))
            return true;
    }

    for (const auto& door : doors)
    {
        if (!door.open && intersectsCircleBoxXZ(testPos, playerRadius, { door.center, door.halfSize }))
            return true;
    }

    return false;
}

int World::getCurrentRoomIndex(const glm::vec3& playerPos) const
{
    for (int i = 0; i < (int)rooms.size(); i++)
    {
        const Room& r = rooms[i];

        if (playerPos.x >= r.min.x && playerPos.x <= r.max.x &&
            playerPos.z >= r.min.z && playerPos.z <= r.max.z)
        {
            return i;
        }
    }

    return -1;
}

int World::getNearestInteractableIndex(const glm::vec3& playerPos) const
{
    int nearestIndex = -1;
    float nearestDistSq = std::numeric_limits<float>::max();

    for (int i = 0; i < (int)interactables.size(); i++)
    {
        const auto& interactable = interactables[i];

        float dx = playerPos.x - interactable.pos.x;
        float dz = playerPos.z - interactable.pos.z;
        float distSq = dx * dx + dz * dz;
        float radiusSq = interactable.radius * interactable.radius;

        if (distSq <= radiusSq && distSq < nearestDistSq)
        {
            nearestDistSq = distSq;
            nearestIndex = i;
        }
    }

    return nearestIndex;
}

int World::getCurrentObjectiveInteractableIndex() const
{
    if (!gameState)
        return -1;

    if (!gameState->oxygenFixed) return 0;
    if (!gameState->powerFixed) return 1;
    if (!gameState->foundNote) return 2;
    if (!gameState->hasCode) return 3;
    if (!gameState->controlUnlocked) return 4;
    if (!gameState->gameFinished) return 5;

    return -1;
}

int World::getNearestObjectiveInteractableIndex(const glm::vec3& playerPos) const
{
    int objectiveIndex = getCurrentObjectiveInteractableIndex();
    if (objectiveIndex == -1)
        return -1;

    const auto& interactable = interactables[objectiveIndex];

    float dx = playerPos.x - interactable.pos.x;
    float dz = playerPos.z - interactable.pos.z;
    float distSq = dx * dx + dz * dz;
    float radiusSq = interactable.radius * interactable.radius;

    if (distSq <= radiusSq)
        return objectiveIndex;

    return -1;
}

bool World::isNearCurrentObjectiveInteractable(const glm::vec3& playerPos) const
{
    return getNearestObjectiveInteractableIndex(playerPos) != -1;
}

void World::tryInteract(const glm::vec3& playerPos)
{
    int objectiveIndex = getNearestObjectiveInteractableIndex(playerPos);
    if (objectiveIndex == -1)
        return;

    interactables[objectiveIndex].onInteract();
}
