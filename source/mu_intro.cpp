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
#include <gf/gf_pad.h>
#include <gf/gf_homemenu.h>

const scriptEntry typeVoiceLines[] = {{snd_se_invalid, 0}, {snd_se_invalid, 0}, {snd_se_menu_Narration_Giant, 0x27}, {snd_se_menu_Narration_Metal, 0x31}};

static muObjectFlags mapFileList[] = {
    {"ItrSimpleMap0000_TopN", mainScene, 0x21},
};
static muObjectFlags panelList[] = {
    {"ItrSimplePanel0000_TopN", stageProgess, 0x10},
    {"ItrSimplePanel0001_TopN", misc1, 0x11},
    {"ItrSimplePanel0002_TopN", misc2, 0x1F},
    {"ItrSimplePanel0010_TopN", allyPointer, 0x20},
};
#define PRELOAD true

#if PRELOAD
#define instanceHeap Heaps::MenuInstance
#define resourceHeap Heaps::Network
#else
#define instanceHeap Heaps::MenuInstance
#define resourceHeap Heaps::MenuResource
#endif
muIntroTask *muIntroTask::create()
{
   muIntroTask *intro = new (instanceHeap) muIntroTask();

   g_gfHomeMenu->setBan(1, 1);
   g_gfHomeMenu->releaseResource();

   // load settings that are set externally
   intro->getStageSetting();
   // load scene
   intro->files.mainScene.readRequest("/menu/intro/enter/cmn.brres", resourceHeap, 0, 0);
   if (intro->mode != breakTheTargets)
   {
      // request load of all portrait files
      intro->createCharModel();
   }
   else
   {
      // request load of BTT file
      char filename[32];
      sprintf(filename, "/menu/intro/enter/mini%d.brres", 1);
      intro->files.miniGame.readRequest(filename, resourceHeap, 0, 0);
   }
   muMenu::loadMenuSound();
   intro->commonFilePre = 0;
   intro->soundScriptStarted = false;

   // interpret situation to create a script for the announcer
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
   if (!this->soundScriptStarted)
   {
      if (isLoadFinished())
      {
         // all files are loaded into memory, construct the visuals
         this->init();
      }
   }
   else
   {
      this->playSoundScript();

      for (int i = 0; i < 8; i++)
      {
         gfPadStatus pad;
         g_gfPadSystem->getSysPadStatus(i, &pad);
         if (pad.buttonMask & START)
         {
            endScene();
         }
      }
   }
}
void muIntroTask::playSoundScript()
{
   scriptEntry *currentVoiceLine = &this->script[this->scriptCurrent];
   // if on first frame of line, say the line
   if (this->voiceLineCurrentTime == 0)
   {
      g_sndSystem->playSE(currentVoiceLine->id, -1, 0, 0, -1);
   }
   this->voiceLineCurrentTime++;
   // if line finished, go to next line
   if (this->voiceLineCurrentTime >= currentVoiceLine->length)
   {
      this->voiceLineCurrentTime = 0;
      this->scriptCurrent++;
   }
   // if script finished, tell scene manager
   if (this->scriptCurrent >= this->scriptCount - 1)
   {
      g_sndSystem->playSE(this->script[this->scriptCurrent].id, -1, 0, 0, -1);
      endScene();
   }
}

void endScene()
{
   gfSceneManager *scnManager = gfSceneManager::getInstance();
   scnManager->unk1 = 0;
   scnManager->processStep = 2; // tell manager to move to next scene
   g_gfHomeMenu->setBan(1, 1);
}
void muIntroTask::init()
{
   this->resFiles.mainScene = loadFile(&this->files.mainScene);
   this->createMuObjects(mapFileList, 1, &this->resFiles.mainScene);

   if (this->mode != breakTheTargets)
   {
      this->loadCharModel();
   }
   else
   {
      this->resFiles.allies[1] = loadFile(&this->files.miniGame);
      this->muObjects.enemies[0] = MuObject::create(&this->resFiles.allies[1], 18, (char)0, 0, instanceHeap);
   }
   g_gfGameApplication->keepFB.endKeepScreen();
   char str[32];
   sprintf(str, "ItrSimpleMap0000_TopN__%d", this->progression + 1);
   MuObject *currentMu = this->muObjects.mainScene;
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
      this->muObjects.enemies[0]->gfModelAnimation->m_anmObjChrRes->SetUpdateRate(1.0);
      this->muObjects.enemies[0]->gfModelAnimation->m_anmObjMatClrRes->SetUpdateRate(1.0);
      scIntro *_intro = (scIntro *)gfSceneManager::getInstance()->m_currentScene;
      ScnGroup *scene = (*_intro->menuRoot)->scene;
      scene->Insert(scene->sceneItemsCount, this->muObjects.enemies[0]->scnObj);
   }
   int rumble = _intro->rumbleSetting;
   if (rumble == 0x78)
   {
      rumble = -1;
   }
   muMenu::startRumbleController(_intro->activeController, 0x14, rumble);
   g_sndSystem->playSE(snd_se_system_044, -1, 0, 0, -1);
   this->commonFilePre = 0;
   this->soundScriptStarted = true;
}

void muIntroTask::createMuObjects(muObjectFlags data[], int num, nw4r::g3d::ResFile **resFile)
{
   for (int i = 0; i < num; i++)
   {
      muObjectFlags currData = data[i];
      MuObject *result = MuObject::create(resFile, currData.node, currData.flag, 0, instanceHeap);
      switch (currData.id)
      {
      case mainScene:
         this->muObjects.mainScene = result;
         break;
      case stageProgess:
         this->muObjects.stageProgess = result;
         break;
      case misc1:
         this->muObjects.misc1 = result;
         break;
      case misc2:
         this->muObjects.misc2 = result;
         break;
      case allyPointer:
         this->muObjects.allyPointer = result;
         break;
      }

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

void muIntroTask::getStageSetting()
{

   gfSceneManager *manager = gfSceneManager::getInstance();

   if (strcmp(manager->m_currentSequence->m_sequenceName, "sqSingleSimple") == 0)
   {
      // classic mode load data that is set up natively into memory
      scIntro *_intro = (scIntro *)manager->m_currentScene;

      this->progression = _intro->progression;
      this->mode = _intro->mode;
      this->enemyCount = _intro->enemyCount;
      this->allyCount = _intro->allyCount;
      for (int i = 0; i < totalEnemies; i++)
      {
         if (i < 3) // vanilla size
         {
            this->enemies[i] = _intro->enemies[i];
         }
      }
      for (int i = 0; i < totalAllies; i++)
      {
         if (i < 2) // vanilla size
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
      // if we're in BTT, just say that instead of any fighters
      this->addScriptEntry(snd_se_menu_Narration_BreakTarget, 0x5A); // "Break the Targets!"
      return;
   }
   // count is how many enemies we will mention
   int count;
   if (this->mode == teams)
   {
      // when its a team, we just mention the first one (should only ever be one natively)
      count = 1;
      this->addScriptEntry(snd_se_menu_Narration_Team, 0x1E); // "TEAM"
   }
   else
   {
      if (this->mode == standard)
      {
         // only say versus in normal classic mode state, skipped otherwise.
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
      int prefixIndex = standardFighter;
      if (this->mode == standard)
      {
         prefixIndex = this->enemies[i].displayId;
      }

      scriptEntry prefix = typeVoiceLines[prefixIndex];
      if (prefix.id != snd_se_invalid) // if not saying a prefix, dont add it
      {
         this->addScriptEntry(prefix.id, prefix.length);
      }
      // say enemy name
      this->addScriptFighterEntry(this->enemies[i].charId);
   }
   if (this->mode == versus) // in versus, we want to list the "allies" list too, aka enemy team
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
         else // if on first voice, say "versus" since this is an enemy team to the first list of fighters said
         {
            this->addScriptEntry(snd_se_menu_Narration_Versus, 0x3C); // "VERSUS"
         }
         scriptEntry prefix = typeVoiceLines[standardFighter];
         if (prefix.id != -1) // if not saying a prefix, dont add it
         {
            this->addScriptEntry(prefix.id, prefix.length);
         }
         this->addScriptFighterEntry(this->allies[i].charId);
      }
   }
}
void muIntroTask::loadCharModel()
{
   this->resFiles.charCommon = loadFile(&this->files.charCommon);
   this->createMuObjects(panelList, 4, &this->resFiles.charCommon);
   this->scnMdl = nw4r::g3d::ScnMdl::Construct(gfHeapManager::getMEMAllocator(instanceHeap), 0, 0xD, this->muObjects.misc1->scnObj);

   if (this->mode == teams)
   {
      char str1[32];
      char str2[32];
      char str3[32];
      this->getEnemyResFileName(str1, str2, str3, this->enemies[0].charId, standardFighter);
      this->resFiles.enemies[0] = loadFile(&this->files.enemies[0]);
      for (int i = 0; i < totalTeamPortraits; i++)
      {
         MuObject *newMu = MuObject::create(&this->resFiles.enemies[0], 0x1C - i, 0, 0, instanceHeap);
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
         this->muObjects.enemies[i] = newMu;
      }
   }
   else if (this->mode == standard || this->mode == versus)
   {
      for (int i = 0; i < this->enemyCount; i++)
      {
         this->resFiles.enemies[i] = loadFile(&this->files.enemies[i]);

         char str1[32];
         char str2[32];
         char str3[32];
         this->getEnemyResFileName(str1, str2, str3, this->enemies[i].charId, this->enemies[i].displayId);
         MuObject *newMu = MuObject::create(&this->resFiles.enemies[i], 0x1C - i, 0, 0, instanceHeap);

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
         this->muObjects.enemies[i] = newMu;
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
      //       MuObject *newMu = MuObject::create(&this->resFiles[6 + i], 0x1E - i, 0, 0, instanceHeap);

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
         this->resFiles.allies[i] = loadFile(&this->files.allies[i]);

         char str1[32];
         char str2[32];
         char str3[32];
         this->getEnemyResFileName(str1, str2, str3, this->allies[i].charId, this->allies[i].displayId);
         MuObject *newMu = MuObject::create(&this->resFiles.allies[i], 0x1E - i, 0, 0, instanceHeap);

         newMu->changeNodeAnimN(str2);
         newMu->gfModelAnimation->m_anmObjChrRes->SetUpdateRate(1.0);
         newMu->changeClrAnimN(str2);
         newMu->gfModelAnimation->m_anmObjMatClrRes->SetUpdateRate(1.0);
         newMu->changeVisAnimN(str3);
         newMu->setFrameVisible(5.0);
         newMu->gfModelAnimation->m_anmObjVisRes->SetUpdateRate(0.0);

         this->muObjects.allies[i] = newMu;
      }
   }
   // continue at line 283
   int startingNode;
   int totalEnemies;
   if (this->mode == teams)
   {
      startingNode = 10;
      totalEnemies = totalTeamPortraits;
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
      nw4r::g3d::ScnMdl::Pushback(this->scnMdl, this->muObjects.enemies[i]->scnObj, targetString);
   }
   for (int i = 0; i < this->allyCount; i++)
   {
      char targetString[32];
      sprintf(targetString, "pos%02d", 0x1E + i);
      nw4r::g3d::ScnMdl::Pushback(this->scnMdl, this->muObjects.allies[i]->scnObj, targetString);
   }
}

void muIntroTask::createCharModel()
{
   if (this->mode == breakTheTargets)
   {
      return;
   }
   // else if (this->mode == versus)
   // {
   //    this->files.charCommon.readRequest("/menu/intro/enter/chrcmn.brres", resourceHeap, 0, 0);
   //    return;
   // }
   else
   {
      this->files.charCommon.readRequest("/menu/intro/enter/chrcmn.brres", resourceHeap, 0, 0);
      if (this->mode == teams)
      {
         char str1[32];
         char str2[32];
         char str3[32];
         fighter enemy = this->enemies[0];
         this->getEnemyResFileName(str1, str2, str3, enemy.charId, standardFighter);
         this->files.enemies[0].readRequest(str1, instanceHeap, 0, 0);
      }
      else
      {
         for (int i = 0; i < this->enemyCount; i++)
         {
            char str1[32];
            char str2[32];
            char str3[32];
            this->getEnemyResFileName(str1, str2, str3, this->enemies[i].charId, this->enemies[i].displayId);
            this->files.enemies[i].readRequest(str1, instanceHeap, 0, 0);
         }
      }
      for (int i = 0; i < this->allyCount; i++)
      {
         char str1[32];
         char str2[32];
         char str3[32];
         this->getEnemyResFileName(str1, str2, str3, this->allies[i].charId, this->allies[i].displayId);
         this->files.allies[i].readRequest(str1, instanceHeap, 0, 0);
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
   this->muObjects.stageProgess->setFrameMatCol((float)progression);

   scIntro *_intro = (scIntro *)gfSceneManager::getInstance()->m_currentScene;
   ScnGroup *scene = (*_intro->menuRoot)->scene;
   scene->Insert(scene->sceneItemsCount, this->muObjects.stageProgess->scnObj);
   scene->Insert(scene->sceneItemsCount, this->scnMdl);

   this->muObjects.misc1->gfModelAnimation->m_anmObjChrRes->SetUpdateRate(1.0);
   this->muObjects.misc1->gfModelAnimation->m_anmObjMatClrRes->SetUpdateRate(1.0);

   if (this->allyCount != 0)
   {
      scene->Insert(scene->sceneItemsCount, this->muObjects.allyPointer->scnObj);
      this->muObjects.allyPointer->gfModelAnimation->m_anmObjChrRes->SetUpdateRate(1.0);
   }

   this->muObjects.misc2->gfModelAnimation->m_anmObjChrRes->SetUpdateRate(1.0);
   this->muObjects.misc2->gfModelAnimation->m_anmObjMatClrRes->SetUpdateRate(1.0);
   scene->Insert(scene->sceneItemsCount, this->muObjects.misc2->scnObj);
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
   this->muObjects.mainScene = 0;
   this->muObjects.stageProgess = 0;
   this->muObjects.misc1 = 0;
   this->muObjects.misc2 = 0;
   this->muObjects.allyPointer = 0;
   for (int i = 0; i < totalTeamPortraits; i++)
   {
      this->muObjects.enemies[i] = 0;
   }
   for (int i = 0; i < totalAllies; i++)
   {
      this->muObjects.allies[i] = 0;
   }

   this->scnMdl = 0;

   this->files.mainScene = muFileIOHandle();
   this->files.charCommon = muFileIOHandle();
   this->files.miniGame = muFileIOHandle();
   for (int i = 0; i < totalEnemies; i++)
   {
      this->files.enemies[i] = muFileIOHandle();
   }
   for (int i = 0; i < totalAllies; i++)
   {
      this->files.enemies[i] = muFileIOHandle();
   }

   this->resFiles.mainScene = 0;
   this->resFiles.charCommon = 0;
   this->resFiles.miniGame = 0;
   for (int i = 0; i < totalEnemies; i++)
   {
      this->resFiles.enemies[i] = 0;
   }
   for (int i = 0; i < totalAllies; i++)
   {
      this->resFiles.allies[i] = 0;
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

   delete this->muObjects.mainScene;
   delete this->muObjects.stageProgess;
   delete this->muObjects.misc1;
   delete this->muObjects.misc2;
   delete this->muObjects.allyPointer;
   for (int i = 0; i < totalTeamPortraits; i++)
   {
      delete this->muObjects.enemies[i];
   }
   for (int i = 0; i < totalAllies; i++)
   {
      delete this->muObjects.allies[i];
   }

   this->files.mainScene.cancelRequest();
   this->files.charCommon.cancelRequest();
   this->files.miniGame.cancelRequest();
   for (int i = 0; i < totalEnemies; i++)
   {
      this->files.enemies[i].cancelRequest();
   }
   for (int i = 0; i < totalAllies; i++)
   {
      this->files.enemies[i].cancelRequest();
   }

   delete this->resFiles.mainScene;
   delete this->resFiles.charCommon;
   delete this->resFiles.miniGame;
   for (int i = 0; i < totalEnemies; i++)
   {
      delete this->resFiles.enemies[i];
   }
   for (int i = 0; i < totalAllies; i++)
   {
      delete this->resFiles.allies[i];
   }
}
bool muIntroTask::isLoadFinished()
{
   if (!this->files.mainScene.isReady())
      return false;
   if (!this->files.charCommon.isReady())
      return false;
   if (!this->files.miniGame.isReady())
      return false;
   for (int i = 0; i < totalEnemies; i++)
   {
      if (!this->files.enemies[i].isReady())
         return false;
   }
   for (int i = 0; i < totalAllies; i++)
   {
      if (!this->files.allies[i].isReady())
         return false;
   }

   return muMenu::isLoadFinishMenuSound();
}

void muIntroTask::initVersusData()
{
   // default state of stuff, unused
   this->progression = 0;
   this->enemyCount = 0;
   this->allyCount = 0;
   this->mode = versus;

   gmGlobalModeMelee *mode = g_GameGlobal->m_modeMelee;
   bool isTeams = mode->m_meleeInitData.m_isTeams;

   // first team ID, once a team is established, each member will be together on this side
   int team1ID = -1;
   for (int i = 0; i < 4; i++) // for each port
   {
      gmPlayerInitData *player = &mode->m_playersInitData[i];
      if (player->m_initState < 3) // if port in use (0 = player, 1 = cpu)
      {
         int playerFighter = muMenu::exchangeGmCharacterKind2MuStockchkind(player->m_slotID); // convert to fighter stock id
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

static nw4r::g3d::ResFile *loadFile(muFileIOHandle *file)
{
   void *buffer = 0;
   if (file->getReturnStatus() != 0x15)
   {
      buffer = file->getBuffer();
      file->release();
   }
   if (buffer != 0)
   {
      nw4r::g3d::ResFile::Init(&buffer);
   }
   return (nw4r::g3d::ResFile *)buffer;
}