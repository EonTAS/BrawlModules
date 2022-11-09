#pragma once
#include <containers.h>
class ftSlot
{
public:
    int getResId(int id, int fighterID, int fighterColour);
    bool isReady();
    bool isLoaded();

    char spacer[0x15C];
    bool disabled;
    char spacers[3];
    char size[0x4D4 - 0x160];
};
class ftSlotManager
{
public:
    struct SlotsContainer
    {
        ftSlot slots[4];
    };
    SlotsContainer *slotsContainer;
};
