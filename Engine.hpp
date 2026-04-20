//
//  Engine.hpp
//  game_engine
//
//  Created by Armand Gago on 1/21/26.
//

#ifndef Engine_hpp
#define Engine_hpp

#include <string>

class SceneDB;
class Renderer;
class ImageDB;
class TextDB;
class AudioDB;
class ComponentDB;

class Engine {
public:
    void game_loop();
    Engine(int windowX, int windowY, SceneDB& sceneDB, Renderer& renderer, ImageDB& imageDB, TextDB& textDB,
           AudioDB& audioDB, ComponentDB& componentDB);

private:
    SceneDB& sceneDB;
    Renderer& rend;
    ImageDB& imageDB;
    TextDB& textDB;
    AudioDB& audioDB;
    ComponentDB& componentDB;

    bool running = true;

    int windowX;
    int windowY;
};

#endif /* Engine_hpp */
