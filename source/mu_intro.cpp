#pragma inline_max_size 512

#include <memory.h>
#include "mu_intro.h"
#include "sc_intro.h"
#include <OS/OSError.h>
#include <gf/gf_scene.h>
#include "printf.h"
#include <mu/mu_menu.h>
#include <gf/gf_model.h>
#include <gf/gf_heap_manager.h>
#include <string.h>
#include <nw4r/g3d/g3d_scnmdl.h>
#include <gf/gf_game_application.h>
#include <snd/snd_system.h>
#include <snd/snd_id.h>
#include <TRK/flushcache.h>
#include <gm/gm_global.h>

const scriptEntry typeVoiceLines[] = {{snd_se_invalid, 0}, {snd_se_invalid, 0}, {snd_se_menu_Narration_Giant, 0x27}, {snd_se_menu_Narration_Metal, 0x31}};
static muObjectFlags mapFileList[] = {
    {"ItrSimpleMap0000_TopN", {0x0, 0x0, 0x21, 0x0}},
};
static muObjectFlags panelList[] = {
    {"ItrSimplePanel0000_TopN", {0x1, 0x1, 0x10, 0x0}},
    {"ItrSimplePanel0001_TopN", {0x2, 0x2, 0x11, 0x0}},
    {"ItrSimplePanel0002_TopN", {0x3, 0x3, 0x1F, 0x0}},
    {"ItrSimplePanel0010_TopN", {0xF, 0xF, 0x20, 0x0}},
};
muIntroTask *muIntroTask::create()
{
   muIntroTask *intro = new (Heaps::MenuInstance) muIntroTask();
   intro->getStageSetting();
   intro->files[0].readRequest("/menu/intro/enter/cmn.brres", Heaps::MenuResource, 0, 0);
   if (intro->mode != breakTheTargets)
   {
      intro->createCharModel();
   }
   else
   {
      char filename[32];
      sprintf(filename, "/menu/intro/enter/mini%d.brres", 1);
      intro->files[5].readRequest(filename, Heaps::MenuResource, 0, 0);
   }
   muMenu::loadMenuSound();
   intro->commonFilePre = 0;
   intro->soundScriptStarted = 0;
   intro->makeSoundScript();

   { // dumb fuckery to make the game use the right function coz i cant edit the reloc's of other rels
      bool (muIntroTask::*checkLoad)() = &muIntroTask::isLoadFinished;
      unsigned int instruction = 0x48000001; // base `bl` instruction
      int volatile *const targetAddress = (int *)0x806C0738;
      int volatile functionAddress = 0;
      __asm__("lwz %[functionAddress],0x8(%[checkLoad])"
              : [functionAddress] "=r"(functionAddress)
              : [checkLoad] "r"(&checkLoad));
      instruction += functionAddress - (int)targetAddress; // add offset to instruction
      *targetAddress = instruction;
      TRK_flush_cache((void *)(targetAddress - 4), (int)(targetAddress + 4));
   }

   return intro;
}

extern gfGameApplication *g_gfGameApplication;
void muIntroTask::processDefault()
{
   if (this->soundScriptStarted == 0)
   {
      if (isLoadFinished())
      {
         this->resFiles[0] = loadFile(this->files[0]);
         this->createMuObjects(mapFileList, 1, &this->resFiles[0]);

         if (this->mode != breakTheTargets)
         {
            this->loadCharModel();
         }
         else
         {
            this->resFiles[7] = loadFile(this->files[5]);
            this->muObjects[4] = MuObject::create(&this->resFiles[7], 18, (char)0, 0, Heaps::MenuInstance);
         }
         g_gfGameApplication->keepFB.endKeepScreen();
         char str[32];
         sprintf(str, "ItrSimpleMap0000_TopN__%d", this->progression + 1);
         MuObject *currentMu = this->muObjects[0];
         currentMu->changeNodeAnimNIf(str);
         currentMu->changeVisAnimNIf(str);
         currentMu->changeTexPatAnimNIf(str);
         currentMu->changeClrAnimNIf(str);
         currentMu->gfModelAnimation->setUpdateRate(1.0);
         scIntro *_intro = (scIntro *)gfSceneManager::getInstance()->m_currentScene;
         (*_intro->menuRoot)->scene->Insert((*_intro->menuRoot)->scene->sceneItemsCount, currentMu->scnObj);
         if (this->mode != breakTheTargets)
         {
            setProgressionMeter(this->progression);
         }
         else
         {
            this->muObjects[4]->gfModelAnimation->m_anmObjChrRes->SetUpdateRate(1.0);
            this->muObjects[4]->gfModelAnimation->m_anmObjMatClrRes->SetUpdateRate(1.0);
            scIntro *_intro = (scIntro *)gfSceneManager::getInstance()->m_currentScene;
            ScnGroup *scene = (*_intro->menuRoot)->scene;
            scene->Insert(scene->sceneItemsCount, this->muObjects[4]->scnObj);
         }
         int rumble = _intro->rumbleSetting;
         if (rumble == 0x78)
         {
            rumble = -1;
         }
         muMenu::startRumbleController(_intro->activeController, 0x14, rumble);
         g_sndSystem->playSE(snd_se_system_044, -1, 0, 0, -1);
         this->commonFilePre = 0;
         this->soundScriptStarted = 1;
      }
   }
   else
   {
      scriptEntry *currentVoiceLine = &this->script[scriptCurrent];
      if (this->voiceLineCurrentTime == 0)
      {
         g_sndSystem->playSE((SndID)currentVoiceLine->id, -1, 0, 0, -1);
      }
      this->voiceLineCurrentTime++;
      if (this->voiceLineCurrentTime >= currentVoiceLine->length)
      {
         this->voiceLineCurrentTime = 0;
         this->scriptCurrent++;
      }
      if (this->scriptCurrent >= this->scriptCount)
      {
         gfSceneManager *scnManager = gfSceneManager::getInstance();
         scnManager->unk1 = 0;
         scnManager->processStep = 2; // tell manager to move to next scene
      }
   }
}
void muIntroTask::createMuObjects(muObjectFlags data[], int num, nw4r::g3d::ResFile **resFile)
{
   for (int i = 0; i < num; i++)
   {
      int maxNodes = 1;

      muObjectFlags currData = data[i];
      if (currData.flags[0] < currData.flags[1])
      {
         maxNodes = currData.flags[1] - currData.flags[0];
      }
      for (int j = 0; j < maxNodes; j++)
      {
         MuObject *result = MuObject::create(resFile, currData.node, currData.flags[2], 0, Heaps::MenuInstance);
         this->muObjects[currData.flags[0] + j] = result;

         char str1[32];
         strcpy(str1, currData.node);
         strcat(str1, "__0");
         result->changeAnimN(str1);
         if (result->gfModelAnimation != 0)
         {
            result->gfModelAnimation->setUpdateRate(0.0);
         }
      }
   }
}

void muIntroTask::getStageSetting()
{

   gfSceneManager *manager = gfSceneManager::getInstance();
   if (manager->m_currentScene->m_sceneName == "sqSingleSimple")
   {
      scIntro *_intro = (scIntro *)manager->m_currentScene;

      this->progression = _intro->progression;
      this->mode = _intro->mode;
      this->enemyCount = _intro->enemyCount;
      this->allyCount = _intro->allyCount;
      for (int i = 0; i < totalEnemies; i++)
      {
         if (i < 3)
         {
            this->enemies[i] = _intro->enemies[i];
         }
      }
      for (int i = 0; i < totalAllies; i++)
      {
         if (i < 2)
         {
            this->allies[i] = _intro->allies[i];
         }
      }
   }
   else
   {
      this->initVersusData();
   }
}

// extern gmGameGlobal *g_GameGlobal;
void muIntroTask::makeSoundScript()
{
   if (this->mode == breakTheTargets)
   {
      this->addScriptEntry(snd_se_menu_Narration_BreakTarget, 0x5A); // "Break the Targets!"
      return;
   }

   int count;
   if (this->mode == teams)
   {
      count = 1;
      this->addScriptEntry(snd_se_menu_Narration_Team, 0x1E); // "TEAM"
   }
   else
   {
      if (this->mode == standard)
      {
         this->addScriptEntry(snd_se_menu_Narration_Versus, 0x3C); // "VERSUS"
      }
      count = this->enemyCount;
   }
   for (int i = 0; i < count; i++)
   {
      if (i > 0) // if the very first voice line, dont have a seperator
      {
         if (i < count - 1) // if not last in list,
         {
            this->addScriptEntry(snd_se_invalid, 10); // "{comma}"
         }
         else
         {
            this->addScriptEntry(snd_se_menu_Narration_And, 0x2B); // "AND"
         }
      }
      int prefixIndex;
      if (this->mode == teams)
      {
         prefixIndex = standardFighter;
      }
      else if (this->mode == standard)
      {
         prefixIndex = this->enemies[i].displayId;
      }

      scriptEntry prefix = typeVoiceLines[prefixIndex];
      if (prefix.id != -1)
      {
         this->addScriptEntry(prefix.id, prefix.length);
      }
      this->addScriptFighterEntry(this->enemies[i].charId);
   }
   if (this->mode == versus)
   {
      count = this->allyCount;
      for (int i = 0; i < count; i++)
      {
         if (i > 0) // if the very first voice line, dont have a seperator
         {
            if (i < count - 1) // if not last in list,
            {
               this->addScriptEntry(snd_se_invalid, 10); // "{comma}"
            }
            else
            {
               this->addScriptEntry(snd_se_menu_Narration_And, 0x2B); // "AND"
            }
         }
         else
         {
            this->addScriptEntry(snd_se_menu_Narration_Versus, 0x3C); // "VERSUS"
         }
         scriptEntry prefix = typeVoiceLines[standardFighter];
         if (prefix.id != -1)
         {
            this->addScriptEntry(prefix.id, prefix.length);
         }
         this->addScriptFighterEntry(this->allies[i].charId);
      }
   }
}
void muIntroTask::loadCharModel()
{
   this->resFiles[1] = loadFile(this->files[1]);
   this->createMuObjects(panelList, 4, &this->resFiles[1]);
   this->scnMdl = nw4r::g3d::ScnMdl::Construct(gfHeapManager::getMEMAllocator(Heaps::MenuInstance), 0, 0xD, this->muObjects[2]->scnObj);

   if (this->mode == teams)
   {

      char str1[32];
      char str2[32];
      char str3[32];
      this->getEnemyResFileName(str1, str2, str3, this->enemies[0].charId, standardFighter);
      this->resFiles[2] = loadFile(this->files[2]);
      for (int i = 0; i < 11; i++)
      {
         MuObject *newMu = MuObject::create(&this->resFiles[2], 0x1C - i, 0, 0, Heaps::MenuInstance);
         newMu->changeNodeAnimN(str2);
         newMu->gfModelAnimation->m_anmObjChrRes->SetUpdateRate(1.0);
         newMu->changeClrAnimN(str2);
         float style;
         if (i == 0)
         {
            style = 1.0;
         }
         else
         {
            style = 0.0;
         }
         newMu->gfModelAnimation->m_anmObjMatClrRes->SetUpdateRate(style);
         newMu->changeVisAnimN(str3);
         newMu->setFrameVisible(3.0);
         newMu->gfModelAnimation->m_anmObjVisRes->SetUpdateRate(0.0);
         this->muObjects[i + 4] = newMu;
      }
   }
   else if (this->mode == standard || this->mode == versus)
   {
      for (int i = 0; i < this->enemyCount; i++)
      {
         this->resFiles[2 + i] = loadFile(this->files[2 + i]);

         char str1[32];
         char str2[32];
         char str3[32];
         this->getEnemyResFileName(str1, str2, str3, this->enemies[i].charId, this->enemies[i].displayId);
         MuObject *newMu = MuObject::create(&this->resFiles[2 + i], 0x1C - i, 0, 0, Heaps::MenuInstance);

         newMu->changeNodeAnimN(str2);
         newMu->gfModelAnimation->m_anmObjChrRes->SetUpdateRate(1.0);
         newMu->changeClrAnimN(str2);
         newMu->gfModelAnimation->m_anmObjMatClrRes->SetUpdateRate(1.0);
         newMu->changeVisAnimN(str3);
         int enemyCount = this->enemyCount;
         float targetFrame;
         if (enemyCount == 2)
         {
            if (i == 0)
            {
               targetFrame = 1.0;
            }
            else
            {
               targetFrame = 2.0;
            }
         }
         else if (enemyCount == 1)
         {
            targetFrame = 0.0;
         }
         else // 3 enemyes or invalid
         {
            targetFrame = 4.0;
         }
         newMu->setFrameVisible(targetFrame);
         newMu->gfModelAnimation->m_anmObjVisRes->SetUpdateRate(0.0);
         this->muObjects[i + 4] = newMu;
      }
   }
   else if (this->mode == versus)
   {
      // for (int i = 0; i < 4; i++)
      // {
      //    fighter fighter = this->enemies[i];
      //    s16 resID = ftCommonDataAccesser::getModelResId(fighter.slotNo, fighter.id, fighter.colour);
      //    if (resID != -1)
      //    {
      //       this->resFiles[6 + i] = soArchiveDb::getManager()->getResFileFromId(resID, 2, 0, 0, -1); // gets main model file
      //       // can get other parts (e.g. animation part) same way
      //       MuObject *newMu = MuObject::create(&this->resFiles[6 + i], 0x1E - i, 0, 0, Heaps::MenuInstance);

      //       // newMu->changeNodeAnimN(str2);
      //       newMu->gfModelAnimation->m_anmObjChrRes->SetUpdateRate(1.0);
      //       // newMu->changeClrAnimN(str2);
      //       newMu->gfModelAnimation->m_anmObjMatClrRes->SetUpdateRate(1.0);
      //       // newMu->changeVisAnimN(str3);
      //       newMu->setFrameVisible(5.0);
      //       newMu->gfModelAnimation->m_anmObjVisRes->SetUpdateRate(0.0);

      //       this->muObjects[i + 16] = newMu;
      //    }
      // }
   }
   // allies loop
   if (this->mode != breakTheTargets)
   {
      for (int i = 0; i < this->allyCount; i++)
      {
         this->resFiles[6 + i] = loadFile(this->files[6 + i]);

         char str1[32];
         char str2[32];
         char str3[32];
         this->getEnemyResFileName(str1, str2, str3, this->allies[i].charId, this->allies[i].displayId);
         MuObject *newMu = MuObject::create(&this->resFiles[6 + i], 0x1E - i, 0, 0, Heaps::MenuInstance);

         newMu->changeNodeAnimN(str2);
         newMu->gfModelAnimation->m_anmObjChrRes->SetUpdateRate(1.0);
         newMu->changeClrAnimN(str2);
         newMu->gfModelAnimation->m_anmObjMatClrRes->SetUpdateRate(1.0);
         newMu->changeVisAnimN(str3);
         newMu->setFrameVisible(5.0);
         newMu->gfModelAnimation->m_anmObjVisRes->SetUpdateRate(0.0);

         this->muObjects[i + 16] = newMu;
      }
   }
   // continue at line 283
   int startingNode;
   int totalEnemies;
   if (this->mode == teams)
   {
      startingNode = 10;
      totalEnemies = 11;
   }
   else if (this->mode == breakTheTargets)
   {
      startingNode = 0;
      totalEnemies = 0;
   }
   else
   {
      totalEnemies = this->enemyCount;
      if (totalEnemies == 2)
      {
         startingNode = 1;
      }
      else if (totalEnemies == 3)
      {
         startingNode = 3;
      }
      else
      {
         startingNode = 0;
      }
   }
   for (int i = 0; i < totalEnemies; i++)
   {
      char targetString[32];
      sprintf(targetString, "pos%02d", startingNode + i);
      nw4r::g3d::ScnMdl::Pushback(this->scnMdl, this->muObjects[4 + i]->scnObj, targetString);
   }
   for (int i = 0; i < this->allyCount; i++)
   {
      char targetString[32];
      sprintf(targetString, "pos%02d", 0x1E + i);
      nw4r::g3d::ScnMdl::Pushback(this->scnMdl, this->muObjects[16 + i]->scnObj, targetString);
   }
}

void muIntroTask::createCharModel()
{
   if (this->mode == breakTheTargets)
   {
      return;
   }
   else if (this->mode == versus)
   {
      this->files[1].readRequest("/menu/intro/enter/chrcmn.brres", Heaps::MenuResource, 0, 0);
      return;
   }
   else
   {
      this->files[1].readRequest("/menu/intro/enter/chrcmn.brres", Heaps::MenuResource, 0, 0);
      if (this->mode == teams)
      {
         char str1[32];
         char str2[32];
         char str3[32];
         fighter enemy = this->enemies[0];
         this->getEnemyResFileName(str1, str2, str3, enemy.charId, standardFighter);
         this->files[2].readRequest(str1, Heaps::MenuInstance, 0, 0);
      }
      else
      {
         for (int i = 0; i < this->enemyCount; i++)
         {
            char str1[32];
            char str2[32];
            char str3[32];
            this->getEnemyResFileName(str1, str2, str3, this->enemies[i].charId, this->enemies[i].displayId);
            this->files[2 + i].readRequest(str1, Heaps::MenuInstance, 0, 0);
         }
      }
      for (int i = 0; i < this->allyCount; i++)
      {
         char str1[32];
         char str2[32];
         char str3[32];
         this->getEnemyResFileName(str1, str2, str3, this->allies[i].charId, this->allies[i].displayId);
         this->files[6 + i].readRequest(str1, Heaps::MenuInstance, 0, 0);
      }
   }
}

void muIntroTask::getEnemyResFileName(char *str1, char *str2, char *str3, int fighterId, int fighterDisplayType)
{
   int displayType = 0;
   if (fighterDisplayType == giantFighter)
   {
      displayType = 1;
   }
   else if (fighterDisplayType == metalFighter)
   {
      displayType = 2;
   }
   int fighterFileId = muMenu::exchangeGmCharacterKind2Something(fighterId) + 1;
   sprintf(str1, "/menu/intro/enter/%s%04d.brres", "chr", fighterFileId);
   sprintf(str2, "ItrSimple%s%04d_TopN__%d", "Chr", fighterFileId, displayType);
   sprintf(str3, "ItrSimple%s%04d_TopN__0", "Chr", fighterFileId);
}

void muIntroTask::setProgressionMeter(int progression)
{
   this->muObjects[1]->setFrameMatCol((float)progression);

   scIntro *_intro = (scIntro *)gfSceneManager::getInstance()->m_currentScene;
   ScnGroup *scene = (*_intro->menuRoot)->scene;
   scene->Insert(scene->sceneItemsCount, this->muObjects[1]->scnObj);
   scene->Insert(scene->sceneItemsCount, this->scnMdl);

   this->muObjects[2]->gfModelAnimation->m_anmObjChrRes->SetUpdateRate(1.0);
   this->muObjects[2]->gfModelAnimation->m_anmObjMatClrRes->SetUpdateRate(1.0);

   if (this->allyCount != 0)
   {
      scene->Insert(scene->sceneItemsCount, this->muObjects[15]->scnObj);
      this->muObjects[15]->gfModelAnimation->m_anmObjChrRes->SetUpdateRate(1.0);
   }

   this->muObjects[3]->gfModelAnimation->m_anmObjChrRes->SetUpdateRate(1.0);
   this->muObjects[3]->gfModelAnimation->m_anmObjMatClrRes->SetUpdateRate(1.0);
   scene->Insert(scene->sceneItemsCount, this->muObjects[3]->scnObj);
}

bool muIntroTask::addScriptFighterEntry(int fighterID)
{
   SndID charSfxId = (SndID)muMenu::exchangeSelchkind2SelCharVoice(fighterID);
   if (charSfxId == -1)
   {
      return false;
   }
   int charLineLength;
   if (g_GameGlobal->getLanguage() == 0)
   {
      charLineLength = muMenu::exchangeSelCharVoice2SelCharVoiceLengthE(fighterID);
   }
   else
   {
      charLineLength = muMenu::exchangeSelCharVoice2SelCharVoiceLengthJ(fighterID);
   }
   this->addScriptEntry(charSfxId, charLineLength);
   return true;
}

inline void muIntroTask::addScriptEntry(SndID id, int length)
{
   scriptEntry *script = &this->script[this->scriptCount++];
   script->id = id;
   script->length = length;
}

muIntroTask::muIntroTask() : gfTask("Intro", 8, 0xf, 8, 1)
{
   for (int i = 0; i < 0x12; i++)
   {
      this->muObjects[i] = 0;
   }
   this->scnMdl = 0;
   for (int i = 0; i < 8; i++)
   {
      this->files[i] = muFileIOHandle();
   }
   for (int i = 0; i < 8; i++)
   {
      this->resFiles[i] = 0;
   }
   this->scriptCurrent = 0;
   this->scriptCount = 0;
   this->commonFilePre = 0;
   this->voiceLineCurrentTime = 0;
}

// 0x811A8CE0
muIntroTask::~muIntroTask()
{
   delete this->scnMdl;
   for (int i = 0; i < 0x12; i++)
   {
      delete this->muObjects[i];
   }
   for (int i = 0; i < 8; i++)
   {
      delete this->resFiles[i];
   }
   for (int i = 0; i < 8; i++)
   {
      this->files[i].cancelRequest();
   }
}
bool muIntroTask::isLoadFinished()
{
   for (int i = 0; i < 8; i++)
   {
      if (!this->files[i].isReady()) // <-this is the current place of problems, this is checking with the pointer containing it, not the actual file
      {
         return false;
      }
   }
   return muMenu::isLoadFinishMenuSound();
}

void muIntroTask::initVersusData()
{

   this->progression = 0;
   this->enemyCount = 0;
   this->allyCount = 0;
   this->mode = versus;

   gmGlobalModeMelee *mode = g_GameGlobal->m_modeMelee;
   bool isTeams = mode->m_meleeInitData.m_isTeams;

   int team1ID = -1;
   for (int i = 0; i < 4; i++)
   {
      gmPlayerInitData *player = &mode->m_playersInitData[i];
      if (player->m_initState < 3)
      {
         int playerFighter = muMenu::exchangeGmCharacterKind2MuStockchkind(player->m_slotID);
         int playerCostume = 0;
         if (isTeams)
         {
            if (team1ID == -1)
            {
               team1ID = player->m_teamID;
            }
            if (player->m_teamID == team1ID)
            {
               this->allies[this->allyCount].charId = playerFighter;
               this->allies[this->allyCount].displayId = playerCostume;
               this->allyCount++;
            }
            else
            {
               this->enemies[this->enemyCount].charId = playerFighter;
               this->enemies[this->enemyCount].displayId = playerCostume;
               this->enemyCount++;
            }
         }
         else
         {
            this->enemies[this->enemyCount].charId = playerFighter;
            this->enemies[this->enemyCount].displayId = playerCostume;
            this->enemyCount++;
         }
      }
   }
}

static nw4r::g3d::ResFile *loadFile(muFileIOHandle file)
{
   void *buffer = 0;
   if (file.getReturnStatus() != 0x15)
   {
      buffer = file.getBuffer();
      file.release();
   }
   if (buffer != 0)
   {
      nw4r::g3d::ResFile::Init(&buffer);
   }
   return (nw4r::g3d::ResFile *)buffer;
}