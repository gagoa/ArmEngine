//
//  EngineUtils.cpp
//  game_engine
//
//  Created by Armand Gago on 1/25/26.
//

#include "EngineUtils.hpp"
#include <stdio.h>
#include <string>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <iostream>

void EngineUtils::ReadJsonFile(const std::string& path, rapidjson::Document & out_document)
{
    FILE* file_pointer = nullptr;
#ifdef _WIN32
    fopen_s(&file_pointer, path.c_str(), "rb");
#else
    file_pointer = fopen(path.c_str(), "rb");
#endif
    char buffer[65536];
    rapidjson::FileReadStream stream(file_pointer, buffer, sizeof(buffer));
    out_document.ParseStream(stream);
    std::fclose(file_pointer);

    if (out_document.HasParseError()) {
        // rapidjson::ParseErrorCode errorCode = out_document.GetParseError();
        std::cout << "error parsing json at [" << path << "]" << std::endl;
        exit(0);
    }
}
