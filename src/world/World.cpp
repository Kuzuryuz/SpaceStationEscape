#include "World.h"

#include <glm/common.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace
{
    bool intersectsSphereBox(const glm::vec3& sphereCenter, float radius, const BoxCollider& box)
    {
        const glm::vec3 closestPoint = glm::clamp(
            sphereCenter,
            box.center - box.halfSize,
            box.center + box.halfSize
        );
        const glm::vec3 delta = sphereCenter - closestPoint;
        return glm::dot(delta, delta) <= radius * radius;
    }

    bool intersectsSphereCylinder(const glm::vec3& sphereCenter, float sphereRadius, const CylinderCollider& cylinder)
    {
        const float dx = sphereCenter.x - cylinder.center.x;
        const float dz = sphereCenter.z - cylinder.center.z;
        const float combinedRadius = sphereRadius + cylinder.radius;
        const bool overlapsXZ = (dx * dx + dz * dz) < (combinedRadius * combinedRadius);
        const bool overlapsY =
            sphereCenter.y + sphereRadius >= cylinder.center.y - cylinder.halfHeight &&
            sphereCenter.y - sphereRadius <= cylinder.center.y + cylinder.halfHeight;

        return overlapsXZ && overlapsY;
    }

    bool intersectsCircleHorizontalCylinderXZ(
        const glm::vec3& circleCenter,
        float circleRadius,
        const HorizontalCylinderCollider& cylinder)
    {
        const glm::vec3 axis = glm::normalize(glm::vec3(cylinder.axisXZ.x, 0.0f, cylinder.axisXZ.z));
        const glm::vec3 offset(circleCenter.x - cylinder.center.x, 0.0f, circleCenter.z - cylinder.center.z);
        const float axialDistance = std::abs(glm::dot(offset, axis));
        const glm::vec3 radialOffset = offset - axis * glm::dot(offset, axis);
        const float radialDistance = glm::length(radialOffset);
        const float outsideAxial = std::max(axialDistance - cylinder.halfLength, 0.0f);
        const float outsideRadial = std::max(radialDistance - cylinder.radius, 0.0f);

        return outsideAxial * outsideAxial + outsideRadial * outsideRadial < circleRadius * circleRadius;
    }

    bool intersectsSphereHorizontalCylinder(
        const glm::vec3& sphereCenter,
        float sphereRadius,
        const HorizontalCylinderCollider& cylinder)
    {
        const glm::vec3 axis = glm::normalize(glm::vec3(cylinder.axisXZ.x, 0.0f, cylinder.axisXZ.z));
        const glm::vec3 offset = sphereCenter - cylinder.center;
        const float axialDistance = std::abs(glm::dot(offset, axis));
        const glm::vec3 radialOffset = offset - axis * glm::dot(offset, axis);
        const float radialDistance = glm::length(radialOffset);
        const float outsideAxial = std::max(axialDistance - cylinder.halfLength, 0.0f);
        const float outsideRadial = std::max(radialDistance - cylinder.radius, 0.0f);

        return outsideAxial * outsideAxial + outsideRadial * outsideRadial < sphereRadius * sphereRadius;
    }

    int getOxygenValveIndex(const std::string& interactableName)
    {
        const std::string prefix = "oxygen_valve_";
        if (interactableName.rfind(prefix, 0) != 0)
            return -1;

        if (interactableName.size() != prefix.size() + 1)
            return -1;

        const char valveNumber = interactableName.back();
        if (valveNumber < '1' || valveNumber > '0' + GameState::kOxygenValveCount)
            return -1;

        return valveNumber - '1';
    }

    bool isInteractableCompleted(const GameState& state, const std::string& interactableName)
    {
        const int oxygenValveIndex = getOxygenValveIndex(interactableName);
        if (oxygenValveIndex != -1)
            return state.oxygenValvesOpened[oxygenValveIndex];

        if (interactableName == "power_console")
            return state.powerFixed;
        if (interactableName == "storage_note")
            return state.foundNote;
        if (interactableName == "lab_decoder")
            return state.hasCode;
        if (interactableName == "control_door")
            return state.controlUnlocked;
        if (interactableName == "control_terminal")
            return state.gameFinished;

        return false;
    }

    bool isInteractableUnlocked(const GameState& state, const std::string& interactableName)
    {
        if (getOxygenValveIndex(interactableName) != -1)
            return !state.oxygenFixed && !state.playerDied;

        if (interactableName == "power_console")
            return state.oxygenFixed;
        if (interactableName == "storage_note")
            return state.powerFixed && state.storageUnlocked;
        if (interactableName == "lab_decoder")
            return state.powerFixed && state.labUnlocked && state.foundNote;
        if (interactableName == "control_door")
            return true;
        if (interactableName == "control_terminal")
            return state.controlUnlocked;

        return true;
    }
}

void World::setGameState(GameState* state)
{
    gameState = state;
}

int World::findInteractableIndexByName(const std::string& name) const
{
    for (int i = 0; i < static_cast<int>(interactables.size()); ++i)
    {
        if (interactables[i].name == name)
            return i;
    }

    return -1;
}

void World::resetInteractables()
{
    interactables.clear();

    const std::array<glm::vec3, GameState::kOxygenValveCount> oxygenValvePositions = {
        glm::vec3(-2.5f, 0.0f, -7.45f),
        glm::vec3(0.0f, 0.0f, -7.45f),
        glm::vec3(2.5f, 0.0f, -7.45f)
    };
    const std::array<int, GameState::kOxygenValveCount> oxygenValveOrder = { 1, 2, 0 };

    for (int valveNumber = 0; valveNumber < GameState::kOxygenValveCount; ++valveNumber)
    {
        interactables.push_back({
            "oxygen_valve_" + std::to_string(valveNumber + 1),
            oxygenValvePositions[valveNumber],
            1.4f,
            [this, valveNumber, oxygenValveOrder]()
            {
                if (!gameState)
                    return;

                if (gameState->playerDied)
                {
                    return;
                }

                if (gameState->oxygenFixed)
                {
                    return;
                }

                if (gameState->oxygenValvesOpened[valveNumber])
                {
                    return;
                }

                const int expectedValve = oxygenValveOrder[gameState->oxygenValveProgress];
                if (valveNumber != expectedValve)
                {
                    gameState->oxygenPuzzleFailed = true;
                    gameState->playerDied = true;
                    gameState->gameFinished = true;
                    return;
                }

                gameState->oxygenValvesOpened[valveNumber] = true;
                ++gameState->oxygenValveProgress;

                if (gameState->oxygenValveProgress >= GameState::kOxygenValveCount)
                {
                    gameState->oxygenFixed = true;
                }
            }
        });
    }

    interactables.push_back({
        "oxygen_terminal",
        glm::vec3(2.2f, 0.0f, -10.0f),
        1.8f,
        [this]()
        {
            if (!gameState)
                return;

            if (gameState->playerDied)
            {
                return;
            }

            if (!gameState->oxygenFixed)
            {
                return;
            }

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
                return;
            }

            if (gameState->powerFixed)
            {
                return;
            }

        }
    });

    interactables.push_back({
        "storage_note",
        glm::vec3(0.0f, 0.0f, 8.0f),
        1.0f,
        [this]()
        {
            if (!gameState)
                return;

            if (!gameState->storageUnlocked)
            {
                return;
            }

            if (gameState->foundNote)
            {
                return;
            }

            gameState->foundNote = true;
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
                return;
            }

            if (!gameState->foundNote)
            {
                return;
            }

            if (gameState->hasCode)
            {
                return;
            }

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
                return;
            }

            if (gameState->controlUnlocked)
            {
                return;
            }

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
                return;
            }

            if (gameState->gameFinished)
            {
                return;
            }

            gameState->gameFinished = true;
        }
    });

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

bool World::intersectsCircleCircleXZ(const glm::vec3& aCenter, float aRadius, const glm::vec3& bCenter, float bRadius)
{
    const float dx = aCenter.x - bCenter.x;
    const float dz = aCenter.z - bCenter.z;
    const float combinedRadius = aRadius + bRadius;
    return (dx * dx + dz * dz) < (combinedRadius * combinedRadius);
}

bool World::collidesWithWorld(const glm::vec3& testPos, float playerRadius) const
{
    for (const auto& box : colliders)
    {
        if (intersectsCircleBoxXZ(testPos, playerRadius, box))
            return true;
    }

    for (const auto& circle : circleColliders)
    {
        if (intersectsCircleCircleXZ(testPos, playerRadius, circle.center, circle.radius))
            return true;
    }

    for (const auto& cylinder : cylinderColliders)
    {
        if (intersectsCircleCircleXZ(testPos, playerRadius, cylinder.center, cylinder.radius))
            return true;
    }

    for (const auto& cylinder : horizontalCylinderColliders)
    {
        if (intersectsCircleHorizontalCylinderXZ(testPos, playerRadius, cylinder))
            return true;
    }

    return false;
}

bool World::collidesWithCamera(const glm::vec3& cameraPos, float cameraRadius) const
{
    for (const auto& box : colliders)
    {
        if (intersectsSphereBox(cameraPos, cameraRadius, box))
            return true;
    }

    for (const auto& circle : circleColliders)
    {
        if (intersectsCircleCircleXZ(cameraPos, cameraRadius, circle.center, circle.radius))
            return true;
    }

    for (const auto& cylinder : cylinderColliders)
    {
        if (intersectsSphereCylinder(cameraPos, cameraRadius, cylinder))
            return true;
    }

    for (const auto& cylinder : horizontalCylinderColliders)
    {
        if (intersectsSphereHorizontalCylinder(cameraPos, cameraRadius, cylinder))
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
        if (gameState && isInteractableCompleted(*gameState, interactable.name))
            continue;
        if (gameState && !isInteractableUnlocked(*gameState, interactable.name))
            continue;

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

    if (gameState->playerDied)
        return -1;

    if (!gameState->oxygenFixed)
    {
        static const std::array<int, GameState::kOxygenValveCount> oxygenValveOrder = { 1, 2, 0 };
        return findInteractableIndexByName(
            "oxygen_valve_" + std::to_string(oxygenValveOrder[gameState->oxygenValveProgress] + 1));
    }
    if (!gameState->powerFixed) return findInteractableIndexByName("power_console");
    if (!gameState->foundNote) return findInteractableIndexByName("storage_note");
    if (!gameState->hasCode) return findInteractableIndexByName("lab_decoder");
    if (!gameState->controlUnlocked) return findInteractableIndexByName("control_door");
    if (!gameState->gameFinished) return findInteractableIndexByName("control_terminal");

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
    int nearestIndex = getNearestInteractableIndex(playerPos);
    if (nearestIndex == -1)
        return;

    interactables[nearestIndex].onInteract();
}
