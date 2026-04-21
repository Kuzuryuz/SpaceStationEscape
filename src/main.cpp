#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <cmath>
#include <string>
#include <array>
#include <vector>

#include "graphics/AnimatedCharacter.h"
#include "graphics/Shader.h"
#include "world/World.h"
#include "GameState.h"

static const unsigned int SCR_WIDTH = 1280;
static const unsigned int SCR_HEIGHT = 720;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

glm::vec3 playerPos(0.0f, 0.0f, 0.0f);
float walkSpeed = 3.5f;
float runSpeed = 6.0f;
float playerRadius = 0.35f;
float playerYaw = 90.0f;

float cameraYaw = -90.0f;
float cameraPitch = -20.0f;
float cameraDistance = 4.5f;
float cameraHeightOffset = 1.5f;
float mouseSensitivity = 0.12f;

bool firstMouse = true;
float lastMouseX = SCR_WIDTH * 0.5f;
float lastMouseY = SCR_HEIGHT * 0.5f;

bool ePressedLastFrame = false;
bool onePressedLastFrame = false;
bool showInteractPrompt = false;
int nearestInteractableIndex = -1;
bool playerIsMoving = false;
bool playerIsRunning = false;
bool playerDanceTriggered = false;
bool playerIsDancing = false;

enum class PlayerAnimationState
{
    Idle,
    Walk,
    Run,
    Dance
};

AnimatedCharacter* getCharacterForState(
    PlayerAnimationState state,
    AnimatedCharacter& idleCharacter,
    AnimatedCharacter& walkCharacter,
    AnimatedCharacter& runCharacter,
    AnimatedCharacter& danceCharacter)
{
    switch (state)
    {
    case PlayerAnimationState::Walk:
        return walkCharacter.isLoaded() ? &walkCharacter : nullptr;
    case PlayerAnimationState::Run:
        return runCharacter.isLoaded() ? &runCharacter : nullptr;
    case PlayerAnimationState::Dance:
        return danceCharacter.isLoaded() ? &danceCharacter : nullptr;
    case PlayerAnimationState::Idle:
    default:
        return idleCharacter.isLoaded() ? &idleCharacter : nullptr;
    }
}

World world;
GameState gameState;

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

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
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
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    const bool onePressedNow = glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS;
    playerDanceTriggered = onePressedNow && !onePressedLastFrame;
    onePressedLastFrame = onePressedNow;

    if (gameState.gameFinished)
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

void updateSubtitles()
{
    if (currentSubtitle.empty() && !subtitleQueue.empty())
    {
        currentSubtitle = subtitleQueue.front().text;
        subtitleTimer = subtitleQueue.front().duration;
        subtitleQueue.erase(subtitleQueue.begin());
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
        queueSubtitle("WARNING THE STATION HAS BEEN DAMAGED", 3.2f);
        queueSubtitle("OXYGEN LEVELS ARE FALLING FAST", 3.2f);
        queueSubtitle("REACH THE OXYGEN CONSOLE AND REPAIR IT NOW", 3.4f);
    }

    if (gameState.oxygenFixed && !seenOxygenFixed)
    {
        seenOxygenFixed = true;
        queueSubtitle("OXYGEN FLOW IS STABLE AGAIN", 2.8f);
        queueSubtitle("WAIT THE POWER SYSTEM IS FAILING", 3.0f);
        queueSubtitle("RESTORE POWER BEFORE THE STATION SHUTS DOWN", 3.2f);
    }

    if (gameState.powerFixed && !seenPowerFixed)
    {
        seenPowerFixed = true;
        queueSubtitle("POWER HAS BEEN RESTORED", 2.8f);
        queueSubtitle("STORAGE AND LAB ACCESS ARE BACK ONLINE", 3.0f);
        queueSubtitle("THE CONTROL ROOM MAY NEED A CODE", 3.0f);
        queueSubtitle("SEARCH STORAGE FOR ANY CLUE", 3.0f);
    }

    if (gameState.foundNote && !seenFoundNote)
    {
        seenFoundNote = true;
        queueSubtitle("THIS NOTE IS BLANK", 2.5f);
        queueSubtitle("THERE MUST BE A WAY TO REVEAL THE MESSAGE", 3.0f);
        queueSubtitle("CHECK THE LAB FOR SOMETHING USEFUL", 3.0f);
    }

    if (gameState.hasCode && !seenHasCode)
    {
        seenHasCode = true;
        queueSubtitle("THE CODE IS REVEALED", 2.5f);
        queueSubtitle("GET TO THE CONTROL ROOM NOW", 2.8f);
        queueSubtitle("ENTER THE CODE TO STOP THE DAMAGE AND OPEN THE EXIT", 3.8f);
    }

    if (gameState.controlUnlocked && !seenControlUnlocked)
    {
        seenControlUnlocked = true;
        queueSubtitle("CONTROL ROOM ACCESS GRANTED", 2.7f);
        queueSubtitle("THE ESCAPE DOOR IS OPEN", 2.7f);
        queueSubtitle("MOVE TO THE FINAL SWITCH AND GET OUT", 3.0f);
    }

    if (gameState.gameFinished && !seenGameFinished)
    {
        seenGameFinished = true;
        queueSubtitle("THE EXIT IS OPEN", 2.4f);
        queueSubtitle("YOU MADE IT OUT ALIVE", 3.0f);
    }
}

std::string getObjectiveText()
{
    if (!gameState.oxygenFixed) return "FIX OXYGEN SYSTEM";
    if (!gameState.powerFixed) return "RESTORE POWER";
    if (!gameState.foundNote) return "SEARCH STORAGE";
    if (!gameState.hasCode) return "CHECK LAB";
    if (!gameState.controlUnlocked) return "UNLOCK CONTROL ROOM";
    if (!gameState.gameFinished) return "ACTIVATE ESCAPE TERMINAL";
    return "MISSION COMPLETE";
}

void updateInteractPrompt()
{
    nearestInteractableIndex = world.getNearestObjectiveInteractableIndex(playerPos);
    showInteractPrompt = (nearestInteractableIndex != -1) && !gameState.gameFinished;
}

void handleInteraction(GLFWwindow* window)
{
    bool ePressedNow = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;

    if (ePressedNow && !ePressedLastFrame && !gameState.gameFinished)
    {
        world.tryInteract(playerPos);
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
    hudShader.setVec2("screenSize", glm::vec2((float)SCR_WIDTH, (float)SCR_HEIGHT));
    glBindVertexArray(quadVAO);

    const float panelX = 16.0f;
    const float panelY = 16.0f;
    const float panelW = 400.0f;
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
    const float panelX = (SCR_WIDTH - panelW) * 0.5f;
    const float panelY = SCR_HEIGHT - 120.0f;

    hudShader.use();
    hudShader.setVec2("screenSize", glm::vec2((float)SCR_WIDTH, (float)SCR_HEIGHT));
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
    const float panelX = (SCR_WIDTH - panelW) * 0.5f;
    const float panelY = SCR_HEIGHT - 220.0f;

    hudShader.use();
    hudShader.setVec2("screenSize", glm::vec2((float)SCR_WIDTH, (float)SCR_HEIGHT));
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
    if (!gameState.gameFinished)
        return;

    hudShader.use();
    hudShader.setVec2("screenSize", glm::vec2((float)SCR_WIDTH, (float)SCR_HEIGHT));
    glBindVertexArray(quadVAO);

    drawRectHUD(hudShader, quadVAO, 0.0f, 0.0f, (float)SCR_WIDTH, (float)SCR_HEIGHT, glm::vec3(0.01f, 0.02f, 0.03f));

    const std::string title = "MISSION COMPLETE";
    const std::string subtitle = "YOU ESCAPED";
    const std::string hint = "PRESS ESC TO EXIT";

    float titlePixel = 6.0f;
    float subtitlePixel = 4.0f;
    float hintPixel = 2.6f;

    drawTextHUD(
        hudShader,
        quadVAO,
        title,
        (SCR_WIDTH - getTextWidth(title, titlePixel, titlePixel)) * 0.5f,
        240.0f,
        titlePixel,
        glm::vec3(0.85f, 0.97f, 1.0f)
    );

    drawTextHUD(
        hudShader,
        quadVAO,
        subtitle,
        (SCR_WIDTH - getTextWidth(subtitle, subtitlePixel, subtitlePixel)) * 0.5f,
        320.0f,
        subtitlePixel,
        glm::vec3(0.35f, 1.0f, 0.65f)
    );

    drawTextHUD(
        hudShader,
        quadVAO,
        hint,
        (SCR_WIDTH - getTextWidth(hint, hintPixel, hintPixel)) * 0.5f,
        392.0f,
        hintPixel,
        glm::vec3(0.80f, 0.86f, 0.92f)
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

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Space Station Escape", nullptr, nullptr);
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

    glEnable(GL_DEPTH_TEST);

    Shader shader(
        std::string(PROJECT_ROOT) + "/shaders/basic.vs",
        std::string(PROJECT_ROOT) + "/shaders/basic.fs"
    );

    Shader characterShader(
        std::string(PROJECT_ROOT) + "/shaders/character.vs",
        std::string(PROJECT_ROOT) + "/shaders/character.fs"
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

    if (!idleCharacter.isLoaded())
        std::cerr << "Idle character failed: " << idleCharacter.getError() << "\n";
    if (!walkCharacter.isLoaded())
        std::cerr << "Walk character failed: " << walkCharacter.getError() << "\n";
    if (!runCharacter.isLoaded())
        std::cerr << "Run character failed: " << runCharacter.getError() << "\n";
    if (!danceCharacter.isLoaded())
        std::cerr << "Dance character failed: " << danceCharacter.getError() << "\n";

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

    auto drawCube = [&](glm::vec3 position, glm::vec3 scale, glm::vec3 color, float rotationY = 0.0f)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, position);
            model = glm::rotate(model, glm::radians(rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, scale);

            shader.setMat4("model", model);
            shader.setVec3("objectColor", color);

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        };

    world.buildDefaultRoom();
    world.setGameState(&gameState);

    bool winPrinted = false;
    PlayerAnimationState currentAnimationState = PlayerAnimationState::Idle;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        if (playerDanceTriggered)
        {
            currentAnimationState = PlayerAnimationState::Dance;
            danceCharacter.update(0.0f, true);
        }

        PlayerAnimationState desiredAnimationState = PlayerAnimationState::Idle;
        if (playerIsRunning)
            desiredAnimationState = PlayerAnimationState::Run;
        else if (playerIsMoving)
            desiredAnimationState = PlayerAnimationState::Walk;

        AnimatedCharacter* previousActiveCharacter = getCharacterForState(
            currentAnimationState,
            idleCharacter,
            walkCharacter,
            runCharacter,
            danceCharacter);

        if (currentAnimationState == PlayerAnimationState::Dance)
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
            danceCharacter);

        if (activeCharacter != previousActiveCharacter &&
            activeCharacter &&
            previousActiveCharacter &&
            activeCharacter->isLooping() &&
            previousActiveCharacter->isLooping())
        {
            activeCharacter->setNormalizedTime(previousActiveCharacter->getNormalizedTime());
        }

        if (activeCharacter && currentAnimationState != PlayerAnimationState::Dance)
            activeCharacter->update(deltaTime);

        playerIsDancing = (currentAnimationState == PlayerAnimationState::Dance);
        updateInteractPrompt();
        handleInteraction(window);
        updateStoryEvents();
        updateSubtitles();

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

        if (gameState.gameFinished && !winPrinted)
        {
            std::cout << "YOU WIN\n";
            winPrinted = true;
        }

        glm::vec3 cameraForward3D = getCameraForward3D();
        glm::vec3 cameraTarget = playerPos + glm::vec3(0.0f, cameraHeightOffset, 0.0f);
        glm::vec3 cameraPos = cameraTarget - cameraForward3D * cameraDistance;

        glClearColor(0.03f, 0.03f, 0.07f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

        shader.setMat4("view", view);
        shader.setMat4("projection", projection);

        const int currentObjectiveIndex = world.getCurrentObjectiveInteractableIndex();
        const std::string currentObjectiveId = currentObjectiveIndex != -1
            ? world.interactables[currentObjectiveIndex].name
            : "";

        const float pulse = 0.55f + 0.45f * std::sin(currentFrame * 4.5f);
        const float flicker = 0.65f + 0.35f * std::sin(currentFrame * 18.0f);

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
        drawEndingOverlay(hudShader, quadVAO);
        glEnable(GL_DEPTH_TEST);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
