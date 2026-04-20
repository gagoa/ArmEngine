//
//  TemplateDB.hpp
//  game_engine
//
//  Created by Armand Gago on 1/29/26.
//

#ifndef TemplateDB_hpp
#define TemplateDB_hpp

#include <string>
#include <unordered_map>

#include "rapidjson/document.h"

class TemplateDB {
public:
    void loadTemplates(const std::string& actorTemplatesPath);
    const rapidjson::Value* getTemplate(const std::string& name) const;
    bool hasTemplate(const std::string& name) const;

private:
    std::unordered_map<std::string, rapidjson::Document> templates;
};

#endif /* TemplateDB_hpp */
