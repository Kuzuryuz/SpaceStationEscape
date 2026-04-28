#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <functional>
#include "GameState.h"

struct BoxCollider
{
    glm::vec3 center;
    glm::vec3 halfSize;
};

struct CircleCollider
{
    glm::vec3 center;
    float radius = 0.0f;
};

struct CylinderCollider
{
    glm::vec3 center;
    float radius = 0.0f;
    float halfHeight = 0.0f;
};

struct HorizontalCylinderCollider
{
    glm::vec3 center;
    glm::vec3 axisXZ{ 0.0f, 0.0f, 1.0f };
    float halfLength = 0.0f;
    float radius = 0.0f;
};

struct Room
{
    std::string name;
    glm::vec3 min;
    glm::vec3 max;
    bool visited = false;
};

struct Interactable
{
    std::string name;
    glm::vec3 pos;
    float radius;
    std::function<void()> onInteract;
};

class World
{
public:
    GameState* gameState = nullptr;

    std::vector<BoxCollider> colliders;
    std::vector<CircleCollider> circleColliders;
    std::vector<CylinderCollider> cylinderColliders;
    std::vector<HorizontalCylinderCollider> horizontalCylinderColliders;
    std::vector<Room> rooms;
    std::vector<Interactable> interactables;

    void resetInteractables();

    bool collidesWithWorld(const glm::vec3& testPos, float playerRadius) const;
    bool collidesWithCamera(const glm::vec3& cameraPos, float cameraRadius) const;

    int getCurrentRoomIndex(const glm::vec3& playerPos) const;

    int getNearestInteractableIndex(const glm::vec3& playerPos) const;
    int getCurrentObjectiveInteractableIndex() const;
    int getNearestObjectiveInteractableIndex(const glm::vec3& playerPos) const;
    bool isNearCurrentObjectiveInteractable(const glm::vec3& playerPos) const;

    void tryInteract(const glm::vec3& playerPos);

    void setGameState(GameState* state);

private:
    int findInteractableIndexByName(const std::string& name) const;
    static bool intersectsCircleBoxXZ(const glm::vec3& circleCenter, float radius, const BoxCollider& box);
    static bool intersectsCircleCircleXZ(const glm::vec3& aCenter, float aRadius, const glm::vec3& bCenter, float bRadius);
};
