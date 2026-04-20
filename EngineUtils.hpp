//
//  EngineUtils.hpp
//  game_engine
//
//  Created by Armand Gago on 1/25/26.
//

#ifndef EngineUtils_hpp
#define EngineUtils_hpp

#include <stdio.h>
#include <string>
#include <fstream>
#include <sstream>
#include <stdio.h>

#include "rapidjson/filereadstream.h"
#include "rapidjson/document.h"

class EngineUtils {
public:
    static void ReadJsonFile(const std::string& path, rapidjson::Document& out_document);
};


#endif /* EngineUtils_hpp */
