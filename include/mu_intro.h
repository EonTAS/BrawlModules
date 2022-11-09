#pragma once

#include <types.h>
#include <st/st_melee.h>
#include "mu_file_io_handle.h"
#include <nw4r/g3d/g3d_resfile.h>
#include <mu/mu_object.h>

struct scriptEntry
{
    int id;
    int length;
};

struct muObjectFlags
{
    char *node;
    char flags[4];
};

const int maxMode = 2;
enum modeType
{
    standard = 0x0,
    teams = 0x1,
    breakTheTargets = 0x2,
    vsFightIntro = 0x3
};
enum displayType
{
    standardFighter = 0x0,
    doubleFighter = 0x1,
    giantFighter = 0x2,
    metalFighter = 0x3
};

struct fighter
{
    int charId;
    int displayId;
};
const int totalPossibleEnemies = 4;
const int totalPossibleAllies = 2;

class muIntroTask : public gfTask
{
protected:
    // 0x40
    union
    {
        struct
        {
            nw4r::g3d::ResFile *mainScene;
            nw4r::g3d::ResFile *charCommon;
            nw4r::g3d::ResFile *enemies[3];
            nw4r::g3d::ResFile *miniGame;
            nw4r::g3d::ResFile *allies[2];
        };
        nw4r::g3d::ResFile *asArray[8];
    } resFiles;
    // 0x60
    union
    {
        struct
        {
            MuObject *mainScene;
            MuObject *stageProgess;
            MuObject *misc[2];
            MuObject *enemies[11];
            MuObject *allyPointer;
            MuObject *allies[2];
        };
        MuObject *asArray[18];
    } muObjects;

    ScnMdl *scnMdl; // G3dObjFv
    // 0xAC
    int progression;
    modeType mode;
    int enemyCount;
    fighter enemies[totalPossibleEnemies];
    int allyCount;
    fighter allies[totalPossibleAllies];
    // 0xE4
    int commonFilePre;

    union
    {
        struct
        {
            muFileIOHandle mainScene;
            muFileIOHandle charCommon;
            muFileIOHandle enemies[3];
            muFileIOHandle miniGame;
            muFileIOHandle allies[2];
        };
        muFileIOHandle asArray[8];
    } files;
    //  0x108
    char soundScriptStarted;
    char _soundScriptStarted[0x3];
    // 0x10C
    scriptEntry script[0x10];
    // 0x18C
    int scriptCount;
    int scriptCurrent;
    int voiceLineCurrentTime;

public:
    muIntroTask();
    static muIntroTask *create();

    void dispEnemy();
    void createObjResFile();
    void getEnemyResFileName(char *str1, char *str2, char *str3, int fighterId, int fighterDisplayType);
    void getStageSetting();
    void makeSoundScript();
    void createCharModel();
    void loadCharModel();
    void load3DCharModels();
    void createMuObjects(muObjectFlags data[], int num, nw4r::g3d::ResFile **resFile);
    void setProgressionMeter(int progression);
    inline void addScriptEntry(int ID, int length);
    bool isLoadFinished();
    virtual void processDefault();
    virtual ~muIntroTask();
};
