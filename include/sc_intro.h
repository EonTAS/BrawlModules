#pragma once

#include <types.h>
#include <gf/gf_scene.h>
#include <mu/mu_menu.h>

class scIntro : public gfScene, muMenu
{
protected:
    char _spacer[0x35C];

public:
    int progression;
    modeType mode;
    int enemyCount;
    int allyCount;
    fighter enemies[3];
    fighter allies[2];
};
