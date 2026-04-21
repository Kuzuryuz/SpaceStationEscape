#include "world/TestRoomScene.h"

#include <glm/gtc/matrix_transform.hpp>

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
    const glm::vec3 kOxygenConsoleScale(1.4f, 1.5f, 0.9f);
    const glm::vec3 kOxygenConsoleColor(0.78f, 0.78f, 0.20f);
    const glm::vec3 kPowerConsoleScale(1.4f, 1.5f, 0.9f);
    const glm::vec3 kPowerConsoleColor(0.20f, 0.55f, 0.95f);

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

    glm::vec3 getPowerRoomCenter()
    {
        const float linkedRoomCenterOffset = 2.0f * (kRoomLargeHalfExtent + kCorridorJoinOffset);
        return glm::vec3(linkedRoomCenterOffset, kRoomLargeBaseY, 0.0f);
    }
}

TestRoomScene createTestRoomScene()
{
    TestRoomScene scene;
    const glm::vec3 oxygenRoomCenter = getOxygenRoomCenter();
    const glm::vec3 powerRoomCenter = getPowerRoomCenter();

    scene.roomPlacements = {
        { glm::vec3(0.0f, kRoomLargeBaseY, 0.0f), glm::vec3(1.0f), 0.0f, glm::vec3(1.0f) },
        { oxygenRoomCenter, glm::vec3(1.0f), 0.0f, glm::vec3(1.0f) },
        { powerRoomCenter, glm::vec3(1.0f), 0.0f, glm::vec3(1.0f) }
    };

    scene.corridorPlacements = {
        { glm::vec3(0.0f, kRoomLargeBaseY, -kRoomLargeHalfExtent - kCorridorJoinOffset), glm::vec3(1.0f), 90.0f, glm::vec3(1.0f) },
        { glm::vec3(0.0f, kRoomLargeBaseY, kRoomLargeHalfExtent + kCorridorJoinOffset), glm::vec3(1.0f), 90.0f, glm::vec3(1.0f) },
        { glm::vec3(-kRoomLargeHalfExtent - kCorridorJoinOffset, kRoomLargeBaseY, 0.0f), glm::vec3(1.0f), 0.0f, glm::vec3(1.0f) },
        { glm::vec3(kRoomLargeHalfExtent + kCorridorJoinOffset, kRoomLargeBaseY, 0.0f), glm::vec3(1.0f), 0.0f, glm::vec3(1.0f) }
    };

    scene.gatePlacements = {
        { glm::vec3(0.0f, kRoomLargeBaseY, -kRoomLargeHalfExtent), glm::vec3(1.0f), 0.0f, glm::vec3(1.0f) },
        { glm::vec3(kRoomLargeHalfExtent, kRoomLargeBaseY, 0.0f), glm::vec3(1.0f), 90.0f, glm::vec3(1.0f) },
        { oxygenRoomCenter + glm::vec3(0.0f, 0.0f, kRoomLargeHalfExtent), glm::vec3(1.0f), 180.0f, glm::vec3(1.0f) },
        { powerRoomCenter + glm::vec3(-kRoomLargeHalfExtent, 0.0f, 0.0f), glm::vec3(1.0f), -90.0f, glm::vec3(1.0f) }
    };

    scene.gateDoorPlacements = {
        { glm::vec3(-kRoomLargeHalfExtent, kRoomLargeBaseY, 0.0f), glm::vec3(1.0f), -90.0f, glm::vec3(1.0f) },
        { oxygenRoomCenter + glm::vec3(-kRoomLargeHalfExtent, 0.0f, 0.0f), glm::vec3(1.0f), -90.0f, glm::vec3(1.0f) },
        { oxygenRoomCenter + glm::vec3(kRoomLargeHalfExtent, 0.0f, 0.0f), glm::vec3(1.0f), 90.0f, glm::vec3(1.0f) },
        { powerRoomCenter + glm::vec3(0.0f, 0.0f, -kRoomLargeHalfExtent), glm::vec3(1.0f), 0.0f, glm::vec3(1.0f) },
        { powerRoomCenter + glm::vec3(0.0f, 0.0f, kRoomLargeHalfExtent), glm::vec3(1.0f), 180.0f, glm::vec3(1.0f) },
        { powerRoomCenter + glm::vec3(kRoomLargeHalfExtent, 0.0f, 0.0f), glm::vec3(1.0f), 90.0f, glm::vec3(1.0f) }
    };

    scene.powerUnlockGatePlacements = {
        { glm::vec3(0.0f, kRoomLargeBaseY, kRoomLargeHalfExtent), glm::vec3(1.0f), 180.0f, glm::vec3(1.0f) },
        { oxygenRoomCenter + glm::vec3(0.0f, 0.0f, -kRoomLargeHalfExtent), glm::vec3(1.0f), 0.0f, glm::vec3(1.0f) }
    };

    scene.oxygenConsolePlacement = {
        oxygenRoomCenter + glm::vec3(0.0f, 1.0f, 0.0f),
        kOxygenConsoleScale,
        0.0f,
        kOxygenConsoleColor
    };

    scene.powerConsolePlacement = {
        powerRoomCenter + glm::vec3(0.0f, 1.0f, 0.0f),
        kPowerConsoleScale,
        0.0f,
        kPowerConsoleColor
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

    return scene;
}

void configureTestRoomWorld(World& world, const TestRoomScene& scene, bool powerFixed)
{
    const glm::vec3 oxygenRoomCenter = getOxygenRoomCenter();
    const glm::vec3 powerRoomCenter = getPowerRoomCenter();

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
        "Power Room",
        powerRoomCenter + glm::vec3(-kRoomLargeHalfExtent, -100.0f, -kRoomLargeHalfExtent),
        powerRoomCenter + glm::vec3(kRoomLargeHalfExtent, 100.0f, kRoomLargeHalfExtent)
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
    addRoomCollision(powerRoomCenter, false, false, true, false);
    world.circleColliders.push_back({ kRoomLargePillarCenter, kRoomLargePillarRadius });
    world.circleColliders.push_back({
        oxygenRoomCenter + glm::vec3(kRoomLargePillarCenter.x, 0.0f, kRoomLargePillarCenter.z),
        kRoomLargePillarRadius
    });
    world.circleColliders.push_back({
        powerRoomCenter + glm::vec3(kRoomLargePillarCenter.x, 0.0f, kRoomLargePillarCenter.z),
        kRoomLargePillarRadius
    });

    for (const auto& bedPlacement : scene.bedPlacements)
    {
        world.colliders.push_back({
            glm::vec3(bedPlacement.position.x, bedPlacement.position.y + 0.175f * kBedScale, bedPlacement.position.z),
            glm::vec3(0.6f * kBedScale, 0.175f * kBedScale, 0.6f * kBedScale)
        });
    }

    world.colliders.push_back({
        scene.oxygenConsolePlacement.position,
        scene.oxygenConsolePlacement.scale * 0.5f
    });
    world.colliders.push_back({
        scene.powerConsolePlacement.position,
        scene.powerConsolePlacement.scale * 0.5f
    });

    for (auto& interactable : world.interactables)
    {
        if (interactable.name == "oxygen_console")
        {
            interactable.pos = glm::vec3(
                scene.oxygenConsolePlacement.position.x,
                0.0f,
                scene.oxygenConsolePlacement.position.z);
        }
        else if (interactable.name == "power_console")
        {
            interactable.pos = glm::vec3(
                scene.powerConsolePlacement.position.x,
                0.0f,
                scene.powerConsolePlacement.position.z);
        }
    }

    addCorridorCollision(scene.corridorPlacements[0].position, true);
    addCorridorCollision(scene.corridorPlacements[1].position, true);
    addCorridorCollision(scene.corridorPlacements[2].position, false);
    addCorridorCollision(scene.corridorPlacements[3].position, false);

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

    std::cout << "Room-large test enabled"
        << " | boxColliders=" << world.colliders.size()
        << " | circleColliders=" << world.circleColliders.size()
        << " | rooms=" << scene.roomPlacements.size()
        << " | corridors=" << scene.corridorPlacements.size()
        << " | gates=" << scene.gatePlacements.size()
        << " | gateDoors=" << scene.gateDoorPlacements.size()
        << " | powerUnlockGates=" << scene.powerUnlockGatePlacements.size()
        << " | beds=" << scene.bedPlacements.size()
        << "\n";
}
