#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <cmath>
#include <string>
#include <array>
#include <vector>
#include <memory>
#include <algorithm>
#include <random>

#include "graphics/AnimatedCharacter.h"
#include "graphics/AnimatedObjectPlayer.h"
#include "graphics/Shader.h"
#include "graphics/StaticModel.h"
#include "world/World.h"
#include "world/TestRoomScene.h"
#include "audio/AudioEngine.h"
#include "GameState.h"

static const unsigned int SCR_WIDTH = 1280;
static const unsigned int SCR_HEIGHT = 720;
static const std::string kControlDoorCode = "0427";
static const std::array<int, 3> kLabStabilizerTarget{ 2, 7, 3 };
static const std::string kMainPowerDownSubtitle = "....MAIN....POWER....DOWN.....";
static const std::array<std::string, 2> kCodeTypingClipIds{ "code_typing_1", "code_typing_2" };
static const std::string kArrowUpClipId = "arrow_up";
static const std::string kArrowDownClipId = "arrow_down";
static const std::string kConfirmationClipId = "confirmation";
static const std::string kErrorClipId = "error";
static const std::string kOpenClipId = "open";
static const std::string kCloseClipId = "close";
static const std::string kCutClipId = "cut";
static const std::string kDoorOpenClipId = "door_open";
static const std::string kStartClipId = "start";
static const std::string kPickupClipId = "pickup";
static const std::string kWinClipId = "win";
static const std::string kLoseClipId = "lose";
static const std::string kPowerShutdownClipId = "power_shutdown";
static const std::string kBeepClipId = "beep";
static const std::string kGasClipId = "gas";
static const std::string kTurnValveClipId = "turn_valve";
static const std::string kSpaceStationLoopId = "space_station_bg";
static const std::string kHeavyBreathingLoopId = "heavy_breathing_loop";
static const std::array<std::string, 5> kFootstepConcreteClipIds{
    "footstep_concrete_000",
    "footstep_concrete_001",
    "footstep_concrete_002",
    "footstep_concrete_003",
    "footstep_concrete_004"
};

enum PowerWireId
{
    kPowerWireRed = 0,
    kPowerWireGreen = 1,
    kPowerWireYellow = 2,
    kPowerWireBlue = 3,
    kPowerWirePink = 4
};

int screenWidth = SCR_WIDTH;
int screenHeight = SCR_HEIGHT;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

glm::vec3 playerPos(0.0f, 0.0f, 0.0f);
float walkSpeed = 3.5f;
float runSpeed = 7.0f;
float playerRadius = 0.35f;
float playerYaw = 90.0f;

float cameraYaw = -90.0f;
float cameraPitch = -20.0f;
float cameraDistance = 4.5f;
float currentCameraDistance = cameraDistance;
float cameraHeightOffset = 1.5f;
float cameraCollisionRadius = 0.18f;
float cameraCollisionSkin = 0.08f;
float cameraMinHeight = 0.35f;
float mouseSensitivity = 0.12f;

bool firstMouse = true;
float lastMouseX = SCR_WIDTH * 0.5f;
float lastMouseY = SCR_HEIGHT * 0.5f;

bool ePressedLastFrame = false;
bool onePressedLastFrame = false;
bool f3PressedLastFrame = false;
bool f11PressedLastFrame = false;
bool enterPressedLastFrame = false;
bool spacePressedLastFrame = false;
bool backspacePressedLastFrame = false;
bool escapePressedLastFrame = false;
bool upPressedLastFrame = false;
bool downPressedLastFrame = false;
bool leftPressedLastFrame = false;
bool rightPressedLastFrame = false;
bool rPressedLastFrame = false;
std::array<bool, 10> digitPressedLastFrame{ false, false, false, false, false, false, false, false, false, false };
bool showCollisionDebug = false;
bool fullscreenEnabled = false;
int windowedX = 100;
int windowedY = 100;
int windowedWidth = SCR_WIDTH;
int windowedHeight = SCR_HEIGHT;
bool gameStarted = false;
bool showInteractPrompt = false;
int nearestInteractableIndex = -1;
bool playerIsMoving = false;
bool playerIsRunning = false;
bool playerDanceTriggered = false;
bool playerIsDancing = false;
bool deathAnimationTriggered = false;
bool deathAnimationFinished = false;
bool controlCodePanelOpen = false;
bool controlCodeRejected = false;
std::string controlCodeInput = "";
bool oxygenTerminalPanelOpen = false;
bool labStabilizerPanelOpen = false;
bool labStabilizerRejected = false;
std::array<int, 3> labStabilizerValues{ 0, 0, 0 };
int labStabilizerSelected = 0;
bool powerWirePanelOpen = false;
bool powerWireResolving = false;
bool powerWireCutWasCorrect = false;
std::array<bool, GameState::kPowerWireCount> powerWireCut{ false, false, false, false, false };
std::vector<int> powerWireCutOrder;
int powerWireSelected = 0;
float powerWireResolveTimer = 0.0f;
float footstepTimer = 0.0f;
float powerWarningBeepTimer = 0.0f;
bool powerWarningBeepRapid = false;

enum class PlayerAnimationState
{
    Idle,
    Walk,
    Run,
    Dance,
    Die
};

AnimatedCharacter* getCharacterForState(
    PlayerAnimationState state,
    AnimatedCharacter& idleCharacter,
    AnimatedCharacter& walkCharacter,
    AnimatedCharacter& runCharacter,
    AnimatedCharacter& danceCharacter,
    AnimatedCharacter& deathCharacter)
{
    switch (state)
    {
    case PlayerAnimationState::Walk:
        return walkCharacter.isLoaded() ? &walkCharacter : nullptr;
    case PlayerAnimationState::Run:
        return runCharacter.isLoaded() ? &runCharacter : nullptr;
    case PlayerAnimationState::Dance:
        return danceCharacter.isLoaded() ? &danceCharacter : nullptr;
    case PlayerAnimationState::Die:
        return deathCharacter.isLoaded() ? &deathCharacter : nullptr;
    case PlayerAnimationState::Idle:
    default:
        return idleCharacter.isLoaded() ? &idleCharacter : nullptr;
    }
}

World world;
GameState gameState;
AudioEngine audio;

struct SubtitleLine
{
    std::string text;
    float duration;
};

std::vector<SubtitleLine> subtitleQueue;
std::string currentSubtitle = "";
float subtitleTimer = 0.0f;

bool introQueued = false;
bool seenOxygenFixed = false;
bool seenPowerFixed = false;
bool seenFoundNote = false;
bool seenHasCode = false;
bool seenControlUnlocked = false;
bool seenGameFinished = false;
bool seenPlayerDeath = false;
bool endingSoundPlayed = false;
bool heavyBreathingPlaying = false;
float heavyBreathingVolume = 0.55f;
bool powerWarningBeepActive = false;

void playRandomCodeTypingSound()
{
    constexpr float kCodeTypingVolume = 0.70f;
    static std::mt19937 rng(std::random_device{}());
    static std::uniform_int_distribution<int> soundIndex(0, 1);
    static const std::array<std::string, 2> typingSounds{
        kCodeTypingClipIds[0],
        kCodeTypingClipIds[1]
    };

    audio.playClip(typingSounds[soundIndex(rng)], kCodeTypingVolume);
}

void playCodeDeleteSound()
{
    constexpr float kCodeDeleteVolume = 1.0f;
    static const std::string deleteSound = std::string(PROJECT_ROOT) + "/assets/audio/delete.wav";
    audio.playOneShot(deleteSound, kCodeDeleteVolume);
}

void playTypingSound(int typingIndex)
{
    constexpr float kTypingVolume = 0.70f;
    if (typingIndex < 0 || typingIndex >= static_cast<int>(kCodeTypingClipIds.size()))
        return;

    audio.playClip(kCodeTypingClipIds[typingIndex], kTypingVolume);
}

void playArrowUpSound()
{
    constexpr float kArrowVolume = 0.85f;
    audio.playClip(kArrowUpClipId, kArrowVolume);
}

void playArrowDownSound()
{
    constexpr float kArrowVolume = 0.85f;
    audio.playClip(kArrowDownClipId, kArrowVolume);
}

void playConfirmationSound()
{
    constexpr float kConfirmationVolume = 0.85f;
    audio.playClip(kConfirmationClipId, kConfirmationVolume);
}

void playErrorSound()
{
    constexpr float kErrorVolume = 0.85f;
    audio.playClip(kErrorClipId, kErrorVolume);
}

void playOpenSound()
{
    constexpr float kOpenVolume = 0.4f;
    audio.playClip(kOpenClipId, kOpenVolume);
}

void playCloseSound()
{
    constexpr float kCloseVolume = 0.4f;
    audio.playClip(kCloseClipId, kCloseVolume);
}

void playCutSound()
{
    constexpr float kCutVolume = 0.25f;
    audio.playClip(kCutClipId, kCutVolume);
}

void playDoorOpenSound()
{
    constexpr float kDoorOpenVolume = 0.6f;
    audio.playClip(kDoorOpenClipId, kDoorOpenVolume);
}

void playStartSound()
{
    constexpr float kStartVolume = 0.7f;
    audio.playClip(kStartClipId, kStartVolume);
}

void playPickupSound()
{
    constexpr float kPickupVolume = 0.7f;
    audio.playClip(kPickupClipId, kPickupVolume);
}

void playWinSound()
{
    constexpr float kWinVolume = 0.7f;
    audio.playClip(kWinClipId, kWinVolume);
}

void playLoseSound()
{
    constexpr float kLoseVolume = 0.55f;
    audio.playClip(kLoseClipId, kLoseVolume);
}

void playPowerShutdownSound()
{
    constexpr float kPowerShutdownVolume = 0.75f;
    audio.playClip(kPowerShutdownClipId, kPowerShutdownVolume);
}

void playBeepSound()
{
    constexpr float kBeepVolume = 0.45f;
    audio.playClip(kBeepClipId, kBeepVolume);
}

void playGasSound()
{
    constexpr float kGasVolume = 0.85f;
    audio.playClip(kGasClipId, kGasVolume);
}

void playTurnValveSound()
{
    constexpr float kTurnValveVolume = 0.8f;
    audio.playClip(kTurnValveClipId, kTurnValveVolume);
}

void startHeavyBreathingLoop()
{
    if (heavyBreathingPlaying || gameState.oxygenFixed)
        return;

    if (audio.playLoop(kHeavyBreathingLoopId, std::string(PROJECT_ROOT) + "/assets/audio/heavy_breathing.wav", heavyBreathingVolume))
        heavyBreathingPlaying = true;
}

void stopHeavyBreathingLoop()
{
    if (!heavyBreathingPlaying)
    {
        heavyBreathingVolume = 0.55f;
        return;
    }

    audio.stopLoop(kHeavyBreathingLoopId);
    heavyBreathingPlaying = false;
    heavyBreathingVolume = 0.55f;
}

void playFootstepSound()
{
    constexpr float kFootstepVolume = 0.35f;
    static size_t selectedIndex = 0;

    audio.playClip(kFootstepConcreteClipIds[selectedIndex], kFootstepVolume);
    selectedIndex = (selectedIndex + 1) % kFootstepConcreteClipIds.size();
}

void updateFootstepSounds()
{
    if (!playerIsMoving || playerIsDancing || gameState.playerDied || gameState.gameFinished)
    {
        footstepTimer = 0.0f;
        return;
    }

    const float footstepInterval = playerIsRunning ? 0.3f : 0.54f;
    footstepTimer -= deltaTime;
    if (footstepTimer <= 0.0f)
    {
        playFootstepSound();
        footstepTimer = footstepInterval;
    }
}

void unlockControlRoomFromCode()
{
    gameState.controlUnlocked = true;
    controlCodePanelOpen = false;
    controlCodeRejected = false;
    controlCodeInput.clear();

    for (auto& door : world.doors)
    {
        if (door.name == "Control Door")
        {
            door.open = true;
            break;
        }
    }
    playDoorOpenSound();

    std::cout << "Control room unlocked\n";
}

void startPowerWarningBeeps()
{
    powerWarningBeepActive = true;
    powerWarningBeepRapid = false;
    powerWarningBeepTimer = 1.6f;
}

void startPowerFailureBeeps()
{
    powerWarningBeepActive = true;
    powerWarningBeepRapid = true;
    powerWarningBeepTimer = 0.0f;
}

void stopPowerWarningBeeps()
{
    powerWarningBeepActive = false;
    powerWarningBeepRapid = false;
    powerWarningBeepTimer = 0.0f;
}

void updatePowerWarningBeeps()
{
    if (!powerWarningBeepActive)
        return;

    if (powerWarningBeepRapid)
    {
        if (!gameState.powerPuzzleFailed || deathAnimationTriggered)
        {
            stopPowerWarningBeeps();
            return;
        }

        powerWarningBeepTimer -= deltaTime;
        if (powerWarningBeepTimer <= 0.0f)
        {
            playBeepSound();
            powerWarningBeepTimer = 0.5f;
        }
        return;
    }

    if (gameState.powerFixed || gameState.gameFinished || gameState.playerDied)
        return;

    powerWarningBeepTimer -= deltaTime;
    if (powerWarningBeepTimer <= 0.0f)
    {
        playBeepSound();
        powerWarningBeepTimer = 4.0f;
    }
}

void completeLabStabilization()
{
    gameState.hasCode = true;
    labStabilizerPanelOpen = false;
    labStabilizerRejected = false;
    std::cout << "AI: Trace markings decoded\n";
}

void openPowerWirePanel()
{
    powerWirePanelOpen = true;
    powerWireResolving = false;
    powerWireCutWasCorrect = false;
    powerWireCut.fill(false);
    powerWireCutOrder.clear();
    powerWireSelected = 0;
    powerWireResolveTimer = 0.0f;
}

void closePowerWirePanel()
{
    powerWirePanelOpen = false;
    powerWireResolving = false;
    powerWireCutWasCorrect = false;
    powerWireCut.fill(false);
    powerWireCutOrder.clear();
    powerWireResolveTimer = 0.0f;
}

bool isValidPowerWireOrderPrefix(const std::vector<int>& order)
{
    static const std::array<int, GameState::kPowerWireCount> kExpectedOrder{
        kPowerWireBlue,
        kPowerWireRed,
        kPowerWirePink,
        kPowerWireGreen,
        kPowerWireYellow
    };

    if (order.size() > kExpectedOrder.size())
        return false;

    for (size_t i = 0; i < order.size(); ++i)
    {
        if (order[i] != kExpectedOrder[i])
            return false;
    }

    return true;
}

void completePowerRestore()
{
    gameState.powerFixed = true;
    gameState.powerPuzzleFailed = false;
    gameState.storageUnlocked = true;
    gameState.labUnlocked = true;
    closePowerWirePanel();

    for (auto& door : world.doors)
    {
        if (door.name == "Storage Door" || door.name == "Lab Door")
            door.open = true;
    }
    playDoorOpenSound();

    std::cout << "AI: Main power restored\n";
    std::cout << "AI: Storage and Lab access online\n";
}

void failPowerPuzzle()
{
    gameState.powerPuzzleFailed = true;
    gameState.playerDied = true;
    gameState.gameFinished = true;
    startPowerFailureBeeps();
    closePowerWirePanel();
    std::cout << "AI: Incorrect wire cut detected\n";
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    screenWidth = width > 0 ? width : 1;
    screenHeight = height > 0 ? height : 1;
    glViewport(0, 0, screenWidth, screenHeight);
}

void toggleFullscreen(GLFWwindow* window)
{
    if (!window)
        return;

    fullscreenEnabled = !fullscreenEnabled;
    if (fullscreenEnabled)
    {
        glfwGetWindowPos(window, &windowedX, &windowedY);
        glfwGetWindowSize(window, &windowedWidth, &windowedHeight);

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = monitor ? glfwGetVideoMode(monitor) : nullptr;
        if (!monitor || !mode)
        {
            fullscreenEnabled = false;
            return;
        }

        if (monitor && mode)
        {
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            screenWidth = mode->width;
            screenHeight = mode->height;
            glViewport(0, 0, screenWidth, screenHeight);
        }
    }
    else
    {
        glfwSetWindowMonitor(window, nullptr, windowedX, windowedY, windowedWidth, windowedHeight, 0);
        screenWidth = windowedWidth;
        screenHeight = windowedHeight;
        glViewport(0, 0, screenWidth, screenHeight);
    }
}

void handleFullscreenToggle(GLFWwindow* window)
{
    const bool f11PressedNow = glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS;
    if (f11PressedNow && !f11PressedLastFrame)
        toggleFullscreen(window);
    f11PressedLastFrame = f11PressedNow;
}

glm::vec3 getCameraForward3D()
{
    glm::vec3 forward;
    forward.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
    forward.y = sin(glm::radians(cameraPitch));
    forward.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
    return glm::normalize(forward);
}

glm::vec3 getCameraForwardXZ()
{
    glm::vec3 forward;
    forward.x = cos(glm::radians(cameraYaw));
    forward.y = 0.0f;
    forward.z = sin(glm::radians(cameraYaw));

    if (glm::length(forward) < 0.0001f)
        return glm::vec3(0.0f, 0.0f, -1.0f);

    return glm::normalize(forward);
}

glm::vec3 getCameraRightXZ()
{
    glm::vec3 forward = getCameraForwardXZ();
    return glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
}

float resolveCameraCollisionDistance(const glm::vec3& cameraTarget, const glm::vec3& cameraBackward, float desiredDistance)
{
    constexpr int kCameraCollisionSweepSteps = 24;
    constexpr int kCameraCollisionRefineSteps = 8;

    float clearDistance = 0.0f;
    float blockedDistance = desiredDistance;
    bool foundBlocker = false;

    for (int i = 1; i <= kCameraCollisionSweepSteps; i++)
    {
        const float testDistance = desiredDistance * static_cast<float>(i) / static_cast<float>(kCameraCollisionSweepSteps);
        const glm::vec3 testPos = cameraTarget + cameraBackward * testDistance;

        if (world.collidesWithCamera(testPos, cameraCollisionRadius))
        {
            blockedDistance = testDistance;
            foundBlocker = true;
            break;
        }

        clearDistance = testDistance;
    }

    if (!foundBlocker)
        return desiredDistance;

    for (int i = 0; i < kCameraCollisionRefineSteps; i++)
    {
        const float testDistance = (clearDistance + blockedDistance) * 0.5f;
        const glm::vec3 testPos = cameraTarget + cameraBackward * testDistance;

        if (world.collidesWithCamera(testPos, cameraCollisionRadius))
            blockedDistance = testDistance;
        else
            clearDistance = testDistance;
    }

    return glm::max(0.0f, clearDistance - cameraCollisionSkin);
}

void tryMovePlayer(glm::vec3 moveDelta)
{
    glm::vec3 testPosX = playerPos + glm::vec3(moveDelta.x, 0.0f, 0.0f);
    if (!world.collidesWithWorld(testPosX, playerRadius))
        playerPos.x = testPosX.x;

    glm::vec3 testPosZ = playerPos + glm::vec3(0.0f, 0.0f, moveDelta.z);
    if (!world.collidesWithWorld(testPosZ, playerRadius))
        playerPos.z = testPosZ.z;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastMouseX = (float)xpos;
        lastMouseY = (float)ypos;
        firstMouse = false;
    }

    float xoffset = (float)xpos - lastMouseX;
    float yoffset = lastMouseY - (float)ypos;

    lastMouseX = (float)xpos;
    lastMouseY = (float)ypos;

    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    cameraYaw += xoffset;
    cameraPitch += yoffset;

    if (cameraPitch > 70.0f) cameraPitch = 70.0f;
    if (cameraPitch < -60.0f) cameraPitch = -60.0f;
}

void processInput(GLFWwindow* window)
{
    if (powerWirePanelOpen && powerWireResolving)
    {
        powerWireResolveTimer -= deltaTime;
        if (powerWireResolveTimer <= 0.0f)
        {
            if (powerWireCutWasCorrect)
                completePowerRestore();
            else
                failPowerPuzzle();
        }
    }

    const bool escapePressedNow = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    if (escapePressedNow && !escapePressedLastFrame)
    {
        if (controlCodePanelOpen)
        {
            controlCodePanelOpen = false;
            controlCodeRejected = false;
            controlCodeInput.clear();
            playCloseSound();
        }
        else if (oxygenTerminalPanelOpen)
        {
            oxygenTerminalPanelOpen = false;
            playCloseSound();
        }
        else if (labStabilizerPanelOpen)
        {
            labStabilizerPanelOpen = false;
            labStabilizerRejected = false;
            playCloseSound();
        }
        else if (powerWirePanelOpen && !powerWireResolving)
        {
            closePowerWirePanel();
        }
        else
        {
            glfwSetWindowShouldClose(window, true);
        }
    }
    escapePressedLastFrame = escapePressedNow;

    const bool f3PressedNow = glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS;
    if (f3PressedNow && !f3PressedLastFrame)
    {
        showCollisionDebug = !showCollisionDebug;
        std::cout << "Collision debug " << (showCollisionDebug ? "enabled" : "disabled") << "\n";
    }
    f3PressedLastFrame = f3PressedNow;

    const bool onePressedNow = glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS;
    playerDanceTriggered = onePressedNow && !onePressedLastFrame;
    onePressedLastFrame = onePressedNow;

    if (controlCodePanelOpen)
    {
        for (int digit = 0; digit < 10; ++digit)
        {
            const bool digitPressedNow = glfwGetKey(window, GLFW_KEY_0 + digit) == GLFW_PRESS ||
                glfwGetKey(window, GLFW_KEY_KP_0 + digit) == GLFW_PRESS;
            if (digitPressedNow && !digitPressedLastFrame[digit] && controlCodeInput.size() < kControlDoorCode.size())
            {
                controlCodeInput.push_back(static_cast<char>('0' + digit));
                controlCodeRejected = false;
                playRandomCodeTypingSound();
            }
            digitPressedLastFrame[digit] = digitPressedNow;
        }

        const bool backspacePressedNow = glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS;
        if (backspacePressedNow && !backspacePressedLastFrame && !controlCodeInput.empty())
        {
            controlCodeInput.pop_back();
            controlCodeRejected = false;
            playCodeDeleteSound();
        }
        backspacePressedLastFrame = backspacePressedNow;

        const bool enterPressedNow = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_KP_ENTER) == GLFW_PRESS;
        if (enterPressedNow && !enterPressedLastFrame && controlCodeInput.size() == kControlDoorCode.size())
        {
            if (controlCodeInput == kControlDoorCode)
            {
                playConfirmationSound();
                unlockControlRoomFromCode();
            }
            else
            {
                playErrorSound();
                controlCodeRejected = true;
                controlCodeInput.clear();
                std::cout << "Wrong control room code\n";
            }
        }
        enterPressedLastFrame = enterPressedNow;

        playerIsMoving = false;
        playerIsRunning = false;
        playerDanceTriggered = false;
        return;
    }

    if (oxygenTerminalPanelOpen)
    {
        playerIsMoving = false;
        playerIsRunning = false;
        playerDanceTriggered = false;
        return;
    }

    if (labStabilizerPanelOpen)
    {
        const bool upPressedNow = glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS;
        const bool downPressedNow = glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS;
        const bool leftPressedNow = glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS;
        const bool rightPressedNow = glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS;

        if (upPressedNow && !upPressedLastFrame)
        {
            labStabilizerSelected = (labStabilizerSelected + 2) % 3;
            playArrowUpSound();
        }
        if (downPressedNow && !downPressedLastFrame)
        {
            labStabilizerSelected = (labStabilizerSelected + 1) % 3;
            playArrowDownSound();
        }
        if (leftPressedNow && !leftPressedLastFrame)
        {
            labStabilizerValues[labStabilizerSelected] = std::max(0, labStabilizerValues[labStabilizerSelected] - 1);
            labStabilizerRejected = false;
            playTypingSound(1);
        }
        if (rightPressedNow && !rightPressedLastFrame)
        {
            labStabilizerValues[labStabilizerSelected] = std::min(9, labStabilizerValues[labStabilizerSelected] + 1);
            labStabilizerRejected = false;
            playTypingSound(0);
        }

        upPressedLastFrame = upPressedNow;
        downPressedLastFrame = downPressedNow;
        leftPressedLastFrame = leftPressedNow;
        rightPressedLastFrame = rightPressedNow;

        const bool enterPressedNow = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_KP_ENTER) == GLFW_PRESS;
        if (enterPressedNow && !enterPressedLastFrame)
        {
            if (labStabilizerValues == kLabStabilizerTarget)
            {
                playConfirmationSound();
                completeLabStabilization();
            }
            else
            {
                playErrorSound();
                labStabilizerRejected = true;
            }
        }
        enterPressedLastFrame = enterPressedNow;

        playerIsMoving = false;
        playerIsRunning = false;
        playerDanceTriggered = false;
        return;
    }

    if (powerWirePanelOpen)
    {
        const bool upPressedNow = glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS;
        const bool downPressedNow = glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS;

        if (!powerWireResolving)
        {
            if (upPressedNow && !upPressedLastFrame)
            {
                powerWireSelected = (powerWireSelected + GameState::kPowerWireCount - 1) % GameState::kPowerWireCount;
                playArrowUpSound();
            }
            if (downPressedNow && !downPressedLastFrame)
            {
                powerWireSelected = (powerWireSelected + 1) % GameState::kPowerWireCount;
                playArrowDownSound();
            }
        }

        upPressedLastFrame = upPressedNow;
        downPressedLastFrame = downPressedNow;
        leftPressedLastFrame = false;
        rightPressedLastFrame = false;

        const bool enterPressedNow = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_KP_ENTER) == GLFW_PRESS;
        if (!powerWireResolving && enterPressedNow && !enterPressedLastFrame)
        {
            if (!powerWireCut[powerWireSelected])
            {
                powerWireCut[powerWireSelected] = true;
                powerWireCutOrder.push_back(powerWireSelected);
                playCutSound();

                if (!isValidPowerWireOrderPrefix(powerWireCutOrder))
                {
                    powerWireCutWasCorrect = false;
                    powerWireResolving = true;
                    powerWireResolveTimer = 0.95f;
                }
                else if (powerWireCutOrder.size() == GameState::kPowerWireCount)
                {
                    playConfirmationSound();
                    powerWireCutWasCorrect = true;
                    powerWireResolving = true;
                    powerWireResolveTimer = 0.75f;
                }
                else
                {
                    for (int i = 1; i <= GameState::kPowerWireCount; ++i)
                    {
                        const int nextIndex = (powerWireSelected + i) % GameState::kPowerWireCount;
                        if (!powerWireCut[nextIndex])
                        {
                            powerWireSelected = nextIndex;
                            break;
                        }
                    }
                }
            }
        }
        enterPressedLastFrame = enterPressedNow;

        playerIsMoving = false;
        playerIsRunning = false;
        playerDanceTriggered = false;
        return;
    }

    if (gameState.gameFinished || gameState.playerDied)
    {
        playerIsMoving = false;
        playerIsRunning = false;
        return;
    }

    if (playerIsDancing)
    {
        playerIsMoving = false;
        playerIsRunning = false;
        return;
    }

    const bool shiftHeld =
        glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    const float currentSpeed = shiftHeld ? runSpeed : walkSpeed;
    float moveAmount = currentSpeed * deltaTime;
    glm::vec3 forward = getCameraForwardXZ();
    glm::vec3 right = getCameraRightXZ();

    glm::vec3 moveDelta(0.0f);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        moveDelta += forward * moveAmount;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        moveDelta -= forward * moveAmount;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        moveDelta -= right * moveAmount;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        moveDelta += right * moveAmount;

    if (glm::length(moveDelta) > 0.0f)
    {
        playerIsMoving = true;
        playerIsRunning = shiftHeld;
        moveDelta = glm::normalize(moveDelta) * moveAmount;
        tryMovePlayer(moveDelta);
        playerYaw = glm::degrees(std::atan2(-moveDelta.z, moveDelta.x));
    }
    else
    {
        playerIsMoving = false;
        playerIsRunning = false;
    }
}

void queueSubtitle(const std::string& text, float duration = 3.5f)
{
    subtitleQueue.push_back({ text, duration });
}

void interruptSubtitles()
{
    subtitleQueue.clear();
    currentSubtitle.clear();
    subtitleTimer = 0.0f;
}

void updateSubtitles()
{
    if (currentSubtitle.empty() && !subtitleQueue.empty())
    {
        currentSubtitle = subtitleQueue.front().text;
        subtitleTimer = subtitleQueue.front().duration;
        subtitleQueue.erase(subtitleQueue.begin());

        if (currentSubtitle == kMainPowerDownSubtitle)
        {
            playPowerShutdownSound();
            startPowerWarningBeeps();
        }
    }

    if (!currentSubtitle.empty())
    {
        subtitleTimer -= deltaTime;
        if (subtitleTimer <= 0.0f)
        {
            currentSubtitle.clear();
            subtitleTimer = 0.0f;
        }
    }
}

void updateStoryEvents()
{
    if (!introQueued)
    {
        introQueued = true;
        queueSubtitle("EMERGENCY AI PROTOCOL ACTIVATED", 3.0f);
        queueSubtitle("A METEOR HAS STRUCK THE SPACESHIP", 3.4f);
        queueSubtitle("ACCESS TO THE CONTROL ROOM IS REQUIRED TO STABILIZE THE SHIP AND OPEN THE ESCAPE ROUTE", 5.0f);
        queueSubtitle("RUNNING DAMAGE DIAGNOSTICS...", 2.8f);
        queueSubtitle("LIFE SUPPORT FAILURE DETECTED", 3.0f);
        queueSubtitle("THE OXYGEN ROOM WAS DAMAGED BY THE IMPACT", 3.8f);
        queueSubtitle("REPAIR IT BEFORE THE CREW SUFFOCATES", 3.6f);
    }

    if (gameState.oxygenFixed && !seenOxygenFixed)
    {
        seenOxygenFixed = true;
        playGasSound();
        stopHeavyBreathingLoop();
        interruptSubtitles();
        queueSubtitle("OXYGEN FLOW STABILIZED", 2.8f);
        queueSubtitle("RUNNING DAMAGE DIAGNOSTICS...", 2.8f);
        queueSubtitle(kMainPowerDownSubtitle, 3.2f);
        queueSubtitle("......RESTORE....POWER....", 3.2f);
    }

    if (gameState.powerFixed && !seenPowerFixed)
    {
        seenPowerFixed = true;
        stopPowerWarningBeeps();
        interruptSubtitles();
        queueSubtitle("MAIN POWER RESTORED", 2.8f);
        queueSubtitle("SYSTEMS REBOOTING...", 2.8f);
        queueSubtitle("RUNNING DAMAGE DIAGNOSTICS...", 2.8f);
        queueSubtitle("NO ADDITIONAL DAMAGE DETECTED", 3.2f);
        queueSubtitle("RUNNING ACCESS CHECK...", 2.8f);
        queueSubtitle("STORAGE ACCESS ONLINE", 2.8f);
        queueSubtitle("LAB ACCESS ONLINE", 2.8f);
        queueSubtitle("CONTROL ROOM ACCESS DENIED", 3.2f);
    }

    if (gameState.foundNote && !seenFoundNote)
    {
        seenFoundNote = true;
        interruptSubtitles();
        queueSubtitle("DOCUMENT RECOVERED", 2.6f);
        queueSubtitle("SCANNING SURFACE...", 2.8f);
        queueSubtitle("NO VISIBLE TEXT DETECTED", 3.0f);
        queueSubtitle("TRACE MARKINGS FOUND", 2.8f);
        queueSubtitle("LAB ANALYSIS REQUIRED", 3.0f);
    }

    if (gameState.hasCode && !seenHasCode)
    {
        seenHasCode = true;
        interruptSubtitles();
        queueSubtitle("DOCUMENT ANALYSIS...", 2.8f);
        queueSubtitle("ANALYSIS COMPLETE", 2.8f);
        queueSubtitle("TRACE MARKINGS DECODED", 3.0f);
        queueSubtitle("CONTROL CODE " + kControlDoorCode, 3.2f);
    }

    if (gameState.controlUnlocked && !seenControlUnlocked)
    {
        seenControlUnlocked = true;
        interruptSubtitles();
        queueSubtitle("CONTROL ROOM ACCESS GRANTED", 2.7f);
        queueSubtitle("COMMAND ACCESS RESTORED", 2.8f);
        queueSubtitle("AUTHORIZE ESCAPE ROUTE FROM THE TERMINAL", 3.6f);
    }

    if (gameState.gameFinished && !gameState.playerDied && !seenGameFinished)
    {
        seenGameFinished = true;
        interruptSubtitles();
        queueSubtitle("AUTHORIZING...", 2.4f);
        queueSubtitle("ESCAPE ROUTE AUTHORIZED", 2.8f);
        queueSubtitle("EVACUATION PATH OPEN", 2.8f);
        queueSubtitle("CREW EVACUATION SUCCESSFUL", 3.0f);
    }

    if (gameState.playerDied && !seenPlayerDeath)
    {
        seenPlayerDeath = true;
        deathAnimationTriggered = false;
        deathAnimationFinished = false;
        interruptSubtitles();
        if (gameState.powerPuzzleFailed)
        {
            queueSubtitle("INCORRECT WIRE CUT DETECTED", 3.0f);
            queueSubtitle("POWER CASCADE TRIGGERED", 2.8f);
            queueSubtitle("CRITICAL SYSTEMS OFFLINE", 3.0f);
        }
        else
        {
            queueSubtitle("INCORRECT SEQUENCE DETECTED", 3.0f);
            queueSubtitle("OXYGEN PURGE TRIGGERED", 2.8f);
            queueSubtitle("ATMOSPHERIC PRESSURE LOST", 3.0f);
        }
        queueSubtitle("CREW VITAL SIGNS LOST", 2.8f);
    }
}

std::string getObjectiveText()
{
    if (gameState.playerDied) return "CREW LOST";
    if (gameState.gameFinished) return "MISSION COMPLETE";
    if (gameState.controlUnlocked) return "AUTHORIZE ESCAPE";
    if (!gameState.oxygenFixed) return "FIX OXYGEN";
    if (!gameState.powerFixed) return "RESTORE POWER";
    if (!gameState.foundNote) return "SEARCH STORAGE";
    if (!gameState.hasCode) return "DECODE CLUE";
    return "ENTER CONTROL CODE";
}

void updateInteractPrompt()
{
    nearestInteractableIndex = world.getNearestInteractableIndex(playerPos);
    showInteractPrompt =
        !controlCodePanelOpen &&
        !oxygenTerminalPanelOpen &&
        !labStabilizerPanelOpen &&
        !powerWirePanelOpen &&
        (nearestInteractableIndex != -1) &&
        !gameState.gameFinished &&
        !gameState.playerDied;
}

void handleInteraction(GLFWwindow* window)
{
    bool ePressedNow = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;

    if (controlCodePanelOpen || oxygenTerminalPanelOpen || labStabilizerPanelOpen || powerWirePanelOpen)
    {
        ePressedLastFrame = ePressedNow;
        return;
    }

    if (ePressedNow && !ePressedLastFrame && !gameState.gameFinished && !gameState.playerDied)
    {
        const std::string interactableName =
            nearestInteractableIndex != -1
            ? world.interactables[nearestInteractableIndex].name
            : "";

        if (nearestInteractableIndex != -1 &&
            interactableName == "control_door" &&
            !gameState.controlUnlocked)
        {
            controlCodePanelOpen = true;
            controlCodeRejected = false;
            controlCodeInput.clear();
            playOpenSound();
            ePressedLastFrame = ePressedNow;
            return;
        }

        if (nearestInteractableIndex != -1 &&
            interactableName == "lab_decoder" &&
            !gameState.hasCode)
        {
            labStabilizerPanelOpen = true;
            labStabilizerRejected = false;
            playOpenSound();
            ePressedLastFrame = ePressedNow;
            return;
        }

        if (nearestInteractableIndex != -1 &&
            interactableName == "power_console" &&
            gameState.oxygenFixed &&
            !gameState.powerFixed)
        {
            openPowerWirePanel();
            ePressedLastFrame = ePressedNow;
            return;
        }

        if (interactableName == "oxygen_terminal")
        {
            oxygenTerminalPanelOpen = true;
            playOpenSound();
            ePressedLastFrame = ePressedNow;
            return;
        }

        const bool hadFoundNote = gameState.foundNote;
        world.tryInteract(playerPos);
        if (!hadFoundNote && gameState.foundNote)
            playPickupSound();
    }

    ePressedLastFrame = ePressedNow;
}

void drawRectHUD(
    Shader& hudShader,
    unsigned int quadVAO,
    float x,
    float y,
    float w,
    float h,
    const glm::vec3& color)
{
    hudShader.setVec3("color", color);
    hudShader.setVec2("offset", glm::vec2(x, y));
    hudShader.setVec2("scale", glm::vec2(w, h));
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

const std::array<std::string, 7>& getGlyph(char c)
{
    static const std::array<std::string, 7> SPACE = {
        "00000","00000","00000","00000","00000","00000","00000"
    };
    static const std::array<std::string, 7> PERIOD = {
        "00000","00000","00000","00000","00000","01100","01100"
    };
    static const std::array<std::string, 7> HYPHEN = {
        "00000","00000","00000","11111","00000","00000","00000"
    };
    static const std::array<std::string, 7> A = {
        "01110","10001","10001","11111","10001","10001","10001"
    };
    static const std::array<std::string, 7> B = {
        "11110","10001","10001","11110","10001","10001","11110"
    };
    static const std::array<std::string, 7> C = {
        "01111","10000","10000","10000","10000","10000","01111"
    };
    static const std::array<std::string, 7> D = {
        "11110","10001","10001","10001","10001","10001","11110"
    };
    static const std::array<std::string, 7> E = {
        "11111","10000","10000","11110","10000","10000","11111"
    };
    static const std::array<std::string, 7> F = {
        "11111","10000","10000","11110","10000","10000","10000"
    };
    static const std::array<std::string, 7> G = {
        "01111","10000","10000","10111","10001","10001","01110"
    };
    static const std::array<std::string, 7> H = {
        "10001","10001","10001","11111","10001","10001","10001"
    };
    static const std::array<std::string, 7> I = {
        "11111","00100","00100","00100","00100","00100","11111"
    };
    static const std::array<std::string, 7> J = {
        "00001","00001","00001","00001","10001","10001","01110"
    };
    static const std::array<std::string, 7> K = {
        "10001","10010","10100","11000","10100","10010","10001"
    };
    static const std::array<std::string, 7> L = {
        "10000","10000","10000","10000","10000","10000","11111"
    };
    static const std::array<std::string, 7> M = {
        "10001","11011","10101","10101","10001","10001","10001"
    };
    static const std::array<std::string, 7> N = {
        "10001","11001","10101","10011","10001","10001","10001"
    };
    static const std::array<std::string, 7> O = {
        "01110","10001","10001","10001","10001","10001","01110"
    };
    static const std::array<std::string, 7> P = {
        "11110","10001","10001","11110","10000","10000","10000"
    };
    static const std::array<std::string, 7> Q = {
        "01110","10001","10001","10001","10101","10010","01101"
    };
    static const std::array<std::string, 7> R = {
        "11110","10001","10001","11110","10100","10010","10001"
    };
    static const std::array<std::string, 7> S = {
        "01111","10000","10000","01110","00001","00001","11110"
    };
    static const std::array<std::string, 7> T = {
        "11111","00100","00100","00100","00100","00100","00100"
    };
    static const std::array<std::string, 7> U = {
        "10001","10001","10001","10001","10001","10001","01110"
    };
    static const std::array<std::string, 7> V = {
        "10001","10001","10001","10001","10001","01010","00100"
    };
    static const std::array<std::string, 7> W = {
        "10001","10001","10001","10101","10101","10101","01010"
    };
    static const std::array<std::string, 7> X = {
        "10001","10001","01010","00100","01010","10001","10001"
    };
    static const std::array<std::string, 7> Y = {
        "10001","10001","01010","00100","00100","00100","00100"
    };
    static const std::array<std::string, 7> Z = {
        "11111","00001","00010","00100","01000","10000","11111"
    };
    static const std::array<std::string, 7> ZERO = {
        "01110","10001","10011","10101","11001","10001","01110"
    };
    static const std::array<std::string, 7> ONE = {
        "00100","01100","00100","00100","00100","00100","01110"
    };
    static const std::array<std::string, 7> TWO = {
        "01110","10001","00001","00010","00100","01000","11111"
    };
    static const std::array<std::string, 7> THREE = {
        "11110","00001","00001","01110","00001","00001","11110"
    };
    static const std::array<std::string, 7> FOUR = {
        "00010","00110","01010","10010","11111","00010","00010"
    };
    static const std::array<std::string, 7> FIVE = {
        "11111","10000","10000","11110","00001","00001","11110"
    };
    static const std::array<std::string, 7> SIX = {
        "01110","10000","10000","11110","10001","10001","01110"
    };
    static const std::array<std::string, 7> SEVEN = {
        "11111","00001","00010","00100","01000","01000","01000"
    };
    static const std::array<std::string, 7> EIGHT = {
        "01110","10001","10001","01110","10001","10001","01110"
    };
    static const std::array<std::string, 7> NINE = {
        "01110","10001","10001","01111","00001","00001","01110"
    };

    switch (c)
    {
    case 'A': return A;
    case 'B': return B;
    case 'C': return C;
    case 'D': return D;
    case 'E': return E;
    case 'F': return F;
    case 'G': return G;
    case 'H': return H;
    case 'I': return I;
    case 'J': return J;
    case 'K': return K;
    case 'L': return L;
    case 'M': return M;
    case 'N': return N;
    case 'O': return O;
    case 'P': return P;
    case 'Q': return Q;
    case 'R': return R;
    case 'S': return S;
    case 'T': return T;
    case 'U': return U;
    case 'V': return V;
    case 'W': return W;
    case 'X': return X;
    case 'Y': return Y;
    case 'Z': return Z;
    case '0': return ZERO;
    case '1': return ONE;
    case '2': return TWO;
    case '3': return THREE;
    case '4': return FOUR;
    case '5': return FIVE;
    case '6': return SIX;
    case '7': return SEVEN;
    case '8': return EIGHT;
    case '9': return NINE;
    case '.': return PERIOD;
    case '-': return HYPHEN;
    case ' ': return SPACE;
    default:  return SPACE;
    }
}

float getTextWidth(const std::string& text, float pixelSize, float letterSpacing)
{
    if (text.empty())
        return 0.0f;

    const float glyphWidth = 5.0f * pixelSize;
    return (float)text.size() * glyphWidth + (float)(text.size() - 1) * letterSpacing;
}

void drawTextHUD(
    Shader& hudShader,
    unsigned int quadVAO,
    const std::string& text,
    float x,
    float y,
    float pixelSize,
    const glm::vec3& color)
{
    float cursorX = x;
    const float letterSpacing = pixelSize;
    const float glyphWidth = 5.0f * pixelSize;

    for (char c : text)
    {
        const auto& glyph = getGlyph(c);

        for (int row = 0; row < 7; row++)
        {
            for (int col = 0; col < 5; col++)
            {
                if (glyph[row][col] == '1')
                {
                    float px = cursorX + col * pixelSize;
                    float py = y + row * pixelSize;
                    drawRectHUD(hudShader, quadVAO, px, py, pixelSize, pixelSize, color);
                }
            }
        }

        cursorX += glyphWidth + letterSpacing;
    }
}

std::vector<std::string> wrapText(const std::string& text, int maxCharsPerLine)
{
    std::vector<std::string> lines;
    std::string current;
    std::string word;

    for (size_t i = 0; i <= text.size(); i++)
    {
        char c = (i < text.size()) ? text[i] : ' ';

        if (c == ' ')
        {
            if (!word.empty())
            {
                if (current.empty())
                {
                    current = word;
                }
                else if ((int)(current.size() + 1 + word.size()) <= maxCharsPerLine)
                {
                    current += " " + word;
                }
                else
                {
                    lines.push_back(current);
                    current = word;
                }
                word.clear();
            }
        }
        else
        {
            word += c;
        }
    }

    if (!current.empty())
        lines.push_back(current);

    return lines;
}

void drawProgressAndObjective(const GameState& s, Shader& hudShader, unsigned int quadVAO)
{
    int progress = 0;

    if (s.oxygenFixed) progress = 1;
    if (s.powerFixed) progress = 2;
    if (s.foundNote) progress = 3;
    if (s.hasCode) progress = 4;
    if (s.controlUnlocked) progress = 5;

    hudShader.use();
    hudShader.setVec2("screenSize", glm::vec2((float)screenWidth, (float)screenHeight));
    glBindVertexArray(quadVAO);

    const float panelX = 16.0f;
    const float panelY = 16.0f;
    const float panelW = 310.0f;
    const float panelH = 96.0f;

    drawRectHUD(hudShader, quadVAO, panelX, panelY, panelW, panelH, glm::vec3(0.08f, 0.10f, 0.14f));
    drawRectHUD(hudShader, quadVAO, panelX + 3.0f, panelY + 3.0f, panelW - 6.0f, panelH - 6.0f, glm::vec3(0.12f, 0.15f, 0.20f));

    const std::string objectiveLabel = "OBJECTIVE";
    drawTextHUD(hudShader, quadVAO, objectiveLabel, panelX + 12.0f, panelY + 10.0f, 2.5f, glm::vec3(0.75f, 0.88f, 1.0f));

    const std::string objectiveText = getObjectiveText();
    drawTextHUD(hudShader, quadVAO, objectiveText, panelX + 12.0f, panelY + 36.0f, 2.6f, glm::vec3(0.96f, 0.96f, 0.96f));

    const float startX = panelX + 12.0f;
    const float startY = panelY + 68.0f;
    const float size = 20.0f;
    const float gap = 8.0f;

    for (int i = 0; i < 5; i++)
    {
        glm::vec3 color = (i < progress)
            ? glm::vec3(0.2f, 1.0f, 0.2f)
            : glm::vec3(0.25f, 0.25f, 0.25f);

        float x = startX + i * (size + gap);
        float y = startY;

        drawRectHUD(hudShader, quadVAO, x, y, size, size, color);
    }

    glBindVertexArray(0);
}

void drawInteractPrompt(Shader& hudShader, unsigned int quadVAO)
{
    if (!showInteractPrompt)
        return;

    const std::string promptText = "PRESS E TO INTERACT";
    const float textPixel = 4.0f;
    const float textWidth = getTextWidth(promptText, textPixel, textPixel);
    const float textHeight = 7.0f * textPixel;

    const float panelPaddingX = 20.0f;
    const float panelPaddingY = 16.0f;
    const float panelW = textWidth + panelPaddingX * 2.0f;
    const float panelH = textHeight + panelPaddingY * 2.0f;
    const float panelX = (screenWidth - panelW) * 0.5f;
    const float panelY = screenHeight - 120.0f;

    hudShader.use();
    hudShader.setVec2("screenSize", glm::vec2((float)screenWidth, (float)screenHeight));
    glBindVertexArray(quadVAO);

    drawRectHUD(hudShader, quadVAO, panelX + 4.0f, panelY + 4.0f, panelW, panelH, glm::vec3(0.0f, 0.0f, 0.0f));
    drawRectHUD(hudShader, quadVAO, panelX, panelY, panelW, panelH, glm::vec3(0.08f, 0.10f, 0.14f));
    drawRectHUD(hudShader, quadVAO, panelX + 3.0f, panelY + 3.0f, panelW - 6.0f, panelH - 6.0f, glm::vec3(0.12f, 0.15f, 0.20f));

    drawTextHUD(
        hudShader,
        quadVAO,
        promptText,
        panelX + panelPaddingX,
        panelY + panelPaddingY,
        textPixel,
        glm::vec3(0.90f, 0.96f, 1.0f)
    );

    glBindVertexArray(0);
}

void drawControlCodePanel(Shader& hudShader, unsigned int quadVAO)
{
    if (!controlCodePanelOpen)
        return;

    hudShader.use();
    hudShader.setVec2("screenSize", glm::vec2((float)screenWidth, (float)screenHeight));
    glBindVertexArray(quadVAO);

    const float panelW = 580.0f;
    const float panelH = 250.0f;
    const float panelX = (screenWidth - panelW) * 0.5f;
    const float panelY = (screenHeight - panelH) * 0.5f;

    drawRectHUD(hudShader, quadVAO, 0.0f, 0.0f, (float)screenWidth, (float)screenHeight, glm::vec3(0.01f, 0.015f, 0.025f));
    drawRectHUD(hudShader, quadVAO, panelX + 5.0f, panelY + 5.0f, panelW, panelH, glm::vec3(0.0f, 0.0f, 0.0f));
    drawRectHUD(hudShader, quadVAO, panelX, panelY, panelW, panelH, glm::vec3(0.08f, 0.10f, 0.14f));
    drawRectHUD(hudShader, quadVAO, panelX + 4.0f, panelY + 4.0f, panelW - 8.0f, panelH - 8.0f, glm::vec3(0.13f, 0.16f, 0.22f));

    const std::string title = "CONTROL ROOM CODE";
    const float titlePixel = 3.4f;
    drawTextHUD(
        hudShader,
        quadVAO,
        title,
        panelX + (panelW - getTextWidth(title, titlePixel, titlePixel)) * 0.5f,
        panelY + 28.0f,
        titlePixel,
        glm::vec3(0.78f, 0.90f, 1.0f)
    );

    const float slotSize = 58.0f;
    const float slotGap = 18.0f;
    const float slotsW = slotSize * 4.0f + slotGap * 3.0f;
    const float slotsX = panelX + (panelW - slotsW) * 0.5f;
    const float slotsY = panelY + 95.0f;

    for (int i = 0; i < 4; ++i)
    {
        const bool filled = i < static_cast<int>(controlCodeInput.size());
        const float x = slotsX + i * (slotSize + slotGap);
        const glm::vec3 boxColor = controlCodeRejected
            ? glm::vec3(0.45f, 0.08f, 0.12f)
            : filled
                ? glm::vec3(0.18f, 0.30f, 0.38f)
                : glm::vec3(0.07f, 0.09f, 0.13f);

        drawRectHUD(hudShader, quadVAO, x, slotsY, slotSize, slotSize, glm::vec3(0.02f, 0.025f, 0.035f));
        drawRectHUD(hudShader, quadVAO, x + 3.0f, slotsY + 3.0f, slotSize - 6.0f, slotSize - 6.0f, boxColor);

        if (filled)
        {
            const std::string digit(1, controlCodeInput[i]);
            const float digitPixel = 4.6f;
            drawTextHUD(
                hudShader,
                quadVAO,
                digit,
                x + (slotSize - getTextWidth(digit, digitPixel, digitPixel)) * 0.5f,
                slotsY + 13.0f,
                digitPixel,
                glm::vec3(0.95f, 0.98f, 1.0f)
            );
        }
    }

    const std::string status = controlCodeRejected ? "ACCESS DENIED" : "ENTER 4 DIGITS";
    const float statusPixel = 2.5f;
    drawTextHUD(
        hudShader,
        quadVAO,
        status,
        panelX + (panelW - getTextWidth(status, statusPixel, statusPixel)) * 0.5f,
        panelY + 178.0f,
        statusPixel,
        controlCodeRejected ? glm::vec3(1.0f, 0.35f, 0.35f) : glm::vec3(0.70f, 0.78f, 0.86f)
    );

    const std::string hint = "ENTER - CONFIRM  BACKSPACE - DELETE  ESC - CLOSE";
    const float hintPixel = 1.8f;
    drawTextHUD(
        hudShader,
        quadVAO,
        hint,
        panelX + (panelW - getTextWidth(hint, hintPixel, hintPixel)) * 0.5f,
        panelY + 214.0f,
        hintPixel,
        glm::vec3(0.54f, 0.62f, 0.72f)
    );

    glBindVertexArray(0);
}

void drawSteppedWireBreak(
    Shader& hudShader,
    unsigned int quadVAO,
    float startX,
    float centerY,
    float thickness,
    const glm::vec3& color,
    bool bendUp)
{
    const float segment = thickness * 1.2f;
    const float step = thickness * 0.55f;
    const float bandY = centerY - thickness * 0.5f;
    float currentX = startX;
    float currentY = bandY;
    const float dirY = bendUp ? -step : step;

    for (int i = 0; i < 3; ++i)
    {
        drawRectHUD(hudShader, quadVAO, currentX, currentY, segment, thickness, color);
        currentX += segment * 0.72f;
        currentY += dirY;
    }
}

void drawPowerWireRow(
    Shader& hudShader,
    unsigned int quadVAO,
    float x,
    float y,
    float width,
    float height,
    const glm::vec3& wireColor,
    bool selected,
    bool cut,
    bool highlightSelected)
{
    const glm::vec3 rowOuter = selected ? glm::vec3(0.19f, 0.28f, 0.36f) : glm::vec3(0.02f, 0.025f, 0.035f);
    const glm::vec3 rowInner = selected ? glm::vec3(0.12f, 0.18f, 0.24f) : glm::vec3(0.06f, 0.08f, 0.11f);
    drawRectHUD(hudShader, quadVAO, x, y, width, height, rowOuter);
    drawRectHUD(hudShader, quadVAO, x + 3.0f, y + 3.0f, width - 6.0f, height - 6.0f, rowInner);

    const float connectorW = 18.0f;
    const float wireThickness = 12.0f;
    const float wireY = y + (height - wireThickness) * 0.5f;
    const float centerY = y + height * 0.5f;
    const float leftX = x + 28.0f;
    const float rightX = x + width - 28.0f - connectorW;
    const float wireStart = leftX + connectorW;
    const float wireEnd = rightX;
    const float wireLength = wireEnd - wireStart;
    const float cutGap = 56.0f;

    drawRectHUD(hudShader, quadVAO, leftX, wireY - 3.0f, connectorW, wireThickness + 6.0f, glm::vec3(0.18f, 0.20f, 0.24f));
    drawRectHUD(hudShader, quadVAO, rightX, wireY - 3.0f, connectorW, wireThickness + 6.0f, glm::vec3(0.18f, 0.20f, 0.24f));

    if (!cut)
    {
        drawRectHUD(hudShader, quadVAO, wireStart, wireY, wireLength, wireThickness, wireColor);
        drawRectHUD(hudShader, quadVAO, wireStart, wireY + 2.0f, wireLength, 3.0f, glm::min(wireColor + glm::vec3(0.18f), glm::vec3(1.0f)));
        if (highlightSelected)
        {
            const float pulse = 0.45f + 0.35f * std::sin(lastFrame * 6.0f);
            drawRectHUD(
                hudShader,
                quadVAO,
                wireStart,
                wireY - 4.0f,
                wireLength,
                2.0f,
                glm::vec3(0.75f + 0.15f * pulse, 0.85f + 0.10f * pulse, 1.0f)
            );
        }
        return;
    }

    const float cutCenter = wireStart + wireLength * 0.5f;
    const float leftLen = glm::max(0.0f, cutCenter - cutGap * 0.5f - wireStart - 10.0f);
    const float rightStart = cutCenter + cutGap * 0.5f + 10.0f;
    const float rightLen = glm::max(0.0f, wireEnd - rightStart);

    if (leftLen > 0.0f)
    {
        drawRectHUD(hudShader, quadVAO, wireStart, wireY, leftLen, wireThickness, wireColor);
        drawRectHUD(hudShader, quadVAO, wireStart, wireY + 2.0f, leftLen, 3.0f, glm::min(wireColor + glm::vec3(0.16f), glm::vec3(1.0f)));
    }

    if (rightLen > 0.0f)
    {
        drawRectHUD(hudShader, quadVAO, rightStart, wireY, rightLen, wireThickness, wireColor);
        drawRectHUD(hudShader, quadVAO, rightStart, wireY + 2.0f, rightLen, 3.0f, glm::min(wireColor + glm::vec3(0.16f), glm::vec3(1.0f)));
    }

    drawSteppedWireBreak(hudShader, quadVAO, cutCenter - cutGap * 0.5f - 26.0f, centerY, wireThickness, wireColor, true);
    drawSteppedWireBreak(hudShader, quadVAO, cutCenter + cutGap * 0.5f - 2.0f, centerY, wireThickness, wireColor, false);
}

void drawPowerWirePanel(Shader& hudShader, unsigned int quadVAO)
{
    if (!powerWirePanelOpen)
        return;

    hudShader.use();
    hudShader.setVec2("screenSize", glm::vec2((float)screenWidth, (float)screenHeight));
    glBindVertexArray(quadVAO);

    const float panelW = 740.0f;
    const float panelH = 470.0f;
    const float panelX = (screenWidth - panelW) * 0.5f;
    const float panelY = (screenHeight - panelH) * 0.5f;

    drawRectHUD(hudShader, quadVAO, 0.0f, 0.0f, (float)screenWidth, (float)screenHeight, glm::vec3(0.01f, 0.015f, 0.025f));
    drawRectHUD(hudShader, quadVAO, panelX + 5.0f, panelY + 5.0f, panelW, panelH, glm::vec3(0.0f, 0.0f, 0.0f));
    drawRectHUD(hudShader, quadVAO, panelX, panelY, panelW, panelH, glm::vec3(0.08f, 0.10f, 0.14f));
    drawRectHUD(hudShader, quadVAO, panelX + 4.0f, panelY + 4.0f, panelW - 8.0f, panelH - 8.0f, glm::vec3(0.13f, 0.16f, 0.22f));

    const std::string title = "POWER ROUTING";
    drawTextHUD(
        hudShader,
        quadVAO,
        title,
        panelX + (panelW - getTextWidth(title, 3.4f, 3.4f)) * 0.5f,
        panelY + 24.0f,
        3.4f,
        glm::vec3(0.78f, 0.90f, 1.0f)
    );

    const std::array<glm::vec3, GameState::kPowerWireCount> wireColors{
        glm::vec3(0.95f, 0.29f, 0.32f),
        glm::vec3(0.22f, 0.78f, 0.38f),
        glm::vec3(0.95f, 0.82f, 0.18f),
        glm::vec3(0.20f, 0.68f, 1.0f),
        glm::vec3(0.96f, 0.44f, 0.82f)
    };
    const std::array<std::string, GameState::kPowerWireCount> wireLabels{
        "LINE 1", "LINE 2", "LINE 3", "LINE 4", "LINE 5"
    };

    const float labelX = panelX + 46.0f;
    const float rowX = panelX + 126.0f;
    const float rowW = panelW - 178.0f;
    const float rowH = 46.0f;
    const float rowStartY = panelY + 88.0f;
    const float rowGap = 58.0f;

    for (int i = 0; i < GameState::kPowerWireCount; ++i)
    {
        const float rowY = rowStartY + rowGap * static_cast<float>(i);
        drawTextHUD(
            hudShader,
            quadVAO,
            wireLabels[i],
            labelX,
            rowY + 12.0f,
            1.7f,
            i == powerWireSelected ? glm::vec3(0.88f, 0.95f, 1.0f) : glm::vec3(0.52f, 0.60f, 0.70f)
        );
        drawPowerWireRow(
            hudShader,
            quadVAO,
            rowX,
            rowY,
            rowW,
            rowH,
            wireColors[i],
            i == powerWireSelected,
            powerWireCut[i],
            i == powerWireSelected && !powerWireCut[i]
        );
    }

    const std::string status =
        powerWireResolving
            ? (powerWireCutWasCorrect ? "POWER RESTORED" : "SEQUENCE ERROR")
            : ("CUT " + std::to_string(powerWireCutOrder.size()) + " OF 5");
    const glm::vec3 statusColor =
        powerWireResolving
            ? (powerWireCutWasCorrect ? glm::vec3(0.45f, 1.0f, 0.62f) : glm::vec3(1.0f, 0.42f, 0.42f))
            : glm::vec3(0.70f, 0.78f, 0.86f);
    drawTextHUD(
        hudShader,
        quadVAO,
        status,
        panelX + (panelW - getTextWidth(status, 2.3f, 2.3f)) * 0.5f,
        panelY + 414.0f,
        2.3f,
        statusColor
    );

    const std::string hint = "ARROWS - SELECT  ENTER - CUT  ESC - CLOSE";
    drawTextHUD(
        hudShader,
        quadVAO,
        hint,
        panelX + (panelW - getTextWidth(hint, 1.7f, 1.7f)) * 0.5f,
        panelY + 444.0f,
        1.7f,
        glm::vec3(0.54f, 0.62f, 0.72f)
    );

    glBindVertexArray(0);
}

void drawLabStabilizerPanel(Shader& hudShader, unsigned int quadVAO)
{
    if (!labStabilizerPanelOpen)
        return;

    hudShader.use();
    hudShader.setVec2("screenSize", glm::vec2((float)screenWidth, (float)screenHeight));
    glBindVertexArray(quadVAO);

    const float panelW = 560.0f;
    const float panelH = 330.0f;
    const float panelX = (screenWidth - panelW) * 0.5f;
    const float panelY = (screenHeight - panelH) * 0.5f;

    drawRectHUD(hudShader, quadVAO, 0.0f, 0.0f, (float)screenWidth, (float)screenHeight, glm::vec3(0.01f, 0.015f, 0.025f));
    drawRectHUD(hudShader, quadVAO, panelX + 5.0f, panelY + 5.0f, panelW, panelH, glm::vec3(0.0f, 0.0f, 0.0f));
    drawRectHUD(hudShader, quadVAO, panelX, panelY, panelW, panelH, glm::vec3(0.08f, 0.10f, 0.14f));
    drawRectHUD(hudShader, quadVAO, panelX + 4.0f, panelY + 4.0f, panelW - 8.0f, panelH - 8.0f, glm::vec3(0.13f, 0.16f, 0.22f));

    const std::string title = "SAMPLE STABILIZATION";
    const float titlePixel = 3.2f;
    drawTextHUD(
        hudShader,
        quadVAO,
        title,
        panelX + (panelW - getTextWidth(title, titlePixel, titlePixel)) * 0.5f,
        panelY + 28.0f,
        titlePixel,
        glm::vec3(0.78f, 0.90f, 1.0f)
    );

    const std::array<std::string, 3> labels{ "LIGHT", "FOCUS", "CONTRAST" };
    const float labelPixel = 2.7f;
    const float valuePixel = 3.8f;
    const float rowY = panelY + 92.0f;
    const float rowGap = 58.0f;
    const float labelX = panelX + 72.0f;
    const float valueX = panelX + 370.0f;

    for (int i = 0; i < 3; ++i)
    {
        const float y = rowY + rowGap * static_cast<float>(i);
        const bool selected = i == labStabilizerSelected;
        const glm::vec3 rowColor = selected ? glm::vec3(0.18f, 0.30f, 0.38f) : glm::vec3(0.07f, 0.09f, 0.13f);
        const glm::vec3 textColor = selected ? glm::vec3(0.95f, 0.98f, 1.0f) : glm::vec3(0.68f, 0.76f, 0.84f);

        drawRectHUD(hudShader, quadVAO, panelX + 48.0f, y - 10.0f, panelW - 96.0f, 44.0f, glm::vec3(0.02f, 0.025f, 0.035f));
        drawRectHUD(hudShader, quadVAO, panelX + 52.0f, y - 6.0f, panelW - 104.0f, 36.0f, rowColor);

        drawTextHUD(hudShader, quadVAO, labels[i], labelX, y, labelPixel, textColor);

        const std::string value = std::to_string(labStabilizerValues[i]);
        drawTextHUD(
            hudShader,
            quadVAO,
            value,
            valueX + (48.0f - getTextWidth(value, valuePixel, valuePixel)) * 0.5f,
            y - 5.0f,
            valuePixel,
            textColor
        );
    }

    const std::string status = labStabilizerRejected ? "SIGNAL UNSTABLE" : "TRACE SIGNAL LOCKED";
    const float statusPixel = 2.4f;
    drawTextHUD(
        hudShader,
        quadVAO,
        status,
        panelX + (panelW - getTextWidth(status, statusPixel, statusPixel)) * 0.5f,
        panelY + 265.0f,
        statusPixel,
        labStabilizerRejected ? glm::vec3(1.0f, 0.35f, 0.35f) : glm::vec3(0.70f, 0.78f, 0.86f)
    );

    const std::string hint = "ARROWS - ADJUST  ENTER - ANALYZE  ESC - CLOSE";
    const float hintPixel = 1.55f;
    drawTextHUD(
        hudShader,
        quadVAO,
        hint,
        panelX + (panelW - getTextWidth(hint, hintPixel, hintPixel)) * 0.5f,
        panelY + 300.0f,
        hintPixel,
        glm::vec3(0.54f, 0.62f, 0.72f)
    );

    glBindVertexArray(0);
}

void drawOxygenTerminalPanel(Shader& hudShader, unsigned int quadVAO)
{
    if (!oxygenTerminalPanelOpen)
        return;

    hudShader.use();
    hudShader.setVec2("screenSize", glm::vec2((float)screenWidth, (float)screenHeight));
    glBindVertexArray(quadVAO);

    const float panelW = 760.0f;
    const float panelH = 310.0f;
    const float panelX = (screenWidth - panelW) * 0.5f;
    const float panelY = (screenHeight - panelH) * 0.5f;

    drawRectHUD(hudShader, quadVAO, 0.0f, 0.0f, (float)screenWidth, (float)screenHeight, glm::vec3(0.02f, 0.03f, 0.06f));
    drawRectHUD(hudShader, quadVAO, panelX + 6.0f, panelY + 6.0f, panelW, panelH, glm::vec3(0.01f, 0.01f, 0.03f));
    drawRectHUD(hudShader, quadVAO, panelX, panelY, panelW, panelH, glm::vec3(0.08f, 0.10f, 0.16f));
    drawRectHUD(hudShader, quadVAO, panelX + 4.0f, panelY + 4.0f, panelW - 8.0f, panelH - 8.0f, glm::vec3(0.09f, 0.12f, 0.20f));
    drawRectHUD(hudShader, quadVAO, panelX + 26.0f, panelY + 68.0f, panelW - 52.0f, 2.0f, glm::vec3(0.36f, 0.90f, 1.0f));

    const std::string title = "OXYGEN TERMINAL";
    const float titlePixel = 3.5f;
    drawTextHUD(
        hudShader,
        quadVAO,
        title,
        panelX + 28.0f,
        panelY + 24.0f,
        titlePixel,
        glm::vec3(0.60f, 0.92f, 1.0f)
    );

    const std::string statusLabel = "STATUS";
    const std::string statusValue = gameState.oxygenFixed ? "OXYGEN NORMAL" : "OXYGEN MALFUNCTION";
    const glm::vec3 statusColor = gameState.oxygenFixed
        ? glm::vec3(0.42f, 1.0f, 0.62f)
        : glm::vec3(1.0f, 0.44f, 0.44f);
    drawTextHUD(hudShader, quadVAO, statusLabel, panelX + 28.0f, panelY + 88.0f, 2.4f, glm::vec3(0.72f, 0.82f, 0.95f));
    drawTextHUD(hudShader, quadVAO, statusValue, panelX + 28.0f, panelY + 122.0f, 3.0f, statusColor);

    const std::vector<std::string> bodyLines = gameState.oxygenFixed
        ? std::vector<std::string>{
            "LIFE SUPPORT FLOW STABLE",
            "PRESSURE AND MIXTURE WITHIN SAFE RANGE",
            "NO FURTHER ACTION REQUIRED"
        }
        : std::vector<std::string>{
            "PLEASE REACTIVATE THE SYSTEM",
            "ROTATE THE PIPES IN THE CORRECT ORDER",
            "TO RESTORE OXYGEN FLOW"
        };

    float lineY = panelY + 174.0f;
    for (const auto& line : bodyLines)
    {
        drawTextHUD(
            hudShader,
            quadVAO,
            line,
            panelX + 28.0f,
            lineY,
            2.15f,
            glm::vec3(0.90f, 0.95f, 1.0f)
        );
        lineY += 34.0f;
    }

    const std::string hint = "ESC CLOSE";
    const float hintPixel = 1.9f;
    drawTextHUD(
        hudShader,
        quadVAO,
        hint,
        panelX + panelW - getTextWidth(hint, hintPixel, hintPixel) - 28.0f,
        panelY + panelH - 40.0f,
        hintPixel,
        glm::vec3(0.58f, 0.68f, 0.82f)
    );

    glBindVertexArray(0);
}

void drawSubtitle(Shader& hudShader, unsigned int quadVAO)
{
    if (currentSubtitle.empty())
        return;

    std::vector<std::string> lines = wrapText(currentSubtitle, 34);
    if (lines.empty())
        return;

    const float textPixel = 3.6f;
    const float lineHeight = 7.0f * textPixel + 10.0f;
    float maxWidth = 0.0f;

    for (const auto& line : lines)
    {
        float w = getTextWidth(line, textPixel, textPixel);
        if (w > maxWidth) maxWidth = w;
    }

    const float panelPaddingX = 22.0f;
    const float panelPaddingY = 16.0f;
    const float panelW = maxWidth + panelPaddingX * 2.0f;
    const float panelH = lines.size() * lineHeight + panelPaddingY * 2.0f - 10.0f;
    const float panelX = (screenWidth - panelW) * 0.5f;
    const float panelY = screenHeight - 220.0f;

    hudShader.use();
    hudShader.setVec2("screenSize", glm::vec2((float)screenWidth, (float)screenHeight));
    glBindVertexArray(quadVAO);

    drawRectHUD(hudShader, quadVAO, panelX, panelY, panelW, panelH, glm::vec3(0.02f, 0.02f, 0.03f));

    float textY = panelY + panelPaddingY;
    for (const auto& line : lines)
    {
        float lineWidth = getTextWidth(line, textPixel, textPixel);
        float textX = panelX + (panelW - lineWidth) * 0.5f;

        drawTextHUD(
            hudShader,
            quadVAO,
            line,
            textX,
            textY,
            textPixel,
            glm::vec3(0.98f, 0.98f, 0.98f)
        );

        textY += lineHeight;
    }

    glBindVertexArray(0);
}

void drawEndingOverlay(Shader& hudShader, unsigned int quadVAO)
{
    if (!gameState.gameFinished && !gameState.playerDied)
        return;

    if (gameState.playerDied &&
        (!currentSubtitle.empty() || !subtitleQueue.empty() || !deathAnimationTriggered || !deathAnimationFinished))
        return;

    hudShader.use();
    hudShader.setVec2("screenSize", glm::vec2((float)screenWidth, (float)screenHeight));
    glBindVertexArray(quadVAO);

    drawRectHUD(hudShader, quadVAO, 0.0f, 0.0f, (float)screenWidth, (float)screenHeight, glm::vec3(0.01f, 0.02f, 0.03f));

    const bool playerLost = gameState.playerDied;
    const std::string title = playerLost ? "MISSION FAILED" : "MISSION COMPLETE";
    const std::string subtitle = playerLost
        ? (gameState.powerPuzzleFailed ? "POWER FAILURE" : "OXYGEN DEPLETED")
        : "YOU ESCAPED";
    const std::string hint = "PRESS R TO RESTART  ESC TO EXIT";

    float titlePixel = 6.0f;
    float subtitlePixel = 4.0f;
    float hintPixel = 2.6f;
    const float centerY = screenHeight * 0.5f;
    const float titleY = centerY - 95.0f;
    const float subtitleY = centerY - 15.0f;
    const float hintY = centerY + 70.0f;

    drawTextHUD(
        hudShader,
        quadVAO,
        title,
        (screenWidth - getTextWidth(title, titlePixel, titlePixel)) * 0.5f,
        titleY,
        titlePixel,
        playerLost ? glm::vec3(1.0f, 0.78f, 0.78f) : glm::vec3(0.85f, 0.97f, 1.0f)
    );

    drawTextHUD(
        hudShader,
        quadVAO,
        subtitle,
        (screenWidth - getTextWidth(subtitle, subtitlePixel, subtitlePixel)) * 0.5f,
        subtitleY,
        subtitlePixel,
        playerLost ? glm::vec3(1.0f, 0.32f, 0.32f) : glm::vec3(0.35f, 1.0f, 0.65f)
    );

    drawTextHUD(
        hudShader,
        quadVAO,
        hint,
        (screenWidth - getTextWidth(hint, hintPixel, hintPixel)) * 0.5f,
        hintY,
        hintPixel,
        glm::vec3(0.80f, 0.86f, 0.92f)
    );

    glBindVertexArray(0);
}

bool isEndingOverlayVisible()
{
    if (!gameState.gameFinished && !gameState.playerDied)
        return false;

    if (gameState.playerDied &&
        (!currentSubtitle.empty() || !subtitleQueue.empty() || !deathAnimationTriggered || !deathAnimationFinished))
        return false;

    return true;
}

void updateHeavyBreathingLoop()
{
    if (!gameStarted || gameState.oxygenFixed)
    {
        stopHeavyBreathingLoop();
        return;
    }

    if (!heavyBreathingPlaying)
        startHeavyBreathingLoop();

    if (!heavyBreathingPlaying)
        return;

    if (gameState.playerDied && gameState.oxygenPuzzleFailed)
    {
        if (deathAnimationTriggered)
        {
            stopHeavyBreathingLoop();
            return;
        }

        if (isEndingOverlayVisible())
            heavyBreathingVolume = 1.0f;
        else
            heavyBreathingVolume = std::min(1.0f, heavyBreathingVolume + deltaTime * 0.25f);

        audio.setLoopVolume(kHeavyBreathingLoopId, heavyBreathingVolume);
    }
}

void drawStartMenu(Shader& hudShader, unsigned int quadVAO, float timeSeconds)
{
    hudShader.use();
    hudShader.setVec2("screenSize", glm::vec2((float)screenWidth, (float)screenHeight));
    glBindVertexArray(quadVAO);

    drawRectHUD(hudShader, quadVAO, 0.0f, 0.0f, (float)screenWidth, (float)screenHeight, glm::vec3(0.01f, 0.015f, 0.03f));

    const std::string title = "SPACE STATION ESCAPE";
    const std::string subtitle = "RESTORE THE STATION AND FIND THE EXIT";
    const std::string startText = "PRESS ENTER OR SPACE TO START";
    const std::string exitText = "PRESS ESC TO EXIT";
    const float titlePixel = 6.0f;
    const float subtitlePixel = 2.7f;
    const float startPixel = 3.0f;
    const float exitPixel = 2.1f;
    const float pulse = 0.65f + 0.35f * std::sin(timeSeconds * 3.5f);
    const float centerY = screenHeight * 0.5f;
    const float titleY = centerY - 150.0f;
    const float subtitleY = centerY - 60.0f;
    const float startY = centerY + 45.0f;
    const float exitY = centerY + 110.0f;

    drawTextHUD(
        hudShader,
        quadVAO,
        title,
        (screenWidth - getTextWidth(title, titlePixel, titlePixel)) * 0.5f,
        titleY,
        titlePixel,
        glm::vec3(0.82f, 0.94f, 1.0f)
    );

    drawTextHUD(
        hudShader,
        quadVAO,
        subtitle,
        (screenWidth - getTextWidth(subtitle, subtitlePixel, subtitlePixel)) * 0.5f,
        subtitleY,
        subtitlePixel,
        glm::vec3(0.56f, 0.68f, 0.80f)
    );

    drawTextHUD(
        hudShader,
        quadVAO,
        startText,
        (screenWidth - getTextWidth(startText, startPixel, startPixel)) * 0.5f,
        startY,
        startPixel,
        glm::vec3(0.45f + 0.35f * pulse, 0.82f + 0.12f * pulse, 1.0f)
    );

    drawTextHUD(
        hudShader,
        quadVAO,
        exitText,
        (screenWidth - getTextWidth(exitText, exitPixel, exitPixel)) * 0.5f,
        exitY,
        exitPixel,
        glm::vec3(0.50f, 0.58f, 0.68f)
    );

    glBindVertexArray(0);
}

int main()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Space Station Escape", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    if (audio.init())
    {
        audio.playLoop(kSpaceStationLoopId, std::string(PROJECT_ROOT) + "/assets/audio/space_station.wav", 0.25f);
        audio.preloadClip(kCodeTypingClipIds[0], std::string(PROJECT_ROOT) + "/assets/audio/typing1.wav", 8);
        audio.preloadClip(kCodeTypingClipIds[1], std::string(PROJECT_ROOT) + "/assets/audio/typing2.wav", 8);
        audio.preloadClip(kArrowUpClipId, std::string(PROJECT_ROOT) + "/assets/audio/arrow_up.wav", 4);
        audio.preloadClip(kArrowDownClipId, std::string(PROJECT_ROOT) + "/assets/audio/arrow_down.wav", 4);
        audio.preloadClip(kConfirmationClipId, std::string(PROJECT_ROOT) + "/assets/audio/confirmation.wav", 4);
        audio.preloadClip(kErrorClipId, std::string(PROJECT_ROOT) + "/assets/audio/error.wav", 4);
        audio.preloadClip(kOpenClipId, std::string(PROJECT_ROOT) + "/assets/audio/open.wav", 4);
        audio.preloadClip(kCloseClipId, std::string(PROJECT_ROOT) + "/assets/audio/close.wav", 4);
        audio.preloadClip(kCutClipId, std::string(PROJECT_ROOT) + "/assets/audio/cut.wav", 4);
        audio.preloadClip(kDoorOpenClipId, std::string(PROJECT_ROOT) + "/assets/audio/door_open.wav", 2);
        audio.preloadClip(kStartClipId, std::string(PROJECT_ROOT) + "/assets/audio/start.wav", 2);
        audio.preloadClip(kPickupClipId, std::string(PROJECT_ROOT) + "/assets/audio/pickup.wav", 2);
        audio.preloadClip(kWinClipId, std::string(PROJECT_ROOT) + "/assets/audio/win.wav", 1);
        audio.preloadClip(kLoseClipId, std::string(PROJECT_ROOT) + "/assets/audio/lose.wav", 1);
        audio.preloadClip(kPowerShutdownClipId, std::string(PROJECT_ROOT) + "/assets/audio/power_shutdown.wav", 1);
        audio.preloadClip(kBeepClipId, std::string(PROJECT_ROOT) + "/assets/audio/beep.wav", 6);
        audio.preloadClip(kGasClipId, std::string(PROJECT_ROOT) + "/assets/audio/gas.wav", 4);
        audio.preloadClip(kTurnValveClipId, std::string(PROJECT_ROOT) + "/assets/audio/turn_valve.wav", 4);
        for (size_t i = 0; i < kFootstepConcreteClipIds.size(); ++i)
        {
            const std::string path =
                std::string(PROJECT_ROOT) + "/assets/audio/footstep_concrete_00" + std::to_string(i) + ".wav";
            if (!audio.preloadClip(kFootstepConcreteClipIds[i], path, 4))
                std::cerr << "Failed to preload footstep clip: " << path << "\n";
        }
    }

    glEnable(GL_DEPTH_TEST);

    Shader shader(
        std::string(PROJECT_ROOT) + "/shaders/basic.vs",
        std::string(PROJECT_ROOT) + "/shaders/basic.fs"
    );

    Shader characterShader(
        std::string(PROJECT_ROOT) + "/shaders/character.vs",
        std::string(PROJECT_ROOT) + "/shaders/character.fs"
    );

    Shader staticModelShader(
        std::string(PROJECT_ROOT) + "/shaders/static_model.vs",
        std::string(PROJECT_ROOT) + "/shaders/static_model.fs"
    );

    Shader hudShader(
        std::string(PROJECT_ROOT) + "/shaders/hud.vs",
        std::string(PROJECT_ROOT) + "/shaders/hud.fs"
    );

    const std::string astronautTexture =
        std::string(PROJECT_ROOT) + "/assets/models/character/astronaut/textures/AstronautColor.png";

    AnimatedCharacter idleCharacter(
        std::string(PROJECT_ROOT) + "/assets/animation/character/idle.glb",
        astronautTexture,
        true
    );

    AnimatedCharacter walkCharacter(
        std::string(PROJECT_ROOT) + "/assets/animation/character/walking.glb",
        astronautTexture,
        true
    );

    AnimatedCharacter runCharacter(
        std::string(PROJECT_ROOT) + "/assets/animation/character/running.glb",
        astronautTexture,
        true
    );

    AnimatedCharacter danceCharacter(
        std::string(PROJECT_ROOT) + "/assets/animation/character/chicken_dance.glb",
        astronautTexture,
        false
    );

    AnimatedCharacter deathCharacter(
        std::string(PROJECT_ROOT) + "/assets/animation/character/dying.glb",
        astronautTexture,
        false
    );

    if (!idleCharacter.isLoaded())
        std::cerr << "Idle character failed: " << idleCharacter.getError() << "\n";
    if (!walkCharacter.isLoaded())
        std::cerr << "Walk character failed: " << walkCharacter.getError() << "\n";
    if (!runCharacter.isLoaded())
        std::cerr << "Run character failed: " << runCharacter.getError() << "\n";
    if (!danceCharacter.isLoaded())
        std::cerr << "Dance character failed: " << danceCharacter.getError() << "\n";
    if (!deathCharacter.isLoaded())
        std::cerr << "Death character failed: " << deathCharacter.getError() << "\n";

    float vertices[] = {
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,

        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,

        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,

         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,

        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,

        -0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    float quadVertices[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,

        0.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };

    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    unsigned int debugLineVAO = 0;
    unsigned int debugLineVBO = 0;
    glGenVertexArrays(1, &debugLineVAO);
    glGenBuffers(1, &debugLineVBO);
    glBindVertexArray(debugLineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, debugLineVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    auto drawCube = [&](glm::vec3 position, glm::vec3 scale, glm::vec3 color, float rotationY = 0.0f)
        {
            shader.use();

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, position);
            model = glm::rotate(model, glm::radians(rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, scale);

            shader.setMat4("model", model);
            shader.setVec3("objectColor", color);

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        };

    auto drawDebugLines = [&](const std::vector<glm::vec3>& vertices, glm::vec3 color)
        {
            if (vertices.empty())
                return;

            shader.use();
            shader.setMat4("model", glm::mat4(1.0f));
            shader.setVec3("objectColor", color);

            glBindVertexArray(debugLineVAO);
            glBindBuffer(GL_ARRAY_BUFFER, debugLineVBO);
            glBufferData(
                GL_ARRAY_BUFFER,
                vertices.size() * sizeof(glm::vec3),
                vertices.data(),
                GL_DYNAMIC_DRAW
            );
            glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertices.size()));
            glBindVertexArray(0);
        };

    auto drawDebugBox = [&](const BoxCollider& box, glm::vec3 color)
        {
            const glm::vec3 min = box.center - box.halfSize;
            const glm::vec3 max = box.center + box.halfSize;
            const glm::vec3 corners[8] = {
                { min.x, min.y, min.z },
                { max.x, min.y, min.z },
                { max.x, min.y, max.z },
                { min.x, min.y, max.z },
                { min.x, max.y, min.z },
                { max.x, max.y, min.z },
                { max.x, max.y, max.z },
                { min.x, max.y, max.z }
            };
            const int edges[24] = {
                0, 1, 1, 2, 2, 3, 3, 0,
                4, 5, 5, 6, 6, 7, 7, 4,
                0, 4, 1, 5, 2, 6, 3, 7
            };

            std::vector<glm::vec3> vertices;
            vertices.reserve(24);
            for (int index : edges)
                vertices.push_back(corners[index]);

            drawDebugLines(vertices, color);
        };

    auto drawDebugVerticalCylinder = [&](glm::vec3 center, float radius, float halfHeight, glm::vec3 color)
        {
            constexpr int kSegments = 32;
            const float bottomY = center.y - halfHeight;
            const float topY = center.y + halfHeight;
            std::vector<glm::vec3> vertices;
            vertices.reserve(kSegments * 6);

            for (int i = 0; i < kSegments; ++i)
            {
                const float angleA = glm::two_pi<float>() * static_cast<float>(i) / static_cast<float>(kSegments);
                const float angleB = glm::two_pi<float>() * static_cast<float>(i + 1) / static_cast<float>(kSegments);
                const glm::vec3 bottomA(center.x + std::cos(angleA) * radius, bottomY, center.z + std::sin(angleA) * radius);
                const glm::vec3 bottomB(center.x + std::cos(angleB) * radius, bottomY, center.z + std::sin(angleB) * radius);
                const glm::vec3 topA(bottomA.x, topY, bottomA.z);
                const glm::vec3 topB(bottomB.x, topY, bottomB.z);

                vertices.push_back(bottomA);
                vertices.push_back(bottomB);
                vertices.push_back(topA);
                vertices.push_back(topB);
                if (i % 4 == 0)
                {
                    vertices.push_back(bottomA);
                    vertices.push_back(topA);
                }
            }

            drawDebugLines(vertices, color);
        };

    auto drawDebugHorizontalCylinder = [&](const HorizontalCylinderCollider& cylinder, glm::vec3 color)
        {
            constexpr int kSegments = 32;
            const glm::vec3 axis = glm::normalize(glm::vec3(cylinder.axisXZ.x, 0.0f, cylinder.axisXZ.z));
            const glm::vec3 side = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), axis));
            const glm::vec3 up(0.0f, 1.0f, 0.0f);
            const glm::vec3 endA = cylinder.center - axis * cylinder.halfLength;
            const glm::vec3 endB = cylinder.center + axis * cylinder.halfLength;
            std::vector<glm::vec3> vertices;
            vertices.reserve(kSegments * 6);

            for (int i = 0; i < kSegments; ++i)
            {
                const float angleA = glm::two_pi<float>() * static_cast<float>(i) / static_cast<float>(kSegments);
                const float angleB = glm::two_pi<float>() * static_cast<float>(i + 1) / static_cast<float>(kSegments);
                const glm::vec3 ringA0 = side * (std::cos(angleA) * cylinder.radius) + up * (std::sin(angleA) * cylinder.radius);
                const glm::vec3 ringA1 = side * (std::cos(angleB) * cylinder.radius) + up * (std::sin(angleB) * cylinder.radius);
                const glm::vec3 a0 = endA + ringA0;
                const glm::vec3 a1 = endA + ringA1;
                const glm::vec3 b0 = endB + ringA0;
                const glm::vec3 b1 = endB + ringA1;

                vertices.push_back(a0);
                vertices.push_back(a1);
                vertices.push_back(b0);
                vertices.push_back(b1);
                if (i % 4 == 0)
                {
                    vertices.push_back(a0);
                    vertices.push_back(b0);
                }
            }

            drawDebugLines(vertices, color);
        };

    auto drawCollisionDebug = [&]()
        {
            if (!showCollisionDebug)
                return;

            glDisable(GL_DEPTH_TEST);
            glLineWidth(2.0f);

            for (const auto& box : world.colliders)
                drawDebugBox(box, glm::vec3(1.0f, 0.35f, 0.20f));
            for (const auto& circle : world.circleColliders)
                drawDebugVerticalCylinder(circle.center + glm::vec3(0.0f, 1.0f, 0.0f), circle.radius, 1.0f, glm::vec3(0.25f, 0.9f, 1.0f));
            for (const auto& cylinder : world.cylinderColliders)
                drawDebugVerticalCylinder(cylinder.center, cylinder.radius, cylinder.halfHeight, glm::vec3(0.2f, 1.0f, 0.35f));
            for (const auto& cylinder : world.horizontalCylinderColliders)
                drawDebugHorizontalCylinder(cylinder, glm::vec3(1.0f, 0.95f, 0.15f));
            for (const auto& door : world.doors)
            {
                if (!door.open)
                    drawDebugBox({ door.center, door.halfSize }, glm::vec3(1.0f, 0.1f, 0.85f));
            }

            glLineWidth(1.0f);
            glEnable(GL_DEPTH_TEST);
        };

    const glm::vec3 oxygenPipeBodyColor(0.75f, 0.78f, 0.94f);

    auto drawStaticModel = [&](StaticModel& modelAsset, const ModelPlacement& placement, bool usePartColors = false, bool recolorBlueToRed = false)
        {
            if (!modelAsset.isLoaded())
                return;

            staticModelShader.use();

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, placement.position);
            model = glm::rotate(model, glm::radians(placement.rotationX), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(placement.rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(placement.rotationZ), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, placement.scale);

            staticModelShader.setMat4("model", model);
            staticModelShader.setVec3("tintColor", placement.color);
            staticModelShader.setInt("usePartColors", usePartColors ? 1 : 0);
            staticModelShader.setInt("recolorBlueToRed", recolorBlueToRed ? 1 : 0);
            staticModelShader.setVec3("partBaseColor", oxygenPipeBodyColor);
            staticModelShader.setVec3("partAccentColor", placement.color);
            staticModelShader.setVec3("lightDir", glm::normalize(glm::vec3(-0.35f, -1.0f, -0.15f)));
            staticModelShader.setVec3("ambientColor", glm::vec3(0.72f, 0.70f, 0.82f));
            if (usePartColors)
                modelAsset.DrawPartColored(staticModelShader, oxygenPipeBodyColor, placement.color);
            else
                modelAsset.Draw(staticModelShader);
        };

    StaticModel roomLarge(std::string(PROJECT_ROOT) + "/assets/models/ModularSpaceKit/room-large.obj");
    StaticModel corridor(std::string(PROJECT_ROOT) + "/assets/models/ModularSpaceKit/corridor.obj");
    StaticModel gate(std::string(PROJECT_ROOT) + "/assets/models/ModularSpaceKit/gate.obj");
    StaticModel gateDoor(std::string(PROJECT_ROOT) + "/assets/models/ModularSpaceKit/gate-door.obj");
    StaticModel bedDouble(std::string(PROJECT_ROOT) + "/assets/models/SpaceStationKit/bed-double.obj");
    StaticModel bedDoubleCover(std::string(PROJECT_ROOT) + "/assets/models/SpaceStationKit/bed-double-cover.obj");
    StaticModel container(std::string(PROJECT_ROOT) + "/assets/models/SpaceStationKit/container.obj");
    StaticModel containerFlat(std::string(PROJECT_ROOT) + "/assets/models/SpaceStationKit/container-flat.obj");
    StaticModel containerFlatOpen(std::string(PROJECT_ROOT) + "/assets/models/SpaceStationKit/container-flat-open.obj");
    StaticModel containerTall(std::string(PROJECT_ROOT) + "/assets/models/SpaceStationKit/container-tall.obj");
    StaticModel containerWide(std::string(PROJECT_ROOT) + "/assets/models/SpaceStationKit/container-wide.obj");
    StaticModel tableDisplay(std::string(PROJECT_ROOT) + "/assets/models/SpaceStationKit/table-display.obj");
    StaticModel tableDisplaySmall(std::string(PROJECT_ROOT) + "/assets/models/SpaceStationKit/table-display-small.obj");
    StaticModel tableInset(std::string(PROJECT_ROOT) + "/assets/models/SpaceStationKit/table-inset.obj");
    StaticModel skipRocks(std::string(PROJECT_ROOT) + "/assets/models/SpaceStationKit/skip-rocks.obj");
    StaticModel rocks(std::string(PROJECT_ROOT) + "/assets/models/SpaceStationKit/rocks.obj");
    StaticModel computerScreen(std::string(PROJECT_ROOT) + "/assets/models/SpaceStationKit/computer-screen.obj");
    StaticModel computer(std::string(PROJECT_ROOT) + "/assets/models/SpaceStationKit/computer.obj");
    StaticModel computerSystem(std::string(PROJECT_ROOT) + "/assets/models/SpaceStationKit/computer-system.obj");
    StaticModel computerWide(std::string(PROJECT_ROOT) + "/assets/models/SpaceStationKit/computer-wide.obj");
    StaticModel displayWallWide(std::string(PROJECT_ROOT) + "/assets/models/SpaceStationKit/display-wall-wide.obj");
    StaticModel labPoster(std::string(PROJECT_ROOT) + "/assets/models/Puzzle/poster_lab.obj");
    StaticModel oxygenPoster(std::string(PROJECT_ROOT) + "/assets/models/Puzzle/poster_oxygen.obj");
    StaticModel powerPosterPowerOff(std::string(PROJECT_ROOT) + "/assets/models/Puzzle/poster_power_off.obj");
    StaticModel powerPosterMaintenance(std::string(PROJECT_ROOT) + "/assets/models/Puzzle/poster_maintenance.obj");
    StaticModel powerPosterCalibration(std::string(PROJECT_ROOT) + "/assets/models/Puzzle/poster_calibration.obj");
    StaticModel oxygenPipeUp(std::string(PROJECT_ROOT) + "/assets/models/Puzzle/pipe-up.obj");
    StaticModel oxygenPipeDown(std::string(PROJECT_ROOT) + "/assets/models/Puzzle/pipe-down.obj");
    StaticModel oxygenTank(std::string(PROJECT_ROOT) + "/assets/models/AdditionalAssets/oxygen_tank.obj");
    StaticModel pottedPlant(std::string(PROJECT_ROOT) + "/assets/models/AdditionalAssets/pottedPlant.obj");
    StaticModel powerBox(std::string(PROJECT_ROOT) + "/assets/models/AdditionalAssets/power_box.obj");
    StaticModel cabinet(std::string(PROJECT_ROOT) + "/assets/models/AdditionalAssets/cabinet.obj");
    const std::string puzzleTexture =
        std::string(PROJECT_ROOT) + "/assets/models/SpaceStationKit/Textures/colormap.png";
    const std::string oxygenAnimationPath =
        std::string(PROJECT_ROOT) + "/assets/animation/object/pipe-animated.fbx";
    const std::string oxygenAnimationName = "ring|pipe-ring-colored.001Action";
    std::array<std::unique_ptr<AnimatedObjectPlayer>, GameState::kOxygenValveCount> oxygenAnimatedPipes;
    std::array<bool, GameState::kOxygenValveCount> oxygenAnimatedPipeStarted{ false, false, false };
    for (int i = 0; i < GameState::kOxygenValveCount; ++i)
    {
        oxygenAnimatedPipes[i] = std::make_unique<AnimatedObjectPlayer>(
            oxygenAnimationPath,
            puzzleTexture,
            false,
            oxygenAnimationName
        );

        if (!oxygenAnimatedPipes[i]->isLoaded())
            std::cerr << "Oxygen animated pipe failed: " << oxygenAnimatedPipes[i]->getError() << "\n";
    }
    TestRoomScene testRoomScene = createTestRoomScene();

    world.buildDefaultRoom();
    world.setGameState(&gameState);

    if (kTemplateRoomTestMode)
    {
        configureTestRoomWorld(world, testRoomScene, gameState.powerFixed, gameState.controlUnlocked);
        playerPos = testRoomScene.playerStart;
    }

    bool winPrinted = false;
    bool testRoomPowerFixedState = gameState.powerFixed;
    bool testRoomControlUnlockedState = gameState.controlUnlocked;
    PlayerAnimationState currentAnimationState = PlayerAnimationState::Idle;
    std::array<bool, GameState::kOxygenValveCount> previousOxygenValveStates = gameState.oxygenValvesOpened;
    auto restartGame = [&]()
    {
        gameState = GameState{};
        world.buildDefaultRoom();
        world.setGameState(&gameState);

        if (kTemplateRoomTestMode)
        {
            configureTestRoomWorld(world, testRoomScene, gameState.powerFixed, gameState.controlUnlocked);
            playerPos = testRoomScene.playerStart;
        }
        else
        {
            playerPos = glm::vec3(0.0f, 0.0f, 0.0f);
        }

        playerYaw = 90.0f;
        cameraYaw = -90.0f;
        cameraPitch = -20.0f;
        currentCameraDistance = cameraDistance;
        firstMouse = true;

        controlCodePanelOpen = false;
        controlCodeRejected = false;
        controlCodeInput.clear();
        oxygenTerminalPanelOpen = false;
        labStabilizerPanelOpen = false;
        labStabilizerRejected = false;
        labStabilizerValues = { 0, 0, 0 };
        labStabilizerSelected = 0;
        closePowerWirePanel();

        playerIsMoving = false;
        playerIsRunning = false;
        playerDanceTriggered = false;
        playerIsDancing = false;
        deathAnimationTriggered = false;
        deathAnimationFinished = false;
        footstepTimer = 0.0f;
        stopPowerWarningBeeps();

        subtitleQueue.clear();
        currentSubtitle.clear();
        subtitleTimer = 0.0f;
        introQueued = false;
        seenOxygenFixed = false;
        seenPowerFixed = false;
        seenFoundNote = false;
        seenHasCode = false;
        seenControlUnlocked = false;
        seenGameFinished = false;
        seenPlayerDeath = false;
        endingSoundPlayed = false;
        stopHeavyBreathingLoop();
        heavyBreathingPlaying = false;

        ePressedLastFrame = false;
        onePressedLastFrame = false;
        enterPressedLastFrame = false;
        spacePressedLastFrame = false;
        backspacePressedLastFrame = false;
        escapePressedLastFrame = false;
        upPressedLastFrame = false;
        downPressedLastFrame = false;
        leftPressedLastFrame = false;
        rightPressedLastFrame = false;
        rPressedLastFrame = false;
        digitPressedLastFrame.fill(false);

        oxygenAnimatedPipeStarted.fill(false);
        previousOxygenValveStates = gameState.oxygenValvesOpened;
        winPrinted = false;
        testRoomPowerFixedState = gameState.powerFixed;
        testRoomControlUnlockedState = gameState.controlUnlocked;
        currentAnimationState = PlayerAnimationState::Idle;
        idleCharacter.update(0.0f, true);

        gameStarted = true;
        startHeavyBreathingLoop();
        std::cout << "Game restarted\n";
    };

    while (!glfwWindowShouldClose(window))
    {
        audio.update();

        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        handleFullscreenToggle(window);

        if (!gameStarted)
        {
            const bool escapePressedNow = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
            if (escapePressedNow && !escapePressedLastFrame)
                glfwSetWindowShouldClose(window, true);
            escapePressedLastFrame = escapePressedNow;

            const bool enterPressedNow = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS ||
                glfwGetKey(window, GLFW_KEY_KP_ENTER) == GLFW_PRESS;
            const bool spacePressedNow = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
            if ((enterPressedNow && !enterPressedLastFrame) || (spacePressedNow && !spacePressedLastFrame))
            {
                gameStarted = true;
                playStartSound();
                startHeavyBreathingLoop();
                enterPressedLastFrame = enterPressedNow;
                spacePressedLastFrame = spacePressedNow;
                subtitleQueue.clear();
                currentSubtitle.clear();
                subtitleTimer = 0.0f;
                std::cout << "Game started\n";
            }
            else
            {
                enterPressedLastFrame = enterPressedNow;
                spacePressedLastFrame = spacePressedNow;
            }

            glClearColor(0.01f, 0.015f, 0.03f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDisable(GL_DEPTH_TEST);
            drawStartMenu(hudShader, quadVAO, currentFrame);
            glEnable(GL_DEPTH_TEST);

            glfwSwapBuffers(window);
            glfwPollEvents();
            continue;
        }

        processInput(window);
        updateFootstepSounds();
        const bool rPressedNow = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
        if (isEndingOverlayVisible() && rPressedNow && !rPressedLastFrame)
        {
            restartGame();
            glfwPollEvents();
            continue;
        }
        rPressedLastFrame = rPressedNow;

        if (gameState.playerDied && seenPlayerDeath)
        {
            const bool deathDialogFinished = currentSubtitle.empty() && subtitleQueue.empty();
            if (deathDialogFinished && !deathAnimationTriggered)
            {
                currentAnimationState = PlayerAnimationState::Die;
                deathCharacter.update(0.0f, true);
                deathAnimationTriggered = true;
                deathAnimationFinished = false;
            }
        }
        else if (playerDanceTriggered)
        {
            currentAnimationState = PlayerAnimationState::Dance;
            danceCharacter.update(0.0f, true);
        }

        PlayerAnimationState desiredAnimationState = PlayerAnimationState::Idle;
        if (gameState.playerDied && deathAnimationTriggered)
            desiredAnimationState = PlayerAnimationState::Die;
        else if (playerIsRunning)
            desiredAnimationState = PlayerAnimationState::Run;
        else if (playerIsMoving)
            desiredAnimationState = PlayerAnimationState::Walk;

        AnimatedCharacter* previousActiveCharacter = getCharacterForState(
            currentAnimationState,
            idleCharacter,
            walkCharacter,
            runCharacter,
            danceCharacter,
            deathCharacter);

        if (currentAnimationState == PlayerAnimationState::Die)
        {
            deathCharacter.update(deltaTime);
            deathAnimationFinished = deathCharacter.isFinished();
        }
        else if (currentAnimationState == PlayerAnimationState::Dance)
        {
            danceCharacter.update(deltaTime);
            if (danceCharacter.isFinished())
                currentAnimationState = desiredAnimationState;
        }
        else
        {
            currentAnimationState = desiredAnimationState;
        }

        AnimatedCharacter* activeCharacter = getCharacterForState(
            currentAnimationState,
            idleCharacter,
            walkCharacter,
            runCharacter,
            danceCharacter,
            deathCharacter);

        if (activeCharacter != previousActiveCharacter &&
            activeCharacter &&
            previousActiveCharacter &&
            activeCharacter->isLooping() &&
            previousActiveCharacter->isLooping())
        {
            activeCharacter->setNormalizedTime(previousActiveCharacter->getNormalizedTime());
        }

        if (activeCharacter &&
            currentAnimationState != PlayerAnimationState::Dance &&
            currentAnimationState != PlayerAnimationState::Die)
            activeCharacter->update(deltaTime);

        playerIsDancing = (currentAnimationState == PlayerAnimationState::Dance);
        updateInteractPrompt();
        handleInteraction(window);
        for (int valveIndex = 0; valveIndex < GameState::kOxygenValveCount; ++valveIndex)
        {
            if (gameState.oxygenValvesOpened[valveIndex] && !previousOxygenValveStates[valveIndex])
            {
                oxygenAnimatedPipeStarted[valveIndex] = true;
                playTurnValveSound();
                if (oxygenAnimatedPipes[valveIndex] && oxygenAnimatedPipes[valveIndex]->isLoaded())
                    oxygenAnimatedPipes[valveIndex]->update(0.0f, true);
            }

            previousOxygenValveStates[valveIndex] = gameState.oxygenValvesOpened[valveIndex];

            if (oxygenAnimatedPipeStarted[valveIndex] &&
                oxygenAnimatedPipes[valveIndex] &&
                oxygenAnimatedPipes[valveIndex]->isLoaded() &&
                !oxygenAnimatedPipes[valveIndex]->isFinished())
            {
                oxygenAnimatedPipes[valveIndex]->update(deltaTime);
            }
        }

        if (kTemplateRoomTestMode &&
            (testRoomPowerFixedState != gameState.powerFixed ||
             testRoomControlUnlockedState != gameState.controlUnlocked))
        {
            configureTestRoomWorld(world, testRoomScene, gameState.powerFixed, gameState.controlUnlocked);
            testRoomPowerFixedState = gameState.powerFixed;
            testRoomControlUnlockedState = gameState.controlUnlocked;
        }
        updateStoryEvents();
        updateSubtitles();
        updateHeavyBreathingLoop();
        updatePowerWarningBeeps();

        int newRoom = world.getCurrentRoomIndex(playerPos);

        if (newRoom != gameState.currentRoom)
        {
            gameState.currentRoom = newRoom;

            if (newRoom != -1)
            {
                Room& room = world.rooms[newRoom];

                std::cout << "Entered: " << room.name << std::endl;

                if (!room.visited)
                {
                    room.visited = true;
                    std::cout << "First time in " << room.name << std::endl;
                }
            }
        }

        if (gameState.gameFinished && !gameState.playerDied && !winPrinted)
        {
            std::cout << "YOU WIN\n";
            winPrinted = true;
        }

        glm::vec3 cameraForward3D = getCameraForward3D();
        glm::vec3 cameraTarget = playerPos + glm::vec3(0.0f, cameraHeightOffset, 0.0f);
        glm::vec3 cameraBackward = -cameraForward3D;
        float targetCameraDistance = resolveCameraCollisionDistance(cameraTarget, cameraBackward, cameraDistance);
        if (targetCameraDistance < currentCameraDistance)
        {
            currentCameraDistance = targetCameraDistance;
        }
        else
        {
            const float cameraReturnAlpha = 1.0f - std::exp(-10.0f * deltaTime);
            currentCameraDistance = glm::mix(currentCameraDistance, targetCameraDistance, cameraReturnAlpha);
        }
        glm::vec3 cameraPos = cameraTarget + cameraBackward * currentCameraDistance;
        cameraPos.y = glm::max(cameraPos.y, cameraMinHeight);

        glClearColor(0.03f, 0.03f, 0.07f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);

        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
        staticModelShader.use();
        staticModelShader.setMat4("view", view);
        staticModelShader.setMat4("projection", projection);

        const int currentObjectiveIndex = world.getCurrentObjectiveInteractableIndex();
        const std::string currentObjectiveId = currentObjectiveIndex != -1
            ? world.interactables[currentObjectiveIndex].name
            : "";

        const float pulse = 0.55f + 0.45f * std::sin(currentFrame * 4.5f);
        const float flicker = 0.65f + 0.35f * std::sin(currentFrame * 18.0f);

        if (kTemplateRoomTestMode)
        {
            for (const auto& roomPlacement : testRoomScene.roomPlacements)
                drawStaticModel(roomLarge, roomPlacement);
            for (const auto& corridorPlacement : testRoomScene.corridorPlacements)
                drawStaticModel(corridor, corridorPlacement);
            for (const auto& gatePlacement : testRoomScene.gatePlacements)
                drawStaticModel(gate, gatePlacement);
            for (const auto& gateDoorPlacement : testRoomScene.gateDoorPlacements)
                drawStaticModel(gateDoor, gateDoorPlacement);
            for (const auto& powerUnlockGatePlacement : testRoomScene.powerUnlockGatePlacements)
            {
                if (gameState.powerFixed)
                    drawStaticModel(gate, powerUnlockGatePlacement);
                else
                    drawStaticModel(gateDoor, powerUnlockGatePlacement);
            }
            for (const auto& controlUnlockGatePlacement : testRoomScene.controlUnlockGatePlacements)
            {
                if (gameState.controlUnlocked)
                    drawStaticModel(gate, controlUnlockGatePlacement);
                else
                    drawStaticModel(gateDoor, controlUnlockGatePlacement, false, true);
            }
            for (const auto& bedPlacement : testRoomScene.bedPlacements)
                drawStaticModel(bedDouble, bedPlacement);
            for (const auto& bedCoverPlacement : testRoomScene.bedCoverPlacements)
                drawStaticModel(bedDoubleCover, bedCoverPlacement);
            for (const auto& placement : testRoomScene.storageContainerPlacements)
                drawStaticModel(container, placement);
            for (const auto& placement : testRoomScene.storageContainerFlatPlacements)
                drawStaticModel(containerFlat, placement);
            for (const auto& placement : testRoomScene.storageContainerFlatOpenPlacements)
                drawStaticModel(containerFlatOpen, placement);
            for (const auto& placement : testRoomScene.storageContainerTallPlacements)
                drawStaticModel(containerTall, placement);
            for (const auto& placement : testRoomScene.storageContainerWidePlacements)
                drawStaticModel(containerWide, placement);
            for (const auto& placement : testRoomScene.powerCabinetPlacements)
                drawStaticModel(cabinet, placement);
            for (const auto& placement : testRoomScene.powerComputerSystemPlacements)
                drawStaticModel(computerSystem, placement);
            drawStaticModel(powerPosterPowerOff, testRoomScene.powerPosterPowerOffPlacement);
            drawStaticModel(powerPosterMaintenance, testRoomScene.powerPosterMaintenancePlacement);
            drawStaticModel(powerPosterCalibration, testRoomScene.powerPosterCalibrationPlacement);
            drawStaticModel(oxygenPoster, testRoomScene.oxygenPosterPlacement);
            drawStaticModel(computer, testRoomScene.oxygenComputerPlacement);
            for (const auto& placement : testRoomScene.oxygenTankPlacements)
                drawStaticModel(oxygenTank, placement);
            for (const auto& placement : testRoomScene.oxygenPlantPlacements)
                drawStaticModel(pottedPlant, placement);
            for (int valveIndex = 0; valveIndex < static_cast<int>(testRoomScene.oxygenValvePlacements.size()); ++valveIndex)
            {
                ModelPlacement valvePlacement = testRoomScene.oxygenValvePlacements[valveIndex];
                const bool hasAnimatedPipe =
                    oxygenAnimatedPipes[valveIndex] &&
                    oxygenAnimatedPipes[valveIndex]->isLoaded();
                const bool animationStarted = oxygenAnimatedPipeStarted[valveIndex];
                const bool animationFinished =
                    animationStarted &&
                    oxygenAnimatedPipes[valveIndex] &&
                    oxygenAnimatedPipes[valveIndex]->isFinished();

                if (gameState.playerDied)
                {
                    valvePlacement.color = glm::vec3(1.0f, 0.18f, 0.18f);
                }

                if (!animationStarted)
                {
                    drawStaticModel(oxygenPipeUp, valvePlacement, true);
                }
                else if (hasAnimatedPipe && !animationFinished)
                {
                    glm::mat4 oxygenPipeBaseMatrix = glm::mat4(1.0f);
                    oxygenPipeBaseMatrix = glm::translate(oxygenPipeBaseMatrix, valvePlacement.position);
                    oxygenPipeBaseMatrix = glm::rotate(oxygenPipeBaseMatrix, glm::radians(valvePlacement.rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
                    oxygenPipeBaseMatrix = glm::scale(oxygenPipeBaseMatrix, valvePlacement.scale);
                    oxygenAnimatedPipes[valveIndex]->draw(
                        characterShader,
                        view,
                        projection,
                        oxygenPipeBaseMatrix,
                        valvePlacement.color
                    );
                }
                else
                {
                    StaticModel& oxygenPipeModel = gameState.oxygenValvesOpened[valveIndex]
                        ? oxygenPipeDown
                        : oxygenPipeUp;
                    drawStaticModel(oxygenPipeModel, valvePlacement, true);
                }
            }
            for (const auto& labTableDisplayPlacement : testRoomScene.labTableDisplayPlacements)
                drawStaticModel(tableDisplay, labTableDisplayPlacement);
            for (const auto& placement : testRoomScene.labSmallTableDisplayPlacements)
                drawStaticModel(tableDisplaySmall, placement);
            for (const auto& placement : testRoomScene.labTableInsetPlacements)
                drawStaticModel(tableInset, placement);
            drawStaticModel(computer, testRoomScene.labComputerPlacement);
            drawStaticModel(computerScreen, testRoomScene.labComputerScreenPlacement);
            drawStaticModel(skipRocks, testRoomScene.labSkipRocksPlacement);
            drawStaticModel(rocks, testRoomScene.labRocksPlacement);
            drawStaticModel(rocks, testRoomScene.labCenterRocksPlacement);
            drawStaticModel(labPoster, testRoomScene.labPosterPlacement);
            drawStaticModel(computerScreen, testRoomScene.controlTerminalPlacement);
            for (const auto& placement : testRoomScene.controlComputerPlacements)
                drawStaticModel(computer, placement);
            for (const auto& placement : testRoomScene.controlComputerWidePlacements)
                drawStaticModel(computerWide, placement);
            for (const auto& placement : testRoomScene.controlDisplayWallWidePlacements)
                drawStaticModel(displayWallWide, placement);

            auto drawInteractableCube = [&](const std::string& id, const ModelPlacement& placement, const glm::vec3& color)
            {
                glm::vec3 drawColor = color;
                if (id == currentObjectiveId)
                    drawColor = glm::min(drawColor + glm::vec3(0.18f + 0.30f * pulse), glm::vec3(1.0f));

                drawCube(placement.position, placement.scale, drawColor, placement.rotationY);
            };

            ModelPlacement powerBoxPlacement = testRoomScene.powerConsolePlacement;
            powerBoxPlacement.color = gameState.powerFixed
                ? glm::vec3(0.76f, 1.10f, 0.82f)
                : glm::vec3(1.0f);
            if (currentObjectiveId == "power_console")
                powerBoxPlacement.color = glm::min(powerBoxPlacement.color + glm::vec3(0.10f + 0.16f * pulse), glm::vec3(1.25f));
            drawStaticModel(powerBox, powerBoxPlacement);

        }
        else
        {
            for (const auto& obj : world.staticObjects)
            {
                glm::vec3 drawColor = obj.color;

                if (obj.id == "oxygen_console")
                {
                    drawColor = gameState.oxygenFixed
                        ? glm::vec3(0.2f, 1.0f, 0.2f)
                        : glm::vec3(0.8f, 0.8f, 0.2f);
                }

                if (obj.id == "power_console")
                {
                    drawColor = gameState.powerFixed
                        ? glm::vec3(0.25f, 1.0f, 0.35f)
                        : glm::vec3(0.20f, 0.55f, 1.0f);
                }

                if (!gameState.powerFixed &&
                    (obj.id.find("floor_storage") != std::string::npos ||
                     obj.id.find("floor_lab") != std::string::npos ||
                     obj.id.find("floor_control") != std::string::npos ||
                     obj.id == "control_terminal" ||
                     obj.id == "control_beacon_left" ||
                     obj.id == "control_beacon_right"))
                {
                    drawColor *= 0.35f;
                }

                if (obj.id == currentObjectiveId)
                {
                    drawColor = glm::min(drawColor + glm::vec3(0.18f + 0.30f * pulse), glm::vec3(1.0f));
                }

                if ((obj.id == "control_beacon_left" || obj.id == "control_beacon_right") && gameState.powerFixed)
                {
                    drawColor = glm::vec3(0.75f, 0.18f, 0.35f);
                }

                drawCube(obj.pos, obj.scale, drawColor, obj.rotationY);
            }

            for (const auto& door : world.doors)
            {
                glm::vec3 doorScale = door.halfSize * 2.0f;

                if (!door.open)
                {
                    glm::vec3 closedColor = door.closedColor;
                    glm::vec3 closedScale = doorScale * 1.2f;

                    if (door.name == "Control Door" && gameState.hasCode)
                    {
                        closedColor = glm::mix(door.closedColor, door.openColor, 0.35f + 0.35f * pulse);
                    }

                    drawCube(door.center, closedScale, closedColor, 0.0f);
                }
                else
                {
                    if (door.rotationY == 0.0f)
                    {
                        drawCube(
                            door.center + glm::vec3(-door.halfSize.x * 0.95f, 0.0f, 0.0f),
                            glm::vec3(door.halfSize.x, doorScale.y, doorScale.z),
                            door.openColor,
                            door.rotationY
                        );
                        drawCube(
                            door.center + glm::vec3(door.halfSize.x * 0.95f, 0.0f, 0.0f),
                            glm::vec3(door.halfSize.x, doorScale.y, doorScale.z),
                            door.openColor,
                            door.rotationY
                        );
                    }
                    else
                    {
                        drawCube(
                            door.center + glm::vec3(0.0f, 0.0f, -door.halfSize.z * 0.95f),
                            glm::vec3(doorScale.x, doorScale.y, door.halfSize.z),
                            door.openColor,
                            0.0f
                        );
                        drawCube(
                            door.center + glm::vec3(0.0f, 0.0f, door.halfSize.z * 0.95f),
                            glm::vec3(doorScale.x, doorScale.y, door.halfSize.z),
                            door.openColor,
                            0.0f
                        );
                    }
                }
            }
        }

        drawCollisionDebug();

        if (activeCharacter)
        {
            activeCharacter->draw(characterShader, view, projection, playerPos, playerYaw);
        }
        else
        {
            drawCube(
                playerPos + glm::vec3(0.0f, 0.25f, 0.0f),
                glm::vec3(0.6f, 1.0f, 0.6f),
                glm::vec3(0.9f, 0.9f, 0.95f),
                playerYaw
            );

            glm::mat4 playerRot = glm::rotate(glm::mat4(1.0f), glm::radians(playerYaw), glm::vec3(0.0f, 1.0f, 0.0f));
            glm::vec3 localMarkerOffset(0.45f, 0.7f, 0.0f);
            glm::vec3 rotatedOffset = glm::vec3(playerRot * glm::vec4(localMarkerOffset, 0.0f));
            glm::vec3 markerPos = playerPos + rotatedOffset;

            drawCube(
                markerPos,
                glm::vec3(0.18f, 0.18f, 0.18f),
                glm::vec3(1.0f, 0.3f, 0.3f),
                playerYaw
            );
        }

        glDisable(GL_DEPTH_TEST);
        drawProgressAndObjective(gameState, hudShader, quadVAO);
        drawInteractPrompt(hudShader, quadVAO);
        drawSubtitle(hudShader, quadVAO);
        drawControlCodePanel(hudShader, quadVAO);
        drawOxygenTerminalPanel(hudShader, quadVAO);
        drawPowerWirePanel(hudShader, quadVAO);
        drawLabStabilizerPanel(hudShader, quadVAO);
        drawEndingOverlay(hudShader, quadVAO);
        if (!endingSoundPlayed && isEndingOverlayVisible())
        {
            if (gameState.playerDied)
                playLoseSound();
            else
                playWinSound();
            endingSoundPlayed = true;
        }
        glEnable(GL_DEPTH_TEST);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);

    audio.shutdown();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

