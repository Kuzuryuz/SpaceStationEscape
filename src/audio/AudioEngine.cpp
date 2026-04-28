#define MINIAUDIO_IMPLEMENTATION
#include "audio/AudioEngine.h"

#include <algorithm>
#include <iostream>

AudioEngine::~AudioEngine()
{
    shutdown();
}

bool AudioEngine::init()
{
    if (initialized)
        return true;

    if (ma_engine_init(nullptr, &engine) != MA_SUCCESS)
    {
        std::cerr << "Failed to initialize audio engine\n";
        return false;
    }

    initialized = true;
    return true;
}

void AudioEngine::shutdown()
{
    if (!initialized)
        return;

    for (auto& clip : clipPools)
    {
        for (auto& sound : clip.second.sounds)
            ma_sound_uninit(sound.get());
    }
    clipPools.clear();

    for (auto& loop : loopSounds)
        ma_sound_uninit(loop.second.get());
    loopSounds.clear();

    for (auto& sound : activeSounds)
        ma_sound_uninit(sound.get());
    activeSounds.clear();

    ma_engine_uninit(&engine);
    initialized = false;
}

void AudioEngine::setVolume(float volume)
{
    if (!initialized)
        return;

    ma_engine_set_volume(&engine, volume);
}

void AudioEngine::update()
{
    clearFinishedSounds();
}

bool AudioEngine::preloadClip(const std::string& id, const std::string& path, int instanceCount)
{
    if (!initialized || instanceCount <= 0)
        return false;

    ClipPool pool;
    pool.sounds.reserve(static_cast<size_t>(instanceCount));

    for (int i = 0; i < instanceCount; ++i)
    {
        auto sound = std::make_unique<ma_sound>();
        if (ma_sound_init_from_file(&engine, path.c_str(), MA_SOUND_FLAG_DECODE, nullptr, nullptr, sound.get()) != MA_SUCCESS)
        {
            std::cerr << "Failed to preload sound: " << path << "\n";
            for (auto& loadedSound : pool.sounds)
                ma_sound_uninit(loadedSound.get());
            return false;
        }

        pool.sounds.push_back(std::move(sound));
    }

    clipPools[id] = std::move(pool);
    return true;
}

void AudioEngine::playClip(const std::string& id, float volume)
{
    if (!initialized)
        return;

    auto clipIt = clipPools.find(id);
    if (clipIt == clipPools.end() || clipIt->second.sounds.empty())
        return;

    ClipPool& pool = clipIt->second;
    size_t selectedIndex = pool.nextSound;
    for (size_t i = 0; i < pool.sounds.size(); ++i)
    {
        const size_t candidateIndex = (pool.nextSound + i) % pool.sounds.size();
        if (!ma_sound_is_playing(pool.sounds[candidateIndex].get()))
        {
            selectedIndex = candidateIndex;
            break;
        }
    }

    pool.nextSound = (selectedIndex + 1) % pool.sounds.size();

    ma_sound* sound = pool.sounds[selectedIndex].get();
    ma_sound_stop(sound);
    ma_sound_seek_to_pcm_frame(sound, 0);
    ma_sound_set_volume(sound, volume);
    ma_sound_start(sound);
}

void AudioEngine::playOneShot(const std::string& path, float volume)
{
    if (!initialized)
        return;

    clearFinishedSounds();

    auto sound = std::make_unique<ma_sound>();
    if (ma_sound_init_from_file(&engine, path.c_str(), 0, nullptr, nullptr, sound.get()) != MA_SUCCESS)
    {
        std::cerr << "Failed to load sound: " << path << "\n";
        return;
    }

    ma_sound_set_volume(sound.get(), volume);
    if (ma_sound_start(sound.get()) != MA_SUCCESS)
    {
        std::cerr << "Failed to play sound: " << path << "\n";
        ma_sound_uninit(sound.get());
        return;
    }

    activeSounds.push_back(std::move(sound));
}

bool AudioEngine::playLoop(const std::string& id, const std::string& path, float volume)
{
    if (!initialized)
        return false;

    stopLoop(id);

    auto sound = std::make_unique<ma_sound>();
    if (ma_sound_init_from_file(&engine, path.c_str(), 0, nullptr, nullptr, sound.get()) != MA_SUCCESS)
    {
        std::cerr << "Failed to load loop sound: " << path << "\n";
        return false;
    }

    ma_sound_set_looping(sound.get(), MA_TRUE);
    ma_sound_set_volume(sound.get(), volume);
    if (ma_sound_start(sound.get()) != MA_SUCCESS)
    {
        std::cerr << "Failed to play loop sound: " << path << "\n";
        ma_sound_uninit(sound.get());
        return false;
    }

    loopSounds[id] = std::move(sound);
    return true;
}

void AudioEngine::stopLoop(const std::string& id)
{
    auto loopIt = loopSounds.find(id);
    if (loopIt == loopSounds.end())
        return;

    ma_sound_stop(loopIt->second.get());
    ma_sound_uninit(loopIt->second.get());
    loopSounds.erase(loopIt);
}

void AudioEngine::setLoopVolume(const std::string& id, float volume)
{
    auto loopIt = loopSounds.find(id);
    if (loopIt == loopSounds.end())
        return;

    ma_sound_set_volume(loopIt->second.get(), volume);
}

void AudioEngine::clearFinishedSounds()
{
    activeSounds.erase(
        std::remove_if(
            activeSounds.begin(),
            activeSounds.end(),
            [](const std::unique_ptr<ma_sound>& sound)
            {
                if (ma_sound_at_end(sound.get()))
                {
                    ma_sound_uninit(sound.get());
                    return true;
                }

                return false;
            }),
        activeSounds.end());
}
