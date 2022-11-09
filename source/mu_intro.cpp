#pragma inline_max_size 512

#include <memory.h>
#include "mu_intro.h"
#include "sc_intro.h"
#include <OSError.h>
#include <gf/gf_scene_manager.h>
#include "printf.h"
#include <mu/mu_menu.h>
#include <gf/gf_model.h>
#include <gf/gf_heap_manager.h>
#include <string.h>
#include <nw4r/g3d/g3d_scnmdl.h>
#include <gf/gf_game_application.h>
#include <snd_system.h>
#include <flush_cache.h>
#include <gm/gm_global.h>
#include <ft/ft_slot.h>
#include <so/so_archive_db.h>
#include <ut/ut_archive_manager.h>
#include <st/st_loader_player.h>

const scriptEntry typeVoiceLines[] = {{-1, 0}, {-1, 0}, {0x203F, 0x27}, {0x2040, 0x31}};
static muObjectFlags mapFileList[] = {
    {"ItrSimpleMap0000_TopN", {0x0, 0x0, 0x21, 0x0}},
};
static muObjectFlags panelList[] = {
    {"ItrSimplePanel0000_TopN", {0x1, 0x1, 0x10, 0x0}},
    {"ItrSimplePanel0001_TopN", {0x2, 0x2, 0x11, 0x0}},
    {"ItrSimplePanel0002_TopN", {0x3, 0x3, 0x1F, 0x0}},
    {"ItrSimplePanel0010_TopN", {0xF, 0xF, 0x20, 0x0}},
};

extern gfGameApplication *g_gfGameApplication;
extern sndSystem *g_sndSystem;
extern gmGameGlobal *g_GameGlobal;
extern ftSlotManager *g_ftSlotManager;
extern stLoaderPlayer *g_stLoaderPlayerArray[4];

muIntroTask *muIntroTask::create()
{
   muIntroTask *intro = new (MenuInstance) muIntroTask();
   intro->getStageSetting();
   intro->files.mainScene.readRequest("/menu/intro/enter/cmn.brres", MenuResource, 0, 0);
   if (intro->mode != breakTheTargets)
   {
      intro->createCharModel();
   }
   else
   {
      char filename[32];
      sprintf(filename, "/menu/intro/enter/mini%d.brres", 1);
      intro->files.miniGame.readRequest(filename, MenuResource, 0, 0);
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
      TRK_flush_cache((int)(targetAddress - 4), (int)(targetAddress + 4));
   }

   return intro;
}

void muIntroTask::processDefault()
{
   if (this->soundScriptStarted == 0)
   {
      if (this->isLoadFinished())
      {
         void *buffer = 0;
         if (this->files.mainScene.getReturnStatus() != 0x15)
         {
            buffer = this->files.mainScene.getBuffer();
            this->files.mainScene.release();
         }
         if (buffer != 0)
         {
            nw4r::g3d::ResFile::Init(&buffer);
         }
         this->resFiles.mainScene = (nw4r::g3d::ResFile *)buffer;

         this->createMuObjects(mapFileList, 1, &this->resFiles.mainScene);

         if (this->mode != breakTheTargets)
         {
            buffer = 0;
            if (this->files.charCommon.getReturnStatus() != 0x15)
            {
               buffer = this->files.charCommon.getBuffer();
               this->files.charCommon.release();
            }
            if (buffer != 0)
            {
               nw4r::g3d::ResFile::Init(&buffer);
            }
            this->resFiles.charCommon = (nw4r::g3d::ResFile *)buffer;
            this->createMuObjects(panelList, 4, &this->resFiles.charCommon);
            this->scnMdl = ScnMdl::Construct(gfHeapManager::getMEMAllocator(MenuInstance), 0, 0xD, this->muObjects.misc[0]->scnObj);
            if (this->mode == vsFightIntro)
            {
            }
            else
            {
               this->loadCharModel();
            }
         }
         else
         {
            buffer = 0;
            if (this->files.miniGame.getReturnStatus() != 0x15)
            {
               buffer = this->files.miniGame.getBuffer();
               this->files.miniGame.release();
            }
            if (buffer != 0)
            {
               nw4r::g3d::ResFile::Init(&buffer);
            }
            this->resFiles.miniGame = (nw4r::g3d::ResFile *)buffer;
            this->muObjects.enemies[0] = MuObject::create(&this->resFiles.miniGame, 18, (char)0, 0, MenuInstance);
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
         scIntro *_intro = (scIntro *)gfSceneManager::getInstance()->currentScene;
         (*_intro->menuRoot)->scene->Insert((*_intro->menuRoot)->scene->sceneItemsCount, currentMu->scnObj);
         if (this->mode != breakTheTargets)
         {
            setProgressionMeter(this->progression);
         }
         else
         {
            this->muObjects.enemies[0]->gfModelAnimation->anmObjChrRes->SetUpdateRate(1.0);
            this->muObjects.enemies[0]->gfModelAnimation->anmObjMatClrRes->SetUpdateFrame(1.0);
            scIntro *_intro = (scIntro *)gfSceneManager::getInstance()->currentScene;
            ScnGroup *scene = (*_intro->menuRoot)->scene;
            scene->Insert(scene->sceneItemsCount, this->muObjects.enemies[0]->scnObj);
         }
         int rumble = _intro->rumbleSetting;
         if (rumble == 0x78)
         {
            rumble = -1;
         }
         muMenu::startRumbleController(_intro->activeController, 0x14, rumble);
         g_sndSystem->playSE(0x2b, -1, 0, 0, -1);
         this->commonFilePre = 0;
         this->soundScriptStarted = 1;
      }
   }
   else
   {
      scriptEntry *currentVoiceLine = &this->script[scriptCurrent];
      if (this->voiceLineCurrentTime == 0)
      {
         g_sndSystem->playSE(currentVoiceLine->id, -1, 0, 0, -1);
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
         MuObject *result = MuObject::create(resFile, currData.node, currData.flags[2], 0, MenuInstance);
         this->muObjects.asArray[currData.flags[0] + j] = result;

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
   scIntro *_intro = (scIntro *)manager->currentScene;

   this->progression = _intro->progression;
   this->mode = _intro->mode;
   this->enemyCount = _intro->enemyCount;
   this->allyCount = _intro->allyCount;

   for (int i = 0; i < totalPossibleEnemies; i++)
   {
      fighter *enemy = &this->enemies[i];
      if (i < 3)
      {
         _fighter *enemyData = &_intro->enemies[i];
         enemy->charId = enemyData->charId;
         enemy->displayId = enemyData->displayId;
      }
      else
      {
         enemy->charId = 0;
         enemy->displayId = 0;
      }
   }
   for (int i = 0; i < totalPossibleAllies; i++)
   {
      fighter *ally = &this->allies[i];
      if (i < 2)
      {
         _fighter *allyData = &_intro->allies[i];
         ally->charId = allyData->charId;
         ally->displayId = allyData->displayId;
      }
      else
      {
         ally->charId = 0;
         ally->displayId = 0;
      }
   }

   // for (int i = 0; i < 4; i++)
   // {
   //    stLoaderPlayer *st = g_stLoaderPlayerArray[i];
   //    int team;
   //    if (st->fighters[0].test_0x14 == 0 || st->fighters[0].test_0x10 == 0)
   //    {
   //       team = st->fighters[st->fighterIndex] + 0xA3;
   //    }
   //    else
   //    {
   //       team = st->fighterIndex;
   //    }
   //    int ids[4];
   //    muMenu::exchangeGmCharacterKind2MuStockchkind(ids, st->fighterID);
   //    int fighterId = ids[0];
   //    int costumeId = st->fighterColour;

   //    // save team, set displayId = costumeColour and have slot saved
   // }
   // each fighter entry needs to have a thing identifying their team** and a correct reading of their fighter + costume
   //  if (this->mode == vsFightIntro) {
   //     if (isTeams ) {
   //        totalTeams == x;
   //        if totalTeams == 2 {
   //           split into enemies vs allies according to the teams
   //           script = "mario" "AND" "kirby" "VS" "link" "AND" "peach"
   //           or just
   //           script = "red team" "VS" "blue team"
   //        } else {
   //           set all into the enemies array
   //           set some flag to say which team each is (or just say "colour team")
   //        }
   //     } else {
   //        enemyCount = fighterCount
   //        add all fighters to the enemy array
   //        set some flag to say this is how its done
   //     }
   //  }
}

void muIntroTask::makeSoundScript()
{
   if (this->mode == breakTheTargets)
   {
      this->addScriptEntry(0x2035, 0x5A); // "Break the Targets!"
      return;
   }

   this->addScriptEntry(0x203E, 0x3C); // "VERSUS"
   int count;
   if (this->mode == teams)
   {
      count = 1;
      this->addScriptEntry(0x203C, 0x1E); // "TEAM"
   }
   else
   {
      count = this->enemyCount;
   }
   for (int i = 0; i < count; i++)
   {
      if (i > 0) // if the very first voice line, dont have a seperator
      {
         if (i < count - 1) // if not last in list,
         {
            this->addScriptEntry(-1, 10); // "{comma}"
         }
         else
         {
            this->addScriptEntry(0x203D, 0x2B); // "AND"
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
      int charSfxId = muMenu::exchangeSelchkind2SelCharVoice(this->enemies[i].charId);
      if (charSfxId != -1)
      {
         int language = 0; // getLanguage
         int charLineLength;
         if (g_GameGlobal->getLanguage() == 0)
         {
            charLineLength = muMenu::exchangeSelCharVoice2SelCharVoiceLengthE(charSfxId);
         }
         else
         {
            charLineLength = muMenu::exchangeSelCharVoice2SelCharVoiceLengthJ(charSfxId);
         }
         this->addScriptEntry(charSfxId, charLineLength);
      }
   }
}
void muIntroTask::loadCharModel()
{
   if (this->mode == teams)
   {

      char str1[32];
      char str2[32];
      char str3[32];
      this->getEnemyResFileName(str1, str2, str3, this->enemies[0].charId, standardFighter);
      void *buffer = 0;
      if (this->files.enemies[0].getReturnStatus() != 0x15)
      {
         buffer = this->files.enemies[0].getBuffer();
         this->files.enemies[0].release();
      }
      if (buffer != 0)
      {
         nw4r::g3d::ResFile::Init(&buffer);
      }
      this->resFiles.enemies[0] = (nw4r::g3d::ResFile *)buffer;
      for (int i = 0; i <= 10; i++)
      {
         MuObject *newMu = MuObject::create(&this->resFiles.enemies[0], 0x1C - i, 0, 0, MenuInstance);
         newMu->changeNodeAnimN(str2);
         newMu->gfModelAnimation->anmObjChrRes->SetUpdateRate(1.0);
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
         newMu->gfModelAnimation->anmObjMatClrRes->SetUpdateFrame(style);
         newMu->changeVisAnimN(str3);
         newMu->setFrameVisible(3.0);
         newMu->gfModelAnimation->anmObjVisRes->SetUpdateFrame(0.0);
         this->muObjects.enemies[i] = newMu;
      }
   }
   else if (this->mode == standard)
   {
      for (int i = 0; i < this->enemyCount; i++)
      {
         void *buffer = 0;
         if (this->files.enemies[i].getReturnStatus() != 0x15)
         {
            buffer = this->files.enemies[i].getBuffer();
            this->files.enemies[i].release();
         }
         if (buffer != 0)
         {
            nw4r::g3d::ResFile::Init(&buffer);
         }
         this->resFiles.enemies[i] = (nw4r::g3d::ResFile *)buffer;

         char str1[32];
         char str2[32];
         char str3[32];
         this->getEnemyResFileName(str1, str2, str3, this->enemies[i].charId, this->enemies[i].displayId);
         MuObject *newMu = MuObject::create(&this->resFiles.enemies[i], 0x1C - i, 0, 0, MenuInstance);

         newMu->changeNodeAnimN(str2);
         newMu->gfModelAnimation->anmObjChrRes->SetUpdateRate(1.0);
         newMu->changeClrAnimN(str2);
         newMu->gfModelAnimation->anmObjMatClrRes->SetUpdateFrame(1.0);
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
         newMu->gfModelAnimation->anmObjVisRes->SetUpdateFrame(0.0);
         this->muObjects.enemies[i] = newMu;
      }
   }
   // allies loop
   if (this->mode != breakTheTargets)
   {
      for (int i = 0; i < this->allyCount; i++)
      {
         void *buffer = 0;
         if (this->files.allies[i].getReturnStatus() != 0x15)
         {
            buffer = this->files.allies[i].getBuffer();
            this->files.allies[i].release();
         }
         if (buffer != 0)
         {
            nw4r::g3d::ResFile::Init(&buffer);
         }
         this->resFiles.allies[i] = (nw4r::g3d::ResFile *)buffer;

         char str1[32];
         char str2[32];
         char str3[32];
         this->getEnemyResFileName(str1, str2, str3, this->allies[i].charId, this->allies[i].displayId);
         MuObject *newMu = MuObject::create(&this->resFiles.allies[i], 0x1E - i, 0, 0, MenuInstance);

         newMu->changeNodeAnimN(str2);
         newMu->gfModelAnimation->anmObjChrRes->SetUpdateRate(1.0);
         newMu->changeClrAnimN(str2);
         newMu->gfModelAnimation->anmObjMatClrRes->SetUpdateFrame(1.0);
         newMu->changeVisAnimN(str3);
         newMu->setFrameVisible(5.0);
         newMu->gfModelAnimation->anmObjVisRes->SetUpdateFrame(0.0);

         this->muObjects.allies[i] = newMu;
      }
   }
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
      ScnMdl::Pushback(this->scnMdl, this->muObjects.enemies[i]->scnObj, targetString);
   }
   for (int i = 0; i < this->allyCount; i++)
   {
      char targetString[32];
      sprintf(targetString, "pos%02d", 0x1E + i);
      ScnMdl::Pushback(this->scnMdl, this->muObjects.allies[i]->scnObj, targetString);
   }
}

void muIntroTask::load3DCharModels()
{
   utArchiveManager *archive = (utArchiveManager *)soArchiveDb::getManager(0);

   for (int i = 0; i < this->enemyCount; i++)
   {
      // int modelResId = getResId(8, this->enemies[i].charId, this->enemies[i].charId);
      // int texturesResId = getResId(8, this->enemies[i].charId, this->enemies[i].charId);
      // int animationResId = getResId(8, this->enemies[i].charId, this->enemies[i].charId);
      // this->resFiles[2+i] = ;
   }

   for (int i = 0; i < 1; i++)
   {
      ftSlot *slot = &g_ftSlotManager->slotsContainer->slots[i];
      if (!slot->disabled)
      {
         this->resFiles.enemies[i] = archive->getResFileFromId(slot->getResId(8, /*fighterId*/ 0, /*fighterColour*/ 0), 0, 0, 0, -1);
         nw4r::g3d::ResFile::Init(&this->resFiles.enemies[i]);
         MuObject *newMu = MuObject::create(&this->resFiles.enemies[i], 0x1C - i, 0, 0, MenuInstance);
         this->muObjects.enemies[i] = newMu;
      }
   }
}

void muIntroTask::createCharModel()
{
   this->files.charCommon.readRequest("/menu/intro/enter/chrcmn.brres", MenuResource, 0, 0);
   if (this->mode == vsFightIntro)
   {
      return;
   }
   if (this->mode == teams)
   {
      char str1[32];
      char str2[32];
      char str3[32];
      fighter enemy = this->enemies[0];
      this->getEnemyResFileName(str1, str2, str3, enemy.charId, standardFighter);
      this->files.enemies[0].readRequest(str1, MenuInstance, 0, 0);
   }
   else
   {
      for (int i = 0; i < this->enemyCount; i++)
      {
         char str1[32];
         char str2[32];
         char str3[32];
         this->getEnemyResFileName(str1, str2, str3, this->enemies[i].charId, this->enemies[i].displayId);
         this->files.enemies[i].readRequest(str1, MenuInstance, 0, 0);
      }
   }
   for (int i = 0; i < this->allyCount; i++)
   {
      char str1[32];
      char str2[32];
      char str3[32];
      this->getEnemyResFileName(str1, str2, str3, this->allies[i].charId, this->allies[i].displayId);
      this->files.allies[i].readRequest(str1, MenuInstance, 0, 0);
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

   scIntro *_intro = (scIntro *)gfSceneManager::getInstance()->currentScene;
   ScnGroup *scene = (*_intro->menuRoot)->scene;
   scene->Insert(scene->sceneItemsCount, this->muObjects.stageProgess->scnObj);
   scene->Insert(scene->sceneItemsCount, this->scnMdl);

   this->muObjects.misc[0]->gfModelAnimation->anmObjChrRes->SetUpdateRate(1.0);
   this->muObjects.misc[0]->gfModelAnimation->anmObjMatClrRes->SetUpdateFrame(1.0);

   if (this->allyCount != 0)
   {
      this->muObjects.allyPointer->gfModelAnimation->anmObjChrRes->SetUpdateRate(1.0);
      scene->Insert(scene->sceneItemsCount, this->muObjects.allyPointer->scnObj);
   }

   this->muObjects.misc[1]->gfModelAnimation->anmObjChrRes->SetUpdateRate(1.0);
   this->muObjects.misc[1]->gfModelAnimation->anmObjMatClrRes->SetUpdateFrame(1.0);
   scene->Insert(scene->sceneItemsCount, this->muObjects.misc[1]->scnObj);
}

inline void muIntroTask::addScriptEntry(int id, int length)
{
   scriptEntry *script = &this->script[this->scriptCount++];
   script->id = id;
   script->length = length;
}

muIntroTask::muIntroTask() : gfTask("Intro", 8, 0xf, 8, 1)
{
   for (int i = 0; i < 0x12; i++)
   {
      this->muObjects.asArray[i] = 0;
   }
   this->scnMdl = 0;
   for (int i = 0; i < 8; i++)
   {
      this->files.asArray[i] = muFileIOHandle();
   }
   for (int i = 0; i < 8; i++)
   {
      this->resFiles.asArray[i] = 0;
   }
   this->scriptCurrent = 0;
   this->scriptCount = 0;
   this->commonFilePre = 0;
   this->voiceLineCurrentTime = 0;
}

muIntroTask::~muIntroTask()
{
   delete this->scnMdl;
   for (int i = 0; i < 0x12; i++)
   {
      delete this->muObjects.asArray[i];
   }
   for (int i = 0; i < 8; i++)
   {
      delete this->resFiles.asArray[i];
   }
   for (int i = 0; i < 8; i++)
   {
      this->files.asArray[i].cancelRequest();
   }
}
bool muIntroTask::isLoadFinished()
{
   for (int i = 0; i < 8; i++)
   {
      if (!this->files.asArray[i].isReady())
      {
         return false;
      }
   }
   if (this->mode == vsFightIntro)
   {
      for (int i = 0; i < 4; i++)
      {
         ftSlot *slot = &g_ftSlotManager->slotsContainer->slots[i];
         if (!slot->disabled && !slot->isReady())
         {
            return false;
         }
      }
   }
   return muMenu::isLoadFinishMenuSound();
}