#pragma once

#include <types.h>
#include "mu_file_io_handle.h"
#include <nw4r/g3d/g3d_resfile.h>
#include <mu/mu_object.h>
#include <gf/gf_task.h>
#include <snd/snd_id.h>
#include <StaticAssert.h>

struct scriptEntry
{
    SndID id;
    int length;
};
struct fighter
{
    int charId;
    int displayId;
};

typedef enum
{
    mainScene,
    stageProgess,
    misc1,
    misc2,
    allyPointer,
} muObjectID;
struct muObjectFlags
{
    char *node;
    muObjectID id;
    char flag;
};

const int maxMode = 2;
enum modeType
{
    standard = 0x0,
    teams = 0x1,
    breakTheTargets = 0x2,
    versus = 0x3
};
enum displayType
{
    standardFighter = 0x0,
    doubleFighter = 0x1,
    giantFighter = 0x2,
    metalFighter = 0x3
};

const int totalEnemies = 3;
const int totalAllies = 2;
const int totalTeamPortraits = 11;
STATIC_CHECK(totalEnemies <= totalTeamPortraits);

class muIntroTask : public gfTask
{
protected:
    struct
    {
        nw4r::g3d::ResFile *mainScene;
        nw4r::g3d::ResFile *charCommon;
        nw4r::g3d::ResFile *enemies[totalEnemies];
        nw4r::g3d::ResFile *miniGame;
        nw4r::g3d::ResFile *allies[totalAllies];
    } resFiles;

    struct
    {
        MuObject *mainScene;
        MuObject *stageProgess;
        MuObject *misc1;
        MuObject *misc2;
        MuObject *enemies[totalTeamPortraits];
        MuObject *allyPointer;
        MuObject *allies[totalAllies];
    } muObjects;

    nw4r::g3d::ScnMdl *scnMdl; // G3dObjFv
    // 0xAC
    int progression;
    modeType mode;
    int enemyCount;
    fighter enemies[totalEnemies];
    int allyCount;
    fighter allies[totalAllies];
    // 0xE4
    int commonFilePre;
    struct
    {
        muFileIOHandle mainScene;
        muFileIOHandle charCommon;
        muFileIOHandle enemies[totalEnemies];
        muFileIOHandle miniGame;
        muFileIOHandle allies[totalAllies];
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
    MuObject *createMuObject(int fileIndex, int node, HeapType heap);
    void createMuObjects(muObjectFlags data[], int num, nw4r::g3d::ResFile **resFile);
    void setProgressionMeter(int progression);
    bool addScriptFighterEntry(int fighterID);
    inline void addScriptEntry(SndID ID, int length);
    bool isLoadFinished();
    void init();
    void initVersusData();
    virtual void processDefault();
    virtual ~muIntroTask();
};

static nw4r::g3d::ResFile *loadFile(muFileIOHandle file);
