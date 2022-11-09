Fight intro [Eon]
op li r14, 0x11 @ $806dcb20
HOOK @ $806DCB58
{
  cmplwi r0, 0x11

  blt %end%
  bne %end%

loadIntroScene:
  #lbz r3, #getChrKind
  #lis r12, 0x800A
  #ori r12, r12, 0xF5B4
  

  lwz r3, 0x14(r15)
  lis r4, 0x8070
  ori r4, r4, 0x2508 # 0x2508+688
  lis r12, 0x8002
  ori r12, r12, 0xD3F4
  mtctr r12
  bctrl
  mr r28, r3
#  0x364 = how many opponents there are
  li r0, 0
  stw r0, 0x364(r3)
#  0x380 = num of ally
  stw r0, 0x380(r3)
end:
#  0x35C = stage num
  li r0, 0x0
  stw r0, 0x35C(r28)
#  0x360 = team "blank" where blank is 
  li r0, 0
  stw r0, 0x360(r28)
  


#8094667c team found hereish

#getSlotManager
  li r31, 0
  b slotLoopStart
slotLoop:
  lwz r4, 0x50(r3) #getSlotNo/[soHeapModuleImpl]
  cmpwi r4, -1
  beq slotLoopNext
  lwz r6, 0x48(r3)
  mulli r7, r6, 92
  lwz r5, 0x40(r3)
  lbz r0, 0x14(r5)
  add r7, r5, r7
  cmpwi r0, 0
  bne team
  lbz r0, 0x13(r5)
  cmpwi r0, 0
  bne team
  mr r29, r6
  b setStats
team:
  lbz r29, 0xA3(r7)
setStats:
#r7 = teamNumber
#
#
  lbz r3, 0x5a(r3) #getFighterId actual id you know that 0x110 thing
  addi r4, r1, 0x4 # needs 0x10 bytes since it gets all 4 fighter IDs (#meaning i could load all 4 for a custom quadruplet scene :eyes:)
  lis r12, 0x800a
  ori r12, r12, 0xf5B4
  mtctr r12 
  bctrl 

  cmpwi r29, 0
  bne teamTwo
teamOne:
  lwz r4, 0x364(r28)
  addi r0, r4, 1
  stw r0, 0x364(r28)
  mulli r4, r4, 0x8
  addi r4, r4, 0x368
  stwx r3, r28, r4
  b loadFile
teamTwo:
  lwz r4, 0x380(r28)
  addi r0, r4, 1
  stw r0, 0x380(r28)
  mulli r4, r4, 0x8
  addi r4, r4, 0x384
  stwx r3, r28, r4
loadFile:
#  mulli r0, r31, 4
#  lis r3, 0x80B9
#  lwz r3, -0x5930(r3)
#  lwzx r3, r3, r0
#  lbz r3, 0x5a(r3) #getFighterId actual id you know that 0x110 thing
#  lis r12, 0x8084
#  ori r12, r12, 0x45EC
#  mtctr r12 
#  bctrl 
#  mr r5, r3
#  mulli r0, r31, 4
#  lis r3, 0x80B9
#  lwz r3, -0x5930(r3)
#  lwzx r3, r3, r0
#  #lbz r5, 0x5a(r3) #getFighterId actual id you know that 0x110 thing
#
#  lwz r4, 0x50(r3) #getSlotNo/[soHeapModuleImpl]
#  #lbz r5, 0x5a(r3) #getFighterId actual id you know that 0x110 thing
#  lbz r6, 0x5b(r3) #getFighterColour
#
#  lis r3, 0x80B8
#  addi r3, r3, 0x7fcc
#  lis r12, 0x8085
#  ori r12, r12, 0x0144
#  mtctr r12 
#  bctrl #    bl getModelResId/[ftCommonDataAccesser]
#  cmplwi r3, 0xFFFF
#  mr r30, r3
#  beq slotLoopNext
#  li r3, 0
#  lis r12, 0x8077
#  ori r12, r12, 0xA894
#  mtctr r12 
#  bctrl #    bl 0x8077A894 #getManager/[soArchiveDb]
#  mr r4, r30
#  li r5, 2 
#  li r6, 0 #getResourceIdAccesser.getMdlResIndex
#  li r7, 0 # always set to 0 # getGroupNo/[soResourceModuleImpl](1) #getResGroupNo/[ftFighterBuildData] #8089d7c8
#  li r8, -1    
#  lis r12, 0x8004
#  ori r12, r12, 0x65F0
#  mtctr r12 
#  bctrl #    bl 0x800465F0 #getResFileFromId/[utArchiveManager]

slotLoopNext:
  #stLoaderManager
  addi r31, r31, 1
slotLoopStart:
  cmpwi r31, 4
  mulli r0, r31, 4
  lis r3, 0x80B9
  lwz r3, -0x5930(r3)
  lwzx r3, r3, r0
  bne slotLoop
slotLoopEnd:
  lwz r3, 0x14(r15)
  lis r4, 0x8070
  ori r4, r4, 0x2508 # 0x2508+688
  li r5, 0
  lis r12, 0x8002
  ori r12, r12, 0xD5AC
  mtctr r12
  bctrl
return: 
  li r0, 8
  stw r0, 0x8(r15)    
  lis r12, 0x806D
  ori r12, r12, 0xCE70
  mtctr r12 
  bctr 
}