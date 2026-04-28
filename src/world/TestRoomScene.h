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
    float rotationX = 0.0f;
    float rotationZ = 0.0f;
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
    std::vector<ModelPlacement> storageContainerPlacements;
    std::vector<ModelPlacement> storageContainerFlatPlacements;
    std::vector<ModelPlacement> storageContainerFlatOpenPlacements;
    std::vector<ModelPlacement> storageContainerTallPlacements;
    std::vector<ModelPlacement> storageContainerWidePlacements;
    std::vector<ModelPlacement> labTableDisplayPlacements;
    std::vector<ModelPlacement> labSmallTableDisplayPlacements;
    std::vector<ModelPlacement> labTableInsetPlacements;
    ModelPlacement labComputerPlacement;
    ModelPlacement labComputerScreenPlacement;
    ModelPlacement labSkipRocksPlacement;
    ModelPlacement labRocksPlacement;
    ModelPlacement labCenterRocksPlacement;
    ModelPlacement labPosterPlacement;
    ModelPlacement oxygenConsolePlacement;
    ModelPlacement oxygenComputerPlacement;
    ModelPlacement oxygenComputerScreenPlacement;
    std::vector<ModelPlacement> oxygenValvePlacements;
    ModelPlacement oxygenPosterPlacement;
    std::vector<ModelPlacement> oxygenTankPlacements;
    std::vector<ModelPlacement> oxygenPlantPlacements;
    ModelPlacement powerConsolePlacement;
    std::vector<ModelPlacement> powerCabinetPlacements;
    std::vector<ModelPlacement> powerComputerSystemPlacements;
    ModelPlacement powerPosterPowerOffPlacement;
    ModelPlacement powerPosterMaintenancePlacement;
    ModelPlacement powerPosterCalibrationPlacement;
    ModelPlacement controlTerminalPlacement;
    std::vector<ModelPlacement> controlComputerPlacements;
    std::vector<ModelPlacement> controlComputerWidePlacements;
    std::vector<ModelPlacement> controlDisplayWallWidePlacements;
    glm::vec3 playerStart{ 0.0f, 0.0f, 6.0f };
};

TestRoomScene createTestRoomScene();
void configureTestRoomWorld(World& world, const TestRoomScene& scene, bool powerFixed, bool controlUnlocked);
