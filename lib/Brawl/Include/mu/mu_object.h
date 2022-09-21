#pragma once
#include <memory.h>
#include <gf/gf_model.h>

class MuObject
{
    char padding[0x10];

public:
    void *test;
    gfModelAnimation *gfModelAnimation;
    static MuObject *create(nw4r::g3d::ResFile *output, char *node, char flag, int otherflag, HeapType type);
    static MuObject *create(nw4r::g3d::ResFile *output, int node, char flag, int otherflag, HeapType type);
    void changeAnimN(char *animName);
    void changeNodeAnimN(char *animName);
    void changeClrAnimN(char *animName);
    void changeVisAnimN(char *animName);
    void setFrameVisible(float frame);
};
