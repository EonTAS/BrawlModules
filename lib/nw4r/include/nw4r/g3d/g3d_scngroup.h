#pragma once

class ScnGroup
{

public:
    char spacer2[0xE4];
    int mainScene;
    // TODO
    virtual u32 IsDerivedFrom(int *unk1);
    virtual void G3dProc(int unk1, int unk2, int unk3);
    virtual ~ScnGroup();
    virtual void *GetTypeObj();
    virtual char *GetTypeName();
    virtual void ForEach();
    virtual void SetScnObjOption();
    virtual int GetScnObjOption();
    virtual void GetValueForSortOpa();
    virtual void GetValueForSortXlu();
    virtual void CalcWorldMtx();
    virtual void Insert(int sceneId, ScnObj *scnObj);
    virtual void Remove();
    virtual void Remove(int thisisdifferent);
};
// Size: 220