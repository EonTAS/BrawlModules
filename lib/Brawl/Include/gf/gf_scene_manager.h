#pragma once

#include <gf/gf_scene.h>

class grCollision; // forward declaration to resolve circular dependancy

class gfSceneManager
{

public:
    // 0
    char spacer[0x4];
    gfScene *currentScene;
    char _spacer[0x27C];
    // 4
    int unk1;
    int processStep;
    static gfSceneManager *getInstance();
};