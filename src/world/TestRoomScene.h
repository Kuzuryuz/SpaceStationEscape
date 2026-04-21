#pragma once

#include <glm/glm.hpp>

#include <vector>

#include "world/World.h"

static constexpr bool kTemplateRoomTestMode = true;

struct ModelPlacement
{
    glm::vec3 position{ 0.0f };
    glm::vec3 scale{ 1.0f };
    float rotationY = 0.0f;
    glm::vec3 color{ 1.0f };
};

struct TestRoomScene
{
    std::vector<ModelPlacement> roomPlacements;
    std::vector<ModelPlacement> corridorPlacements;
    std::vector<ModelPlacement> gatePlacements;
    std::vector<ModelPlacement> gateDoorPlacements;
    std::vector<ModelPlacement> powerUnlockGatePlacements;
    std::vector<ModelPlacement> controlUnlockGatePlacements;
    std::vector<ModelPlacement> bedPlacements;
    std::vector<ModelPlacement> bedCoverPlacements;
    std::vector<ModelPlacement> labTableDisplayPlacements;
    ModelPlacement labSkipRocksPlacement;
    ModelPlacement labRocksPlacement;
    ModelPlacement oxygenConsolePlacement;
    ModelPlacement powerConsolePlacement;
    ModelPlacement storageNotePlacement;
    ModelPlacement controlTerminalPlacement;
    glm::vec3 playerStart{ 0.0f, 0.0f, 6.0f };
};

TestRoomScene createTestRoomScene();
void configureTestRoomWorld(World& world, const TestRoomScene& scene, bool powerFixed, bool controlUnlocked);
