#pragma once

class muMenu
{
public:
    int globalRumbleSetting;

    static void loadMenuSound();
    static bool isLoadFinishMenuSound();
    static int exchangeSelchkind2SelCharVoice(int id);
    void startRumbleController(int rumbleSetting);
};
