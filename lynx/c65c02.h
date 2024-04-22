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
// 65C02 Emulation class                                                    //
//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// This class emulates a 65C02 processor. It is interfaced to the rest of   //
// the system via the PEEK/POKE macros and a number of global variables     //
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

#ifndef C65C02_H
#define C65C02_H

#include "handy.h"

//
// Handy definitions
//

#define NMI_VECTOR	0xfffa
#define BOOT_VECTOR	0xfffc
#define IRQ_VECTOR	0xfffe

#define MAX_CPU_BREAKPOINTS	8

//
// ACCESS MACROS
//

//#define CPU_PEEK(m)				(mSystem.Peek_CPU(m))
//#define CPU_PEEKW(m)			(mSystem.PeekW_CPU(m))
//#define CPU_POKE(m1,m2)			(mSystem.Poke_CPU(m1,m2))

#define CPU_PEEK(m)				(((m<0xfc00)?mRamPointer[m]:mSystem.Peek_CPU(m)))
#define CPU_PEEKW(m)			(((m<0xfc00)?(mRamPointer[m]+(mRamPointer[m+1]<<8)):mSystem.PeekW_CPU(m)))
#define CPU_POKE(m1,m2)			{if(m1<0xfc00) mRamPointer[m1]=m2; else mSystem.Poke_CPU(m1,m2);}


enum
{
   illegal=0,
   accu,
   imm,
   absl,
   zp,
   zpx,
   zpy,
   absx,
   absy,
   iabsx,
   impl,
   rel,
   zrel,
   indx,
   indy,
   iabs,
   ind
};

struct C6502_REGS
{
   int PS;		// Processor status register   8 bits
   int A;		// Accumulator                 8 bits
   int X;		// X index register            8 bits
   int Y;		// Y index register            8 bits
   int SP;		// Stack Pointer               8 bits
   int Opcode;	// Instruction opcode          8 bits
   int Operand;// Intructions operand		  16 bits
   int PC;		// Program Counter            16 bits
   bool NMI;
   bool IRQ;
   bool WAIT;
};

//
// The CPU emulation macros
//
#include "c6502mak.h"
//
// The CPU emulation macros
//

class C65C02
{
   public:
      C65C02(CSystem& parent);

      ~C65C02()
      {
      }

   public:
      void Reset(void);

      inline bool ContextSave(LSS_FILE *fp)
      {
         int mPS;
         mPS=PS();
         if(!lss_printf(fp,"C6502::ContextSave")) return 0;
         if(!lss_write(&mA,sizeof(ULONG),1,fp)) return 0;
         if(!lss_write(&mX,sizeof(ULONG),1,fp)) return 0;
         if(!lss_write(&mY,sizeof(ULONG),1,fp)) return 0;
         if(!lss_write(&mSP,sizeof(ULONG),1,fp)) return 0;
         if(!lss_write(&mPS,sizeof(ULONG),1,fp)) return 0;
         if(!lss_write(&mPC,sizeof(ULONG),1,fp)) return 0;
         if(!lss_write(&mIRQActive,sizeof(ULONG),1,fp)) return 0;
         return 1;
      }

      inline bool ContextLoad(LSS_FILE *fp)
      {
         int mPS;
         char teststr[100]="XXXXXXXXXXXXXXXXXX";
         if(!lss_read(teststr,sizeof(char),18,fp)) return 0;
         if(strcmp(teststr,"C6502::ContextSave")!=0) return 0;
         if(!lss_read(&mA,sizeof(ULONG),1,fp)) return 0;
         if(!lss_read(&mX,sizeof(ULONG),1,fp)) return 0;
         if(!lss_read(&mY,sizeof(ULONG),1,fp)) return 0;
         if(!lss_read(&mSP,sizeof(ULONG),1,fp)) return 0;
         if(!lss_read(&mPS,sizeof(ULONG),1,fp)) return 0;
         if(!lss_read(&mPC,sizeof(ULONG),1,fp)) return 0;
         if(!lss_read(&mIRQActive,sizeof(ULONG),1,fp)) return 0;
         PS(mPS);
         return 1;
      }

      void Update(void);

      void SetRegs(C6502_REGS &regs);

      void GetRegs(C6502_REGS &regs);

      inline int GetPC(void) { return mPC; }

      inline void xILLEGAL(void)
      {
         handy_log(RETRO_LOG_ERROR,
               "C65C02::Update() - Illegal opcode (%02x) at PC=$%04x.\n",
               mOpcode, mPC);
      }

   private:
      CSystem	&mSystem;

      // CPU Flags & status

      int mA;		// Accumulator                 8 bits
      int mX;		// X index register            8 bits
      int mY;		// Y index register            8 bits
      int mSP;		// Stack Pointer               8 bits
      int mOpcode;  // Instruction opcode          8 bits
      int mOperand; // Intructions operand		  16 bits
      int mPC;		// Program Counter            16 bits

      int mN;		// N flag for processor status register
      int mV;		// V flag for processor status register
      int mB;		// B flag for processor status register
      int mD;		// D flag for processor status register
      int mI;		// I flag for processor status register
      int mZ;		// Z flag for processor status register
      int mC;		// C flag for processor status register

      int mIRQActive;

      UBYTE *mRamPointer;

      // Associated lookup tables

      int mBCDTable[2][256];

      //
      // Opcode prototypes
      //

   private:

      // Answers value of the Processor Status register
      int PS() const
      {
         UBYTE ps = 0x20;
         if(mN) ps|=0x80;
         if(mV) ps|=0x40;
         if(mB) ps|=0x10;
         if(mD) ps|=0x08;
         if(mI) ps|=0x04;
         if(mZ) ps|=0x02;
         if(mC) ps|=0x01;
         return ps;
      }


      // Change the processor flags to correspond to the given value
      void PS(int ps)
      {
         mN=ps&0x80;
         mV=ps&0x40;
         mB=ps&0x10;
         mD=ps&0x08;
         mI=ps&0x04;
         mZ=ps&0x02;
         mC=ps&0x01;
      }

};


#endif

