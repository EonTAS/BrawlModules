#pragma once

#include <types.h>
#include <gf/gf_scene.h>
#include <mu/mu_menu.h>
#include <mu/mu_menuroot.h>
#include <gf/gf_module.h>

class muMenuController
{
    char unk[0xA8];
};
class scIntro : public gfScene, public muMenu
{
    // 90ff6b60
    //  entire scIntro = 0x3AC
    // 0x0 and 0x4 = vtable stuff 0x8;
protected:
public:
    // 0x8
    muMenuController menuControllers[5];
    // 0x350
    char _spacer[0xC];
    int progression;
    modeType mode;
    int enemyCount;
    fighter enemies[3];
    int allyCount;
    fighter allies[2];
    // offset 0x398
    // 0x394
    int activeController;
    MenuRoot **menuRoot;
    char isCameraReady;
    char rumbleSetting;
    char spacers[2];
    gfFileIOHandle cameraFileHandle; // used for loading the correct aspect ratio
    int something;
    // 0x3A4 = last value
};
