#pragma once

class muMenu
{
public:
    //    int activeRumbleController;
    //    int globalRumbleSetting;

    static void loadMenuSound();
    static bool isLoadFinishMenuSound();
    static int exchangeSelchkind2SelCharVoice(int id);
    static void startRumbleController(int controller, int rumbleStrength, int rumbleSetting);
};
