#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <miniaudio.h>

class AudioEngine
{
public:
    AudioEngine() = default;
    ~AudioEngine();

    bool init();
    void shutdown();
    void setVolume(float volume);
    void update();
    bool preloadClip(const std::string& id, const std::string& path, int instanceCount);
    void playClip(const std::string& id, float volume = 1.0f);
    void playOneShot(const std::string& path, float volume = 1.0f);
    bool playLoop(const std::string& id, const std::string& path, float volume = 1.0f);
    void stopLoop(const std::string& id);
    void setLoopVolume(const std::string& id, float volume);

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

private:
    struct ClipPool
    {
        std::vector<std::unique_ptr<ma_sound>> sounds;
        size_t nextSound = 0;
    };

    void clearFinishedSounds();

    ma_engine engine{};
    std::unordered_map<std::string, ClipPool> clipPools;
    std::unordered_map<std::string, std::unique_ptr<ma_sound>> loopSounds;
    std::vector<std::unique_ptr<ma_sound>> activeSounds;
    bool initialized = false;
};
