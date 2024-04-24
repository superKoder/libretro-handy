//
// Copyright (c) 2004 K. Wilkins : original, for Handy emulator.
// Copyright (c) 2024 superKoder : later improvements for Handy MP.
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from
// the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not
//    be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//

//////////////////////////////////////////////////////////////////////////////
//                       Handy - An Atari Lynx Emulator                     //
//                          Copyright (c) 1996,1997                         //
//                                 K. Wilkins                               //
//////////////////////////////////////////////////////////////////////////////
// System object header file                                                //
//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// This header file provides the interface definition and inline code for   //
// the system object, this object if what binds together all of the Handy   //
// hardware enmulation objects, its the glue that holds the system together //
//                                                                          //
//    K. Wilkins                                                            //
// August 1997                                                              //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////
// Revision History:                                                        //
// -----------------                                                        //
//                                                                          //
// 01Aug1997 KW Document header added & class documented.                   //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#ifndef SYSTEM_H
#define SYSTEM_H

#pragma inline_depth (255)
#pragma inline_recursion (on)

#include "machine.h"
#include "errorinterface.h"

class C65C02;  // fwd decl

#define HANDY_SYSTEM_FREQ                       16000000
#define HANDY_TIMER_FREQ                        20
#define HANDY_AUDIO_SAMPLE_FREQ                 48000
#define HANDY_AUDIO_SAMPLE_PERIOD               (HANDY_SYSTEM_FREQ/HANDY_AUDIO_SAMPLE_FREQ)
#define HANDY_AUDIO_WAVESHAPER_TABLE_LENGTH     0x200000

#define HANDY_AUDIO_BUFFER_SIZE                 (HANDY_AUDIO_SAMPLE_FREQ)


#define HANDY_FILETYPE_LNX      0
#define HANDY_FILETYPE_HOMEBREW 1
#define HANDY_FILETYPE_SNAPSHOT 2
#define HANDY_FILETYPE_ILLEGAL  3
#define HANDY_FILETYPE_RAW      4

#define HANDY_SCREEN_WIDTH   160
#define HANDY_SCREEN_HEIGHT  102

#include <functional>

typedef struct lssfile
{
   UBYTE *memptr;
   ULONG index;
   ULONG index_limit;
   UBYTE nul_stream;
} LSS_FILE;

int lss_read(void* dest, int varsize, int varcount, LSS_FILE *fp);
int lss_write(void* src, int varsize, int varcount, LSS_FILE *fp);
int lss_printf(LSS_FILE *fp, const char *str);

//
// Define the interfaces before we start pulling in the classes
// as many classes look for articles from the interfaces to
// allow compilation

class CSystem;
struct C6502_REGS;   // fwd decl

//
// Now pull in the parts that build the system
//
#include "lynxbase.h"
//#include "memfault.h"
#include "ram.h"
#include "rom.h"
#include "memmap.h"
#include "cart.h"
#include "eeprom.h"
#include "susie.h"
#include "mikie.h"
#include "c65c02.h"

#include <stdint.h>

#define TOP_START   0xfc00
#define TOP_MASK    0x03ff
#define TOP_SIZE    0x400
#define SYSTEM_SIZE 65536

#define LSS_VERSION_OLD "LSS2"
#define LSS_VERSION     "LSS3"

class CSystem
{
   public:
      CSystem(const char *gamefile,
              const UBYTE *gamedata,
              ULONG gamesize,
              const char *romfile,
              bool useEmu,
              const char *eepromfile, 
              int player_id);
      ~CSystem();
    void SaveEEPROM(void);
      uint8_t ID = 0;

   public:
      void HLE_BIOS_FE00(void);
      void HLE_BIOS_FE19(void);
      void HLE_BIOS_FE4A(void);
      void HLE_BIOS_FF80(void);
      void Reset(void);
      size_t ContextSize(void);
      bool ContextSave(LSS_FILE *fp);
      bool ContextLoad(LSS_FILE *fp);

      void Update(void);

      void Overclock(void);

      inline void FetchAudioSamples(void)
      {
         mMikie->AudioEndOfFrame();
      }

      //
      // We MUST have separate CPU & RAM peek & poke handlers as all CPU accesses must
      // go thru the address generator at $FFF9
      //
      // BUT, Mikie video refresh & Susie see the whole system as RAM
      //
      // Complete and utter wankers, its taken me 1 week to find the 2 lines
      // in all the documentation that mention this fact, the mother of all
      // bugs has been found and FIXED.......

      //
      // CPU
      //
      inline void  Poke_CPU(ULONG addr, UBYTE data) { mMemoryHandlers[addr]->Poke(addr,data);};
      inline UBYTE Peek_CPU(ULONG addr) { return mMemoryHandlers[addr]->Peek(addr);};
      inline void  PokeW_CPU(ULONG addr,UWORD data) { mMemoryHandlers[addr]->Poke(addr,data&0xff);addr++;mMemoryHandlers[addr]->Poke(addr,data>>8);};
      inline UWORD PeekW_CPU(ULONG addr) {return ((mMemoryHandlers[addr]->Peek(addr))+(mMemoryHandlers[addr]->Peek(addr+1)<<8));};

      //
      // RAM
      //
      inline void  Poke_RAM(ULONG addr, UBYTE data) { mRam->Poke(addr,data);};
      inline UBYTE Peek_RAM(ULONG addr) { return mRam->Peek(addr);};
      inline void  PokeW_RAM(ULONG addr,UWORD data) { mRam->Poke(addr,data&0xff);addr++;mRam->Poke(addr,data>>8);};
      inline UWORD PeekW_RAM(ULONG addr) {return ((mRam->Peek(addr))+(mRam->Peek(addr+1)<<8));};

      // High level cart access for debug etc

      inline void  Poke_CART(ULONG addr, UBYTE data) {mCart->Poke(addr,data);};
      inline UBYTE Peek_CART(ULONG addr) {return mCart->Peek(addr);};
      inline void  CartBank(EMMODE bank) {mCart->BankSelect(bank);};
      inline ULONG CartSize(void) {return mCart->ObjectSize();};
      inline const char* CartGetName(void) { return mCart->CartGetName();};
      inline const char* CartGetManufacturer(void) { return mCart->CartGetManufacturer();};
      inline ULONG CartGetRotate(void) {return mCart->CartGetRotate();};

      // Low level cart access for Suzy, Mikey

      inline void  Poke_CARTB0(UBYTE data) {mCart->Poke0(data);};
      inline void  Poke_CARTB1(UBYTE data) {mCart->Poke1(data);};
      inline void  Poke_CARTB0A(UBYTE data) {mCart->Poke0A(data);};
      inline void  Poke_CARTB1A(UBYTE data) {mCart->Poke1A(data);};
      inline UBYTE Peek_CARTB0(void) {return mCart->Peek0();}
      inline UBYTE Peek_CARTB1(void) {return mCart->Peek1();}
      inline UBYTE Peek_CARTB0A(void) {return mCart->Peek0A();}
      inline UBYTE Peek_CARTB1A(void) {return mCart->Peek1A();}
      inline void  CartAddressStrobe(bool strobe) {mCart->CartAddressStrobe(strobe);};
      inline void  CartAddressData(bool data) {mCart->CartAddressData(data);};

      // Low level CPU access

      void   SetRegs(C6502_REGS &regs);
      void   GetRegs(C6502_REGS &regs);

      // Mikey system interfacing

      void   DisplaySetAttributes(ULONG rotate, 
                                  ULONG format,
                                  ULONG pitch, 
                                  CMikie::DisplayCallback callback, 
                                  ULONG objref) { 
         mMikie->DisplaySetAttributes(rotate, format, pitch, callback, objref); 
      };

      void   ComLynxCable(int status) { mMikie->ComLynxCable(status); };
      void   ComLynxRxData(int data)  { mMikie->ComLynxRxData(data); };
      void   ComLynxTxCallback(void (*function)(int data, ULONG objref), ULONG objref) { mMikie->ComLynxTxCallback(function, objref); };

      // Suzy system interfacing

      ULONG  PaintSprites(void) {return mSusie->PaintSprites();};

      // Miscellaneous

      void   SetButtonData(ULONG data) {mSusie->SetButtonData(data);};
      ULONG  GetButtonData(void) {return mSusie->GetButtonData();};
      void   SetCycleBreakpoint(ULONG breakpoint) {mCycleCountBreakpoint=breakpoint;};
      UBYTE* GetRamPointer(void) {return mRam->GetRamPointer();};

   public:
      ULONG         mCycleCountBreakpoint;
      CLynxBase     *mMemoryHandlers[SYSTEM_SIZE];
      CCart         *mCart;
      CRom          *mRom;
      CMemMap       *mMemMap;
      CRam          *mRam;
      C65C02        *mCpu;
      CMikie        *mMikie;
      CSusie        *mSusie;
      CEEPROM       *mEEPROM;

      ULONG         mFileType;


   public: // ex-globals
      ULONG   mSystemCycleCount=0;
      ULONG   mLastRunCycleCount=0;
      ULONG   mNextTimerEvent=0;
      ULONG   mCPUWakeupTime=0;
      ULONG   mIRQEntryCycle=0;
      ULONG   mCPUBootAddress=0;
      ULONG   mSingleStepMode=FALSE;
      ULONG   mSingleStepModeSprites=FALSE;  // not actually used?
      ULONG   mSystemIRQ=FALSE;
      ULONG   mSystemNMI=FALSE;
      ULONG   mSystemCPUSleep=FALSE;
      ULONG   mSystemCPUSleep_Saved=FALSE;   // not actually used?
      ULONG   mSystemHalt=FALSE;
      ULONG   mThrottleNextCycleCheckpoint=0;

      ULONG   mAudioEnabled=FALSE;
      UBYTE   mAudioBuffer[HANDY_AUDIO_BUFFER_SIZE];
      ULONG   mAudioBufferPointer=0;
      ULONG   mAudioLastUpdateCycle=0;

      UBYTE   mSkipFrame=FALSE;

      CErrorInterface *mError=NULL;

   private:    // ex-globals
      ULONG   mBreakpointHit=FALSE;
      ULONG   mThrottleMaxPercentage=100;
      ULONG   mThrottleLastTimerCount=0;

      volatile ULONG mTimerCount=0;
};

#endif
