//
//  TemplateDB.cpp
//  game_engine
//
//  Created by Armand Gago on 1/29/26.
//

#include "TemplateDB.hpp"

#include <filesystem>
#include <string>

#include "EngineUtils.hpp"

namespace fs = std::filesystem;

void TemplateDB::loadTemplates(const std::string& actorTemplatesPath) {
    if (!fs::exists(actorTemplatesPath) || !fs::is_directory(actorTemplatesPath)) {
        return;
    }

    for (const auto& entry : fs::directory_iterator(actorTemplatesPath)) {
        if (entry.is_regular_file()) {
            const std::string pathStr = entry.path().string();
            const std::string filename = entry.path().filename().string();

            if (filename.size() > 9 && filename.substr(filename.size() - 9) == ".template") {
                std::string name = filename.substr(0, filename.size() - 9);

                rapidjson::Document doc;
                EngineUtils::ReadJsonFile(pathStr, doc);

                templates.emplace(std::move(name), std::move(doc));
            }
        }
    }
}

const rapidjson::Value* TemplateDB::getTemplate(const std::string& name) const {
    auto it = templates.find(name);
    if (it != templates.end()) {
        return &it->second;
    }
    return nullptr;
}

bool TemplateDB::hasTemplate(const std::string& name) const {
    return templates.find(name) != templates.end();
}
