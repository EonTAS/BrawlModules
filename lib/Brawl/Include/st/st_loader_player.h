#pragma once
struct fighterData // size 0x5C
{
    char test_0x1;
    char test2_0x2;
    char test3_0x3;
    char test4_0x4;
    char test5_0x5;
    char test_0xb;
    char test_0xc;
    char test_0xe;
    char test_0xf;
    char test_0x10;
    short test18_0x12;
    char test_0x14;
    int test23_0x18;
    int test36_0x28;
    int test37_0x2c;
    int test38_0x30;
    int test39_0x34;
    int test40_0x38;
    int test41_0x3c;
    float test42_0x40;
    float test43_0x44;
    float test44_0x48;
    char test_0x50;
    char test59_0x5a;
};

class stLoaderPlayer
{
public:
    char spacer[0x40];
    // 0x40
    fighterData *fighters; //->0xA3 = team
    // 0x44
    int unk;
    // 0x48
    int fighterIndex;
    // 0x4C

    // 0x50
    int ftSlotID;

    // 0x5a
    char fighterID;
    // 0x5b
    char fighterColour;
};