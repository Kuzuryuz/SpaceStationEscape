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

struct WorldObject
{
    std::string id;
    glm::vec3 pos;
    glm::vec3 scale;
    glm::vec3 color;
    float rotationY = 0.0f;
    bool hasCollision;
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

struct Door
{
    std::string name;
    glm::vec3 center;
    glm::vec3 halfSize;
    glm::vec3 closedColor;
    glm::vec3 openColor;
    float rotationY = 0.0f;
    bool open = false;
};

class World
{
public:
    GameState* gameState = nullptr;

    std::vector<WorldObject> staticObjects;
    std::vector<BoxCollider> colliders;
    std::vector<CircleCollider> circleColliders;
    std::vector<Room> rooms;
    std::vector<Interactable> interactables;
    std::vector<Door> doors;

    void buildDefaultRoom();
    void rebuildColliders();

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
    void addObject(const std::string& id, glm::vec3 pos, glm::vec3 scale, glm::vec3 color, bool hasCollision, float rotationY = 0.0f);
    void addRoomFloor(const std::string& id, float minX, float maxX, float minZ, float maxZ, const glm::vec3& color);
    void addHorizontalWallRun(const std::string& idPrefix, float startX, float endX, float z, const glm::vec3& color);
    void addVerticalWallRun(const std::string& idPrefix, float x, float startZ, float endZ, const glm::vec3& color);
    void addCornerPiece(const std::string& id, float x, float z, const glm::vec3& color);
    void addDoor(const std::string& name, const glm::vec3& center, const glm::vec3& halfSize, float rotationY, bool open, const glm::vec3& closedColor, const glm::vec3& openColor);
    Door* findDoor(const std::string& name);
    static bool intersectsCircleBoxXZ(const glm::vec3& circleCenter, float radius, const BoxCollider& box);
    static bool intersectsCircleCircleXZ(const glm::vec3& aCenter, float aRadius, const glm::vec3& bCenter, float bRadius);
};
