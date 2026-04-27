#include "world/TestRoomScene.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>

namespace
{
    constexpr float kRoomLargeBaseY = -0.55f;
    constexpr float kRoomLargeHalfExtent = 9.35f;
    constexpr float kRoomLargeWallThickness = 0.55f;
    constexpr float kRoomLargeWallHeight = 2.2f;
    constexpr float kRoomLargeDoorOpeningHalfWidth = 1.0f;
    const glm::vec3 kRoomLargePillarCenter(2.0f, 1.0f, -2.0f);
    constexpr float kRoomLargePillarRadius = 0.9f;
    constexpr float kCorridorJoinOffset = 2.65f;
    constexpr float kCorridorCollisionHalfLength = kCorridorJoinOffset + 0.05f;
    constexpr float kCorridorWalkableHalfWidth = kRoomLargeDoorOpeningHalfWidth;
    constexpr float kCorridorWallHalfThickness = kRoomLargeWallThickness * 0.5f;
    constexpr float kCorridorOuterHalfWidth =
        kCorridorWalkableHalfWidth + kCorridorWallHalfThickness;
    constexpr float kGateHalfWidth = 2.1f;
    constexpr float kGateHalfThickness = 0.7f;
    constexpr float kGateFramePostHalfWidth =
        (kGateHalfWidth - kRoomLargeDoorOpeningHalfWidth) * 0.5f;
    constexpr float kGateFramePostOffset =
        kRoomLargeDoorOpeningHalfWidth + kGateFramePostHalfWidth;
    constexpr float kCornerFurnitureInset = 2.9f;
    constexpr float kBedScale = 4.0f;
    constexpr float kStoragePropScale = 2.0f;
    constexpr float kStorageLongContainerScale = 1.45f;
    constexpr float kStoragePropWallInset = 2.1f;
    constexpr float kStoragePropSideOffset = 5.4f;
    const glm::vec3 kStorageContainerHalfSize(0.65f, 0.45f, 0.65f);
    const glm::vec3 kStorageContainerFlatHalfSize(0.9f, 0.55f, 1.4f);
    const glm::vec3 kStorageContainerTallHalfSize(1.0f, 1.0f, 1.0f);
    const glm::vec3 kStorageContainerWideHalfSize(1.0f, 0.8f, 1.0f);
    constexpr float kLabTableDisplayScale = 3.0f;
    constexpr float kLabTableDisplayCornerInset = 1.75f;
    constexpr float kLabTableDisplaySpacing = 3.0f;
    const glm::vec3 kLabTableDisplayHalfSize(2.0f, 1.0f, 1.7f);
    constexpr float kLabTableInteractForwardOffset = 2.7f;
    constexpr float kLabCornerPropInset = 1.75f;
    constexpr float kLabCornerPropSpacing = 3.0f;
    constexpr float kLabSkipRocksScale = 2.4f;
    constexpr float kLabRocksScale = 2.2f;
    const glm::vec3 kLabSkipRocksHalfSize(1.1f, 0.85f, 1.6f);
    const glm::vec3 kLabRocksHalfSize(1.35f, 0.45f, 1.25f);
    const glm::vec3 kLabRocksCollisionCenterOffset(-0.56f * kLabRocksScale, 0.0f, 0.52f * kLabRocksScale);
    constexpr float kControlComputerScreenScale = 3.0f;
    constexpr float kControlComputerScreenWallInset = 1.5f;
    constexpr float kControlComputerInteractOffset = 2.2f;
    const glm::vec3 kControlComputerScreenHalfSize(1.6f, 1.0f, 1.0f);
    constexpr float kControlComputerSideSpacing = 2.0f;
    constexpr float kControlComputerWideSideSpacing = 4.0f;
    const glm::vec3 kControlComputerHalfSize(1.3f, 0.9f, 1.1f);
    const glm::vec3 kControlComputerWideHalfSize(1.7f, 0.9f, 1.2f);
    constexpr float kControlDisplayWallWideHeight = 2.0f;
    constexpr float kStorageNoteInteractBackOffset = 2.0f;
    const glm::vec3 kOxygenConsoleScale(1.4f, 1.5f, 0.9f);
    const glm::vec3 kOxygenConsoleColor(0.78f, 0.78f, 0.20f);
    const glm::vec3 kOxygenPosterScale(5.0f, 5.0f, 1.0f);
    constexpr float kOxygenValveHeight = -0.05f;
    constexpr float kOxygenValveScale = 2.5f;
    constexpr float kOxygenValveSpacing = 1.25f;
    const glm::vec3 kPowerConsoleScale(1.4f, 1.5f, 0.9f);
    const glm::vec3 kPowerConsoleColor(0.20f, 0.55f, 0.95f);
    const glm::vec3 kStorageNoteScale(0.9f, 0.9f, 0.9f);
    const glm::vec3 kStorageNoteColor(0.80f, 0.48f, 0.20f);
    const glm::vec3 kControlTerminalColor(0.88f, 0.22f, 0.78f);

    glm::vec3 rotateOffsetY(const glm::vec3& offset, float degrees)
    {
        const glm::mat4 rotation =
            glm::rotate(glm::mat4(1.0f), glm::radians(degrees), glm::vec3(0.0f, 1.0f, 0.0f));
        return glm::vec3(rotation * glm::vec4(offset, 0.0f));
    }

    glm::vec3 getOxygenRoomCenter()
    {
        const float linkedRoomCenterOffset = 2.0f * (kRoomLargeHalfExtent + kCorridorJoinOffset);
        return glm::vec3(0.0f, kRoomLargeBaseY, -linkedRoomCenterOffset);
    }

    glm::vec3 getStorageRoomCenter()
    {
        const float linkedRoomCenterOffset = 2.0f * (kRoomLargeHalfExtent + kCorridorJoinOffset);
        return glm::vec3(0.0f, kRoomLargeBaseY, linkedRoomCenterOffset);
    }

    glm::vec3 getControlRoomCenter()
    {
        const float linkedRoomCenterOffset = 2.0f * (kRoomLargeHalfExtent + kCorridorJoinOffset);
        return glm::vec3(-linkedRoomCenterOffset, kRoomLargeBaseY, 0.0f);
    }

    glm::vec3 getPowerRoomCenter()
    {
        const float linkedRoomCenterOffset = 2.0f * (kRoomLargeHalfExtent + kCorridorJoinOffset);
        return glm::vec3(linkedRoomCenterOffset, kRoomLargeBaseY, 0.0f);
    }

    glm::vec3 getLabRoomCenter()
    {
        const float linkedRoomCenterOffset = 2.0f * (kRoomLargeHalfExtent + kCorridorJoinOffset);
        return getOxygenRoomCenter() + glm::vec3(0.0f, 0.0f, -linkedRoomCenterOffset);
    }
}

TestRoomScene createTestRoomScene()
{
    TestRoomScene scene;
    const glm::vec3 oxygenRoomCenter = getOxygenRoomCenter();
    const glm::vec3 storageRoomCenter = getStorageRoomCenter();
    const glm::vec3 controlRoomCenter = getControlRoomCenter();
    const glm::vec3 powerRoomCenter = getPowerRoomCenter();
    const glm::vec3 labRoomCenter = getLabRoomCenter();

    scene.roomPlacements = {
        { glm::vec3(0.0f, kRoomLargeBaseY, 0.0f), glm::vec3(1.0f), 0.0f, glm::vec3(1.0f) },
        { oxygenRoomCenter, glm::vec3(1.0f), 0.0f, glm::vec3(1.0f) },
        { storageRoomCenter, glm::vec3(1.0f), 0.0f, glm::vec3(1.0f) },
        { controlRoomCenter, glm::vec3(1.0f), 0.0f, glm::vec3(1.0f) },
        { powerRoomCenter, glm::vec3(1.0f), 0.0f, glm::vec3(1.0f) },
        { labRoomCenter, glm::vec3(1.0f), 0.0f, glm::vec3(1.0f) }
    };

    scene.corridorPlacements = {
        { glm::vec3(0.0f, kRoomLargeBaseY, -kRoomLargeHalfExtent - kCorridorJoinOffset), glm::vec3(1.0f), 90.0f, glm::vec3(1.0f) },
        { glm::vec3(0.0f, kRoomLargeBaseY, kRoomLargeHalfExtent + kCorridorJoinOffset), glm::vec3(1.0f), 90.0f, glm::vec3(1.0f) },
        { glm::vec3(-kRoomLargeHalfExtent - kCorridorJoinOffset, kRoomLargeBaseY, 0.0f), glm::vec3(1.0f), 0.0f, glm::vec3(1.0f) },
        { glm::vec3(kRoomLargeHalfExtent + kCorridorJoinOffset, kRoomLargeBaseY, 0.0f), glm::vec3(1.0f), 0.0f, glm::vec3(1.0f) },
        { oxygenRoomCenter + glm::vec3(0.0f, 0.0f, -kRoomLargeHalfExtent - kCorridorJoinOffset), glm::vec3(1.0f), 90.0f, glm::vec3(1.0f) }
    };

    scene.gatePlacements = {
        { glm::vec3(0.0f, kRoomLargeBaseY, -kRoomLargeHalfExtent), glm::vec3(1.0f), 0.0f, glm::vec3(1.0f) },
        { glm::vec3(kRoomLargeHalfExtent, kRoomLargeBaseY, 0.0f), glm::vec3(1.0f), 90.0f, glm::vec3(1.0f) },
        { oxygenRoomCenter + glm::vec3(0.0f, 0.0f, kRoomLargeHalfExtent), glm::vec3(1.0f), 180.0f, glm::vec3(1.0f) },
        { powerRoomCenter + glm::vec3(-kRoomLargeHalfExtent, 0.0f, 0.0f), glm::vec3(1.0f), -90.0f, glm::vec3(1.0f) }
    };

    scene.gateDoorPlacements = {
        { controlRoomCenter + glm::vec3(0.0f, 0.0f, -kRoomLargeHalfExtent), glm::vec3(1.0f), 0.0f, glm::vec3(1.0f) },
        { controlRoomCenter + glm::vec3(-kRoomLargeHalfExtent, 0.0f, 0.0f), glm::vec3(1.0f), -90.0f, glm::vec3(1.0f) },
        { controlRoomCenter + glm::vec3(0.0f, 0.0f, kRoomLargeHalfExtent), glm::vec3(1.0f), 180.0f, glm::vec3(1.0f) },
        { oxygenRoomCenter + glm::vec3(-kRoomLargeHalfExtent, 0.0f, 0.0f), glm::vec3(1.0f), -90.0f, glm::vec3(1.0f) },
        { oxygenRoomCenter + glm::vec3(kRoomLargeHalfExtent, 0.0f, 0.0f), glm::vec3(1.0f), 90.0f, glm::vec3(1.0f) },
        { storageRoomCenter + glm::vec3(0.0f, 0.0f, kRoomLargeHalfExtent), glm::vec3(1.0f), 180.0f, glm::vec3(1.0f) },
        { storageRoomCenter + glm::vec3(-kRoomLargeHalfExtent, 0.0f, 0.0f), glm::vec3(1.0f), -90.0f, glm::vec3(1.0f) },
        { storageRoomCenter + glm::vec3(kRoomLargeHalfExtent, 0.0f, 0.0f), glm::vec3(1.0f), 90.0f, glm::vec3(1.0f) },
        { powerRoomCenter + glm::vec3(0.0f, 0.0f, -kRoomLargeHalfExtent), glm::vec3(1.0f), 0.0f, glm::vec3(1.0f) },
        { powerRoomCenter + glm::vec3(0.0f, 0.0f, kRoomLargeHalfExtent), glm::vec3(1.0f), 180.0f, glm::vec3(1.0f) },
        { powerRoomCenter + glm::vec3(kRoomLargeHalfExtent, 0.0f, 0.0f), glm::vec3(1.0f), 90.0f, glm::vec3(1.0f) },
        { labRoomCenter + glm::vec3(0.0f, 0.0f, -kRoomLargeHalfExtent), glm::vec3(1.0f), 0.0f, glm::vec3(1.0f) },
        { labRoomCenter + glm::vec3(-kRoomLargeHalfExtent, 0.0f, 0.0f), glm::vec3(1.0f), -90.0f, glm::vec3(1.0f) },
        { labRoomCenter + glm::vec3(kRoomLargeHalfExtent, 0.0f, 0.0f), glm::vec3(1.0f), 90.0f, glm::vec3(1.0f) }
    };

    scene.controlUnlockGatePlacements = {
        { glm::vec3(-kRoomLargeHalfExtent, kRoomLargeBaseY, 0.0f), glm::vec3(1.0f), -90.0f, glm::vec3(1.0f) },
        { controlRoomCenter + glm::vec3(kRoomLargeHalfExtent, 0.0f, 0.0f), glm::vec3(1.0f), 90.0f, glm::vec3(1.0f) }
    };

    scene.powerUnlockGatePlacements = {
        { glm::vec3(0.0f, kRoomLargeBaseY, kRoomLargeHalfExtent), glm::vec3(1.0f), 180.0f, glm::vec3(1.0f) },
        { storageRoomCenter + glm::vec3(0.0f, 0.0f, -kRoomLargeHalfExtent), glm::vec3(1.0f), 0.0f, glm::vec3(1.0f) },
        { oxygenRoomCenter + glm::vec3(0.0f, 0.0f, -kRoomLargeHalfExtent), glm::vec3(1.0f), 0.0f, glm::vec3(1.0f) },
        { labRoomCenter + glm::vec3(0.0f, 0.0f, kRoomLargeHalfExtent), glm::vec3(1.0f), 180.0f, glm::vec3(1.0f) }
    };

    scene.oxygenConsolePlacement = {
        oxygenRoomCenter + glm::vec3(0.0f, 1.0f, 4.9f),
        kOxygenConsoleScale,
        0.0f,
        kOxygenConsoleColor
    };
    scene.oxygenValvePlacements = {
        { oxygenRoomCenter + glm::vec3(-kOxygenValveSpacing, kOxygenValveHeight, 1.15f), glm::vec3(kOxygenValveScale), 0.0f, glm::vec3(1.0f, 0.30f, 0.24f) },
        { oxygenRoomCenter + glm::vec3(0.0f, kOxygenValveHeight, 1.15f), glm::vec3(kOxygenValveScale), 0.0f, glm::vec3(0.20f, 0.58f, 1.0f) },
        { oxygenRoomCenter + glm::vec3(kOxygenValveSpacing, kOxygenValveHeight, 1.15f), glm::vec3(kOxygenValveScale), 0.0f, glm::vec3(0.25f, 0.95f, 0.38f) }
    };
    scene.oxygenPosterPlacement = {
        oxygenRoomCenter + glm::vec3(-kRoomLargeHalfExtent + 0.01f, 2.5f, 4.0f),
        kOxygenPosterScale,
        90.0f,
        glm::vec3(1.0f)
    };

    scene.powerConsolePlacement = {
        powerRoomCenter + glm::vec3(0.0f, 1.0f, 0.0f),
        kPowerConsoleScale,
        0.0f,
        kPowerConsoleColor
    };

    const float storagePropY = kRoomLargeBaseY + 0.55f;
    scene.storageContainerPlacements = {
        { storageRoomCenter + glm::vec3(-kStoragePropSideOffset, storagePropY, kRoomLargeHalfExtent - kStoragePropWallInset), glm::vec3(kStorageLongContainerScale), 90.0f, glm::vec3(1.0f) },
        { storageRoomCenter + glm::vec3(-kRoomLargeHalfExtent + kStoragePropWallInset, storagePropY, kStoragePropSideOffset), glm::vec3(kStorageLongContainerScale), 0.0f, glm::vec3(1.0f) }
    };
    scene.storageContainerFlatPlacements = {
        { storageRoomCenter + glm::vec3(2.25f, storagePropY, 0.9f), glm::vec3(kStoragePropScale), 45.0f, glm::vec3(1.0f) },
        { storageRoomCenter + glm::vec3(kStoragePropSideOffset, storagePropY, kRoomLargeHalfExtent - kStoragePropWallInset), glm::vec3(kStoragePropScale), 90.0f, glm::vec3(1.0f) },
        { storageRoomCenter + glm::vec3(-kStoragePropSideOffset, storagePropY, -kRoomLargeHalfExtent + kStoragePropWallInset), glm::vec3(kStoragePropScale), -90.0f, glm::vec3(1.0f) }
    };
    scene.storageContainerFlatOpenPlacements = {
        { storageRoomCenter + glm::vec3(kRoomLargeHalfExtent - kStoragePropWallInset, storagePropY, -kStoragePropSideOffset), glm::vec3(kStoragePropScale), 0.0f, glm::vec3(1.0f) }
    };
    scene.storageContainerTallPlacements = {
        { storageRoomCenter + glm::vec3(0.0f, storagePropY, 0.0f), glm::vec3(kStoragePropScale), 35.0f, glm::vec3(1.0f) },
        { storageRoomCenter + glm::vec3(-kRoomLargeHalfExtent + kStoragePropWallInset, storagePropY, -kStoragePropSideOffset), glm::vec3(kStoragePropScale), 25.0f, glm::vec3(1.0f) },
        { storageRoomCenter + glm::vec3(kRoomLargeHalfExtent - kStoragePropWallInset, storagePropY, kStoragePropSideOffset), glm::vec3(kStoragePropScale), -20.0f, glm::vec3(1.0f) }
    };
    scene.storageContainerWidePlacements = {
        { storageRoomCenter + glm::vec3(kStoragePropSideOffset, storagePropY, -kRoomLargeHalfExtent + kStoragePropWallInset), glm::vec3(kStoragePropScale), -90.0f, glm::vec3(1.0f) }
    };

    scene.controlTerminalPlacement = {
        controlRoomCenter + glm::vec3(-kRoomLargeHalfExtent + kControlComputerScreenWallInset, 0.0f, 0.0f),
        glm::vec3(kControlComputerScreenScale),
        90.0f,
        kControlTerminalColor
    };
    scene.controlComputerPlacements = {
        {
            scene.controlTerminalPlacement.position + glm::vec3(0.0f, 0.0f, -kControlComputerSideSpacing),
            glm::vec3(kControlComputerScreenScale),
            90.0f,
            glm::vec3(1.0f)
        },
        {
            scene.controlTerminalPlacement.position + glm::vec3(0.0f, 0.0f, kControlComputerSideSpacing),
            glm::vec3(kControlComputerScreenScale),
            90.0f,
            glm::vec3(1.0f)
        }
    };
    scene.controlComputerWidePlacements = {
        {
            scene.controlTerminalPlacement.position + glm::vec3(-0.5f, 0.0f, -kControlComputerWideSideSpacing),
            glm::vec3(kControlComputerScreenScale),
            90.0f,
            glm::vec3(1.0f)
        },
        {
            scene.controlTerminalPlacement.position + glm::vec3(-0.5f, 0.0f, kControlComputerWideSideSpacing),
            glm::vec3(kControlComputerScreenScale),
            90.0f,
            glm::vec3(1.0f)
        }
    };
    scene.controlDisplayWallWidePlacements = {
        {
            scene.controlComputerWidePlacements[0].position + glm::vec3(-1.0f, kControlDisplayWallWideHeight, 0.0f),
            glm::vec3(kControlComputerScreenScale),
            90.0f,
            glm::vec3(1.0f)
        },
        {
            scene.controlComputerWidePlacements[1].position + glm::vec3(-1.0f, kControlDisplayWallWideHeight, 0.0f),
            glm::vec3(kControlComputerScreenScale),
            90.0f,
            glm::vec3(1.0f)
        }
    };

    const glm::vec3 labTableCorner =
        labRoomCenter + glm::vec3(
            -kRoomLargeHalfExtent + kLabTableDisplayCornerInset,
            kRoomLargeBaseY,
            -kRoomLargeHalfExtent + kLabTableDisplayCornerInset);
    scene.labTableDisplayPlacements = {
        { labTableCorner + glm::vec3(kLabTableDisplaySpacing, 1.3f, 0.0f), glm::vec3(kLabTableDisplayScale), 0.0f, glm::vec3(1.0f) },
        { labTableCorner + glm::vec3(0.0f, 1.3f, kLabTableDisplaySpacing), glm::vec3(kLabTableDisplayScale), 90.0f, glm::vec3(1.0f) }
    };

    const glm::vec3 labSkipRocksCorner =
        labRoomCenter + glm::vec3(
            kRoomLargeHalfExtent - kLabCornerPropInset,
            kRoomLargeBaseY,
            -kRoomLargeHalfExtent + kLabCornerPropInset);
    const glm::vec3 labRocksCorner =
        labRoomCenter + glm::vec3(
            kRoomLargeHalfExtent - 0.75f,
            kRoomLargeBaseY,
            -kRoomLargeHalfExtent + 0.75f);
    scene.labSkipRocksPlacement = {
        labSkipRocksCorner + glm::vec3(-kLabCornerPropSpacing, 0.3f, 0.0f),
        glm::vec3(kLabSkipRocksScale),
        90.0f,
        glm::vec3(1.0f)
    };
    scene.labRocksPlacement = {
        labRocksCorner + glm::vec3(0.0f, 0.5f, 0.0f),
        glm::vec3(kLabRocksScale),
        0.0f,
        glm::vec3(1.0f)
    };

    const glm::vec3 bedCoverLocalOffset(-0.25f * kBedScale, 0.20f * kBedScale, 0.15f * kBedScale);
    auto addBedSet = [&](const glm::vec3& position, float rotationY)
    {
        scene.bedPlacements.push_back({ position, glm::vec3(kBedScale), rotationY, glm::vec3(1.0f) });
        scene.bedCoverPlacements.push_back({
            position + rotateOffsetY(bedCoverLocalOffset, rotationY),
            glm::vec3(kBedScale),
            rotationY,
            glm::vec3(1.0f)
        });
    };

    const float bedY = kRoomLargeBaseY - 0.5f;
    addBedSet(glm::vec3(-kRoomLargeHalfExtent + kCornerFurnitureInset, bedY, -kRoomLargeHalfExtent + kCornerFurnitureInset), 0.0f);
    addBedSet(glm::vec3(kRoomLargeHalfExtent - kCornerFurnitureInset, bedY, -kRoomLargeHalfExtent + kCornerFurnitureInset), 0.0f);
    addBedSet(glm::vec3(-kRoomLargeHalfExtent + kCornerFurnitureInset, bedY, kRoomLargeHalfExtent - kCornerFurnitureInset), 180.0f);
    addBedSet(glm::vec3(kRoomLargeHalfExtent - kCornerFurnitureInset, bedY, kRoomLargeHalfExtent - kCornerFurnitureInset), 180.0f);

    scene.playerStart = glm::vec3(0.0f);

    return scene;
}

void configureTestRoomWorld(World& world, const TestRoomScene& scene, bool powerFixed, bool controlUnlocked)
{
    const glm::vec3 oxygenRoomCenter = getOxygenRoomCenter();
    const glm::vec3 storageRoomCenter = getStorageRoomCenter();
    const glm::vec3 controlRoomCenter = getControlRoomCenter();
    const glm::vec3 powerRoomCenter = getPowerRoomCenter();
    const glm::vec3 labRoomCenter = getLabRoomCenter();

    auto addCorridorCollision = [&](const glm::vec3& position, bool alongZ)
    {
        if (alongZ)
        {
            world.colliders.push_back({
                glm::vec3(position.x - kCorridorOuterHalfWidth, 1.0f, position.z),
                glm::vec3(kCorridorWallHalfThickness, kRoomLargeWallHeight, kCorridorCollisionHalfLength)
            });
            world.colliders.push_back({
                glm::vec3(position.x + kCorridorOuterHalfWidth, 1.0f, position.z),
                glm::vec3(kCorridorWallHalfThickness, kRoomLargeWallHeight, kCorridorCollisionHalfLength)
            });
        }
        else
        {
            world.colliders.push_back({
                glm::vec3(position.x, 1.0f, position.z - kCorridorOuterHalfWidth),
                glm::vec3(kCorridorCollisionHalfLength, kRoomLargeWallHeight, kCorridorWallHalfThickness)
            });
            world.colliders.push_back({
                glm::vec3(position.x, 1.0f, position.z + kCorridorOuterHalfWidth),
                glm::vec3(kCorridorCollisionHalfLength, kRoomLargeWallHeight, kCorridorWallHalfThickness)
            });
        }
    };

    auto addGateFrameCollision = [&](const ModelPlacement& placement)
    {
        const bool gateRunsAlongX = std::abs(std::cos(glm::radians(placement.rotationY))) > 0.5f;

        if (gateRunsAlongX)
        {
            world.colliders.push_back({
                placement.position + glm::vec3(-kGateFramePostOffset, 1.0f, 0.0f),
                glm::vec3(kGateFramePostHalfWidth, kRoomLargeWallHeight, kGateHalfThickness)
            });
            world.colliders.push_back({
                placement.position + glm::vec3(kGateFramePostOffset, 1.0f, 0.0f),
                glm::vec3(kGateFramePostHalfWidth, kRoomLargeWallHeight, kGateHalfThickness)
            });
        }
        else
        {
            world.colliders.push_back({
                placement.position + glm::vec3(0.0f, 1.0f, -kGateFramePostOffset),
                glm::vec3(kGateHalfThickness, kRoomLargeWallHeight, kGateFramePostHalfWidth)
            });
            world.colliders.push_back({
                placement.position + glm::vec3(0.0f, 1.0f, kGateFramePostOffset),
                glm::vec3(kGateHalfThickness, kRoomLargeWallHeight, kGateFramePostHalfWidth)
            });
        }
    };

    auto addGateDoorCollision = [&](const ModelPlacement& placement)
    {
        const bool gateRunsAlongX = std::abs(std::cos(glm::radians(placement.rotationY))) > 0.5f;
        addGateFrameCollision(placement);

        world.colliders.push_back({
            placement.position + glm::vec3(0.0f, 1.0f, 0.0f),
            gateRunsAlongX
                ? glm::vec3(kRoomLargeDoorOpeningHalfWidth, kRoomLargeWallHeight, kGateHalfThickness)
                : glm::vec3(kGateHalfThickness, kRoomLargeWallHeight, kRoomLargeDoorOpeningHalfWidth)
        });
    };

    world.colliders.clear();
    world.circleColliders.clear();
    world.cylinderColliders.clear();
    world.horizontalCylinderColliders.clear();
    world.doors.clear();
    world.rooms.clear();
    world.rooms.push_back({
        "Main Room",
        glm::vec3(-kRoomLargeHalfExtent, -100.0f, -kRoomLargeHalfExtent),
        glm::vec3(kRoomLargeHalfExtent, 100.0f, kRoomLargeHalfExtent)
    });
    world.rooms.push_back({
        "Oxygen Room",
        oxygenRoomCenter + glm::vec3(-kRoomLargeHalfExtent, -100.0f, -kRoomLargeHalfExtent),
        oxygenRoomCenter + glm::vec3(kRoomLargeHalfExtent, 100.0f, kRoomLargeHalfExtent)
    });
    world.rooms.push_back({
        "Storage",
        storageRoomCenter + glm::vec3(-kRoomLargeHalfExtent, -100.0f, -kRoomLargeHalfExtent),
        storageRoomCenter + glm::vec3(kRoomLargeHalfExtent, 100.0f, kRoomLargeHalfExtent)
    });
    world.rooms.push_back({
        "Control Room",
        controlRoomCenter + glm::vec3(-kRoomLargeHalfExtent, -100.0f, -kRoomLargeHalfExtent),
        controlRoomCenter + glm::vec3(kRoomLargeHalfExtent, 100.0f, kRoomLargeHalfExtent)
    });
    world.rooms.push_back({
        "Power Room",
        powerRoomCenter + glm::vec3(-kRoomLargeHalfExtent, -100.0f, -kRoomLargeHalfExtent),
        powerRoomCenter + glm::vec3(kRoomLargeHalfExtent, 100.0f, kRoomLargeHalfExtent)
    });
    world.rooms.push_back({
        "Lab",
        labRoomCenter + glm::vec3(-kRoomLargeHalfExtent, -100.0f, -kRoomLargeHalfExtent),
        labRoomCenter + glm::vec3(kRoomLargeHalfExtent, 100.0f, kRoomLargeHalfExtent)
    });

    auto addRoomCollision = [&](const glm::vec3& roomCenter, bool openNegZ, bool openPosZ, bool openNegX, bool openPosX)
    {
        const float wallCenterOffset = kRoomLargeHalfExtent - kRoomLargeWallThickness * 0.5f;
        const float wallSegmentHalfLength = (kRoomLargeHalfExtent - kRoomLargeDoorOpeningHalfWidth) * 0.5f;
        const float wallSegmentCenterOffset = kRoomLargeDoorOpeningHalfWidth + wallSegmentHalfLength;
        const glm::vec3 roomOffset(roomCenter.x, 0.0f, roomCenter.z);

        auto addHorizontalWall = [&](float z, bool hasOpening)
        {
            if (hasOpening)
            {
                world.colliders.push_back({
                    roomOffset + glm::vec3(-wallSegmentCenterOffset, 1.0f, z),
                    glm::vec3(wallSegmentHalfLength, kRoomLargeWallHeight, kRoomLargeWallThickness * 0.5f)
                });
                world.colliders.push_back({
                    roomOffset + glm::vec3(wallSegmentCenterOffset, 1.0f, z),
                    glm::vec3(wallSegmentHalfLength, kRoomLargeWallHeight, kRoomLargeWallThickness * 0.5f)
                });
            }
            else
            {
                world.colliders.push_back({
                    roomOffset + glm::vec3(0.0f, 1.0f, z),
                    glm::vec3(kRoomLargeHalfExtent, kRoomLargeWallHeight, kRoomLargeWallThickness * 0.5f)
                });
            }
        };

        auto addVerticalWall = [&](float x, bool hasOpening)
        {
            if (hasOpening)
            {
                world.colliders.push_back({
                    roomOffset + glm::vec3(x, 1.0f, -wallSegmentCenterOffset),
                    glm::vec3(kRoomLargeWallThickness * 0.5f, kRoomLargeWallHeight, wallSegmentHalfLength)
                });
                world.colliders.push_back({
                    roomOffset + glm::vec3(x, 1.0f, wallSegmentCenterOffset),
                    glm::vec3(kRoomLargeWallThickness * 0.5f, kRoomLargeWallHeight, wallSegmentHalfLength)
                });
            }
            else
            {
                world.colliders.push_back({
                    roomOffset + glm::vec3(x, 1.0f, 0.0f),
                    glm::vec3(kRoomLargeWallThickness * 0.5f, kRoomLargeWallHeight, kRoomLargeHalfExtent)
                });
            }
        };

        addHorizontalWall(-wallCenterOffset, openNegZ);
        addHorizontalWall(wallCenterOffset, openPosZ);
        addVerticalWall(-wallCenterOffset, openNegX);
        addVerticalWall(wallCenterOffset, openPosX);
    };

    addRoomCollision(glm::vec3(0.0f), true, true, true, true);
    addRoomCollision(oxygenRoomCenter, true, true, false, false);
    addRoomCollision(storageRoomCenter, true, false, false, false);
    addRoomCollision(controlRoomCenter, false, false, false, true);
    addRoomCollision(powerRoomCenter, false, false, true, false);
    addRoomCollision(labRoomCenter, false, true, false, false);
    world.circleColliders.push_back({ kRoomLargePillarCenter, kRoomLargePillarRadius });
    world.circleColliders.push_back({
        oxygenRoomCenter + glm::vec3(kRoomLargePillarCenter.x, 0.0f, kRoomLargePillarCenter.z),
        kRoomLargePillarRadius
    });
    world.circleColliders.push_back({
        storageRoomCenter + glm::vec3(kRoomLargePillarCenter.x, 0.0f, kRoomLargePillarCenter.z),
        kRoomLargePillarRadius
    });
    world.circleColliders.push_back({
        controlRoomCenter + glm::vec3(kRoomLargePillarCenter.x, 0.0f, kRoomLargePillarCenter.z),
        kRoomLargePillarRadius
    });
    world.circleColliders.push_back({
        powerRoomCenter + glm::vec3(kRoomLargePillarCenter.x, 0.0f, kRoomLargePillarCenter.z),
        kRoomLargePillarRadius
    });
    world.circleColliders.push_back({
        labRoomCenter + glm::vec3(kRoomLargePillarCenter.x, 0.0f, kRoomLargePillarCenter.z),
        kRoomLargePillarRadius
    });

    for (const auto& bedPlacement : scene.bedPlacements)
    {
        world.colliders.push_back({
            glm::vec3(bedPlacement.position.x, bedPlacement.position.y + 0.175f * kBedScale, bedPlacement.position.z),
            glm::vec3(0.6f * kBedScale, 0.175f * kBedScale, 0.6f * kBedScale)
        });
    }

    for (const auto& tableDisplayPlacement : scene.labTableDisplayPlacements)
    {
        const bool tableRunsAlongX =
            std::abs(std::cos(glm::radians(tableDisplayPlacement.rotationY))) > 0.5f;
        world.colliders.push_back({
            tableDisplayPlacement.position + glm::vec3(0.0f, kLabTableDisplayHalfSize.y, 0.0f),
            tableRunsAlongX
                ? kLabTableDisplayHalfSize
                : glm::vec3(kLabTableDisplayHalfSize.z, kLabTableDisplayHalfSize.y, kLabTableDisplayHalfSize.x)
        });
    }

    auto addAxisAlignedRotatedCollider = [&](const ModelPlacement& placement, const glm::vec3& halfSize)
    {
        const bool runsAlongX = std::abs(std::cos(glm::radians(placement.rotationY))) > 0.5f;
        world.colliders.push_back({
            placement.position + glm::vec3(0.0f, halfSize.y, 0.0f),
            runsAlongX
                ? halfSize
                : glm::vec3(halfSize.z, halfSize.y, halfSize.x)
        });
    };

    auto addCylinderPropCollider = [&](const ModelPlacement& placement, const glm::vec3& halfSize)
    {
        world.cylinderColliders.push_back({
            placement.position + glm::vec3(0.0f, halfSize.y, 0.0f),
            std::max(halfSize.x, halfSize.z),
            halfSize.y
        });
    };

    auto addHorizontalCylinderPropCollider = [&](const ModelPlacement& placement, const glm::vec3& halfSize)
    {
        const glm::vec3 axis = rotateOffsetY(glm::vec3(0.0f, 0.0f, 1.0f), placement.rotationY);
        world.horizontalCylinderColliders.push_back({
            placement.position + glm::vec3(0.0f, halfSize.y, 0.0f),
            glm::vec3(axis.x, 0.0f, axis.z),
            halfSize.z,
            std::max(halfSize.x, halfSize.y)
        });
    };

    for (const auto& placement : scene.storageContainerPlacements)
        addCylinderPropCollider(placement, kStorageContainerHalfSize);
    for (const auto& placement : scene.storageContainerFlatPlacements)
        addHorizontalCylinderPropCollider(placement, kStorageContainerFlatHalfSize);
    for (const auto& placement : scene.storageContainerFlatOpenPlacements)
        addHorizontalCylinderPropCollider(placement, kStorageContainerFlatHalfSize);
    for (const auto& placement : scene.storageContainerTallPlacements)
        addCylinderPropCollider(placement, kStorageContainerTallHalfSize);
    for (const auto& placement : scene.storageContainerWidePlacements)
        addCylinderPropCollider(placement, kStorageContainerWideHalfSize);

    addAxisAlignedRotatedCollider(scene.labSkipRocksPlacement, kLabSkipRocksHalfSize);
    world.colliders.push_back({
        scene.labRocksPlacement.position + kLabRocksCollisionCenterOffset + glm::vec3(0.0f, kLabRocksHalfSize.y, 0.0f),
        kLabRocksHalfSize
    });

    world.colliders.push_back({
        scene.powerConsolePlacement.position,
        scene.powerConsolePlacement.scale * 0.5f
    });
    for (const auto& placement : scene.oxygenValvePlacements)
    {
        world.colliders.push_back({
            placement.position + glm::vec3(0.0f, 0.7f, 0.0f),
            glm::vec3(0.82f, 0.95f, 0.82f)
        });
    }
    addAxisAlignedRotatedCollider(scene.controlTerminalPlacement, kControlComputerScreenHalfSize);
    for (const auto& placement : scene.controlComputerPlacements)
        addAxisAlignedRotatedCollider(placement, kControlComputerHalfSize);
    for (const auto& placement : scene.controlComputerWidePlacements)
        addAxisAlignedRotatedCollider(placement, kControlComputerWideHalfSize);

    for (auto& interactable : world.interactables)
    {
        if (interactable.name == "oxygen_console")
        {
            interactable.pos = glm::vec3(
                scene.oxygenConsolePlacement.position.x,
                0.0f,
                scene.oxygenConsolePlacement.position.z);
        }
        else if (interactable.name.rfind("oxygen_valve_", 0) == 0)
        {
            const int valveIndex = interactable.name.back() - '1';
            if (valveIndex >= 0 && valveIndex < static_cast<int>(scene.oxygenValvePlacements.size()))
            {
                interactable.pos = glm::vec3(
                    scene.oxygenValvePlacements[valveIndex].position.x,
                    0.0f,
                    scene.oxygenValvePlacements[valveIndex].position.z);
            }
        }
        else if (interactable.name == "power_console")
        {
            interactable.pos = glm::vec3(
                scene.powerConsolePlacement.position.x,
                0.0f,
                scene.powerConsolePlacement.position.z);
        }
        else if (interactable.name == "storage_note")
        {
            const ModelPlacement& storageNoteContainer = scene.storageContainerFlatOpenPlacements.front();
            const glm::vec3 noteInteractOffset =
                rotateOffsetY(glm::vec3(0.0f, 0.0f, kStorageNoteInteractBackOffset), storageNoteContainer.rotationY);
            interactable.pos = glm::vec3(
                storageNoteContainer.position.x + noteInteractOffset.x,
                0.0f,
                storageNoteContainer.position.z + noteInteractOffset.z);
        }
        else if (interactable.name == "lab_decoder")
        {
            const ModelPlacement& labInteractTable = scene.labTableDisplayPlacements.front();
            const glm::vec3 tableInteractOffset =
                rotateOffsetY(glm::vec3(0.0f, 0.0f, kLabTableInteractForwardOffset), labInteractTable.rotationY);
            interactable.pos = glm::vec3(
                labInteractTable.position.x + tableInteractOffset.x,
                0.0f,
                labInteractTable.position.z + tableInteractOffset.z);
        }
        else if (interactable.name == "control_door")
        {
            interactable.pos = glm::vec3(
                -kRoomLargeHalfExtent,
                0.0f,
                0.0f);
        }
        else if (interactable.name == "control_terminal")
        {
            const glm::vec3 computerInteractOffset =
                rotateOffsetY(glm::vec3(0.0f, 0.0f, kControlComputerInteractOffset), scene.controlTerminalPlacement.rotationY);
            interactable.pos = glm::vec3(
                scene.controlTerminalPlacement.position.x + computerInteractOffset.x,
                0.0f,
                scene.controlTerminalPlacement.position.z + computerInteractOffset.z);
        }
    }

    addCorridorCollision(scene.corridorPlacements[0].position, true);
    addCorridorCollision(scene.corridorPlacements[1].position, true);
    addCorridorCollision(scene.corridorPlacements[2].position, false);
    addCorridorCollision(scene.corridorPlacements[3].position, false);
    addCorridorCollision(scene.corridorPlacements[4].position, true);

    for (const auto& gatePlacement : scene.gatePlacements)
        addGateFrameCollision(gatePlacement);
    for (const auto& gateDoorPlacement : scene.gateDoorPlacements)
        addGateDoorCollision(gateDoorPlacement);
    for (const auto& powerUnlockGatePlacement : scene.powerUnlockGatePlacements)
    {
        if (powerFixed)
            addGateFrameCollision(powerUnlockGatePlacement);
        else
            addGateDoorCollision(powerUnlockGatePlacement);
    }
    for (const auto& controlUnlockGatePlacement : scene.controlUnlockGatePlacements)
    {
        if (controlUnlocked)
            addGateFrameCollision(controlUnlockGatePlacement);
        else
            addGateDoorCollision(controlUnlockGatePlacement);
    }

    std::cout << "Room-large test enabled"
        << " | boxColliders=" << world.colliders.size()
        << " | circleColliders=" << world.circleColliders.size()
        << " | cylinderColliders=" << world.cylinderColliders.size()
        << " | horizontalCylinderColliders=" << world.horizontalCylinderColliders.size()
        << " | rooms=" << scene.roomPlacements.size()
        << " | corridors=" << scene.corridorPlacements.size()
        << " | gates=" << scene.gatePlacements.size()
        << " | gateDoors=" << scene.gateDoorPlacements.size()
        << " | powerUnlockGates=" << scene.powerUnlockGatePlacements.size()
        << " | controlUnlockGates=" << scene.controlUnlockGatePlacements.size()
        << " | beds=" << scene.bedPlacements.size()
        << "\n";
}
