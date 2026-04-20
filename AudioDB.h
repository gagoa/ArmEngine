//
//  AudioDB.h
//  game_engine
//
//  Created by Armand Gago on 2/4/26.
//

#ifndef AudioDB_h
#define AudioDB_h

#include <filesystem>
#include <string>
#include <unordered_map>

#include "AudioHelper.h"

namespace fs = std::filesystem;

class AudioDB {
public:
    void initAudio() {
        if (AudioHelper::Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
            std::cout << "error initializing audio " << Mix_GetError();
        }
        AudioHelper::Mix_AllocateChannels(50);
    }

    Mix_Chunk* loadAudio(const std::string& clipName) {
        auto it = audioCache.find(clipName);
        if (it != audioCache.end()) return it->second;
        Mix_Chunk* chunk = tryLoadAudioFile(clipName);
        audioCache[clipName] = chunk;
        return chunk;
    }

    std::unordered_map<std::string, Mix_Chunk*> audioCache;

private:
    Mix_Chunk* tryLoadAudioFile(const std::string& baseName) {
        fs::path wavPath = fs::path("resources") / "audio" / baseName;
        wavPath.replace_extension(".wav");
        if (fs::exists(wavPath)) {
            return AudioHelper::Mix_LoadWAV(wavPath.string().c_str());
        }
        fs::path oggPath = fs::path("resources") / "audio" / baseName;
        oggPath.replace_extension(".ogg");
        if (fs::exists(oggPath)) {
            return AudioHelper::Mix_LoadWAV(oggPath.string().c_str());
        }
        return nullptr;
    }
};

#endif /* AudioDB_h */
