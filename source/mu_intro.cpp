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
muIntroTask *muIntroTask::create()
{
   muIntroTask *intro = new (MenuInstance) muIntroTask();
   intro->getStageSetting();
   intro->files[0].readRequest("/menu/intro/enter/cmn.brres", MenuResource, 0, 0);
   if (intro->mode != breakTheTargets)
   {
      intro->createCharModel();
   }
   else
   {
      char filename[32];
      sprintf(filename, "/menu/intro/enter/mini%d.brres", 1);
      intro->files[5].readRequest(filename, MenuResource, 0, 0);
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

extern gfGameApplication *g_gfGameApplication;
extern sndSystem *g_sndSystem;
void muIntroTask::processDefault()
{
   if (this->soundScriptStarted == 0)
   {
      bool ready = true;
      for (int i = 0; i < 8; i++)
      {
         if (!this->files[i].isReady())
         {
            ready = false;
            break;
         }
      }
      if (!muMenu::isLoadFinishMenuSound())
      {
         ready = false;
      }
      if (ready)
      {
         void *buffer = 0;
         if (this->files[0].getReturnStatus() != 0x15)
         {
            buffer = this->files[0].getBuffer();
            this->files[0].release();
         }
         if (buffer != 0)
         {
            nw4r::g3d::ResFile::Init(&buffer);
         }
         this->resFiles[0] = (nw4r::g3d::ResFile *)buffer;

         this->createMuObjects(mapFileList, 1, &this->resFiles[0]);

         if (this->mode != breakTheTargets)
         {
            this->loadCharModel();
         }
         else
         {
            buffer = 0;
            if (this->files[5].getReturnStatus() != 0x15)
            {
               buffer = this->files[5].getBuffer();
               this->files[5].release();
            }
            if (buffer != 0)
            {
               nw4r::g3d::ResFile::Init(&buffer);
            }
            this->resFiles[7] = (nw4r::g3d::ResFile *)buffer;
            this->muObjects[4] = MuObject::create(&this->resFiles[7], 18, (char)0, 0, MenuInstance);
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
         scIntro *_intro = (scIntro *)gfSceneManager::getInstance()->currentScene;
         (*_intro->menuRoot)->scene->Insert((*_intro->menuRoot)->scene->sceneItemsCount, currentMu->scnObj);
         if (this->mode != breakTheTargets)
         {
            setProgressionMeter(this->progression);
         }
         else
         {
            this->muObjects[4]->gfModelAnimation->anmObjChrRes->SetUpdateRate(1.0);
            this->muObjects[4]->gfModelAnimation->anmObjMatClrRes->SetUpdateFrame(1.0);
            scIntro *_intro = (scIntro *)gfSceneManager::getInstance()->currentScene;
            ScnGroup *scene = (*_intro->menuRoot)->scene;
            scene->Insert(scene->sceneItemsCount, this->muObjects[4]->scnObj);
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
   scIntro *_intro = (scIntro *)manager->currentScene;

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
         if (language == 0)
         {
            charLineLength = charSfxId; // getEnglishVoiceLength(this->enemies[i].charId);
         }
         else
         {
            charLineLength = charSfxId + 1; // getJapaneseVoiceLength(this->enemies[i].charId);
         }
         this->addScriptEntry(charSfxId, charLineLength);
      }
   }
}
void muIntroTask::loadCharModel()
{
   void *buffer = 0;
   if (this->files[1].getReturnStatus() != 0x15)
   {
      buffer = this->files[1].getBuffer();
      this->files[1].release();
   }
   if (buffer != 0)
   {
      nw4r::g3d::ResFile::Init(&buffer);
   }
   this->resFiles[1] = (nw4r::g3d::ResFile *)buffer;
   this->createMuObjects(panelList, 4, &this->resFiles[1]);
   this->scnMdl = ScnMdl::Construct(gfHeapManager::getMEMAllocator(MenuInstance), 0, 0xD, this->muObjects[2]->scnObj);

   if (this->mode == teams)
   {

      char str1[32];
      char str2[32];
      char str3[32];
      this->getEnemyResFileName(str1, str2, str3, this->enemies[0].charId, standardFighter);
      buffer = 0;
      if (this->files[2].getReturnStatus() != 0x15)
      {
         buffer = this->files[2].getBuffer();
         this->files[2].release();
      }
      if (buffer != 0)
      {
         nw4r::g3d::ResFile::Init(&buffer);
      }
      this->resFiles[2] = (nw4r::g3d::ResFile *)buffer;
      for (int i = 0; i < 11; i++)
      {
         MuObject *newMu = MuObject::create(&this->resFiles[2], 0x1C - i, 0, 0, MenuInstance);
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
         this->muObjects[i + 4] = newMu;
      }
   }
   else if (this->mode == standard)
   {
      for (int i = 0; i < this->enemyCount; i++)
      {
         buffer = 0;
         if (this->files[2 + i].getReturnStatus() != 0x15)
         {
            buffer = this->files[2 + i].getBuffer();
            this->files[2 + i].release();
         }
         if (buffer != 0)
         {
            nw4r::g3d::ResFile::Init(&buffer);
         }
         this->resFiles[2 + i] = (nw4r::g3d::ResFile *)buffer;

         char str1[32];
         char str2[32];
         char str3[32];
         this->getEnemyResFileName(str1, str2, str3, this->enemies[i].charId, this->enemies[i].displayId);
         MuObject *newMu = MuObject::create(&this->resFiles[2 + i], 0x1C - i, 0, 0, MenuInstance);

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
         this->muObjects[i + 4] = newMu;
      }
   }
   // allies loop
   if (this->mode != breakTheTargets)
   {
      for (int i = 0; i < this->allyCount; i++)
      {
         buffer = 0;
         if (this->files[6 + i].getReturnStatus() != 0x15)
         {
            buffer = this->files[6 + i].getBuffer();
            this->files[6 + i].release();
         }
         if (buffer != 0)
         {
            nw4r::g3d::ResFile::Init(&buffer);
         }
         this->resFiles[6 + i] = (nw4r::g3d::ResFile *)buffer;

         char str1[32];
         char str2[32];
         char str3[32];
         this->getEnemyResFileName(str1, str2, str3, this->allies[i].charId, this->allies[i].displayId);
         MuObject *newMu = MuObject::create(&this->resFiles[6 + i], 0x1E - i, 0, 0, MenuInstance);

         newMu->changeNodeAnimN(str2);
         newMu->gfModelAnimation->anmObjChrRes->SetUpdateRate(1.0);
         newMu->changeClrAnimN(str2);
         newMu->gfModelAnimation->anmObjMatClrRes->SetUpdateFrame(1.0);
         newMu->changeVisAnimN(str3);
         newMu->setFrameVisible(5.0);
         newMu->gfModelAnimation->anmObjVisRes->SetUpdateFrame(0.0);

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
      ScnMdl::Pushback(this->scnMdl, this->muObjects[4 + i]->scnObj, targetString);
   }
   for (int i = 0; i < this->allyCount; i++)
   {
      char targetString[32];
      sprintf(targetString, "pos%02d", 0x1E + i);
      ScnMdl::Pushback(this->scnMdl, this->muObjects[16 + i]->scnObj, targetString);
   }
}

void muIntroTask::createCharModel()
{
   if (this->mode == breakTheTargets)
   {
      return;
   }
   this->files[1].readRequest("/menu/intro/enter/chrcmn.brres", MenuResource, 0, 0);
   if (this->mode == teams)
   {
      char str1[32];
      char str2[32];
      char str3[32];
      fighter enemy = this->enemies[0];
      this->getEnemyResFileName(str1, str2, str3, enemy.charId, standardFighter);
      this->files[2].readRequest(str1, MenuInstance, 0, 0);
   }
   else
   {
      for (int i = 0; i < this->enemyCount; i++)
      {
         char str1[32];
         char str2[32];
         char str3[32];
         this->getEnemyResFileName(str1, str2, str3, this->enemies[i].charId, this->enemies[i].displayId);
         this->files[2 + i].readRequest(str1, MenuInstance, 0, 0);
      }
   }
   for (int i = 0; i < this->allyCount; i++)
   {
      char str1[32];
      char str2[32];
      char str3[32];
      this->getEnemyResFileName(str1, str2, str3, this->allies[i].charId, this->allies[i].displayId);
      this->files[6 + i].readRequest(str1, MenuInstance, 0, 0);
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
   int fighterFileId = fighterId + 1; // convertFighterID(fighterId);
   sprintf(str1, "/menu/intro/enter/%s%04d.brres", "chr", fighterFileId);
   sprintf(str2, "ItrSimple%s%04d_TopN__%d", "Chr", fighterFileId, displayType);
   sprintf(str3, "ItrSimple%s%04d_TopN__0", "Chr", fighterFileId);
}

void muIntroTask::setProgressionMeter(int progression)
{
   this->muObjects[1]->setFrameMatCol((float)progression);

   scIntro *_intro = (scIntro *)gfSceneManager::getInstance()->currentScene;
   ScnGroup *scene = (*_intro->menuRoot)->scene;
   scene->Insert(scene->sceneItemsCount, this->muObjects[1]->scnObj);
   scene->Insert(scene->sceneItemsCount, this->scnMdl);

   this->muObjects[2]->gfModelAnimation->anmObjChrRes->SetUpdateRate(1.0);
   this->muObjects[2]->gfModelAnimation->anmObjMatClrRes->SetUpdateFrame(1.0);

   if (this->allyCount != 0)
   {
      scene->Insert(scene->sceneItemsCount, this->muObjects[15]->scnObj);
      this->muObjects[15]->gfModelAnimation->anmObjChrRes->SetUpdateRate(1.0);
   }

   this->muObjects[3]->gfModelAnimation->anmObjChrRes->SetUpdateRate(1.0);
   this->muObjects[3]->gfModelAnimation->anmObjMatClrRes->SetUpdateFrame(1.0);
   scene->Insert(scene->sceneItemsCount, this->muObjects[3]->scnObj);
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
      this->muObjects[i] = 0;
   }
   this->scnMdl = 0;
   for (int i = 0; i < 8; i++)
   {
      this->files[i] = muFileIOHandle();
   }
   this->scriptCurrent = 0;
   this->scriptCount = 0;
   this->commonFilePre = 0;
   this->voiceLineCurrentTime = 0;
}
muIntroTask::~muIntroTask()
{
   // for (int i = 0; i < 8; i++)
   //{
   //    delete &this->files[i];
   // }
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