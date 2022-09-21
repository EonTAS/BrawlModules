#pragma once
#include <memory.h>
#include <gf/gf_model.h>

class MuObject
{
    char padding[0x14];

public:
    gfModelAnimation *gfModelAnimation;
    static MuObject *create(nw4r::g3d::ResFile *output, char *node, char flag, int otherflag, HeapType type);
    void changeAnimM(char *animName);
};
