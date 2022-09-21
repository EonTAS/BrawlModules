#pragma once

#include <nw4r/g3d/g3d_scnmdlsmpl.h>

class ScnMdl : public ScnMdlSimple
{
public:
    // TODO
    virtual u32 IsDerivedFrom(int *unk1);
    virtual void G3dProc(int unk1, int unk2, int unk3);
    virtual ~ScnMdl();

    char _spacer[80];

    static ScnMdl Construct(void *memAllocator, int x, int y, void *thingInMuObject);
    static void Pushback(void *thing, void *thing2, char *str); // PushBack/[nw4r3g3d12ScnMdlExpandFPQ34nw4r3g3]/(g3d_scnmdlexpand.o)
};

// Size: 392
