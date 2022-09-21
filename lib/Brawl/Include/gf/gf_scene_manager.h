#pragma once

#include <gf/gf_scene.h>

class grCollision; // forward declaration to resolve circular dependancy

class gfSceneManager
{
protected:
    // 0
    char _spacer[0x04];
    // 4

public:
    gfScene *currentScene;
    static gfSceneManager *getInstance();
};