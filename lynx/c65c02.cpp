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

#include "c65c02.h"
#include <stdio.h>

C65C02::C65C02(CSystem& parent)
         :mSystem(parent) 
      {
         // Compute the BCD lookup table
         for(UWORD t=0;t<256;++t)
         {
            mBCDTable[0][t]=((t >> 4) * 10) + (t & 0x0f);
            mBCDTable[1][t]=(((t % 100) / 10) << 4) | (t % 10);
         }
         Reset();
      }

      void C65C02::Reset(void)
      {
         mRamPointer=mSystem.GetRamPointer();
         mA=0;
         mX=0;
         mY=0;
         mSP=0xff;
         mOpcode=0;
         mOperand=0;
         mPC=CPU_PEEKW(BOOT_VECTOR);
         mN=FALSE;
         mV=FALSE;
         mB=FALSE;
         mD=FALSE;
         mI=TRUE;
         mZ=TRUE;
         mC=FALSE;
         mIRQActive=FALSE;

         mSystem.mSystemNMI=FALSE;
         mSystem.mSystemIRQ=FALSE;
         mSystem.mSystemCPUSleep=FALSE;
         mSystem.mSystemCPUSleep_Saved=FALSE;
      }

      void C65C02::Update(void)
      {
         // NMI is currently unused by the lynx so lets save some time
         //
         //			Check NMI & IRQ status, prioritise NMI then IRQ
         //			if(mNMI)
         //			{
         //				// Mark the NMI as services
         //				mNMI=FALSE;
         //				mProcessingInterrupt++;
         //
         //				// Push processor status
         //				CPU_POKE(0x0100+mSP--,mPC>>8);
         //				CPU_POKE(0x0100+mSP--,mPC&0x00ff);
         //				CPU_POKE(0x0100+mSP--,PS());
         //
         //				// Pick up the new PC
         //				mPC=CPU_PEEKW(NMI_VECTOR);
         //			}

         //    fprintf(stderr, "cpu update\n");

         if(mSystem.mSystemIRQ && !mI)
         {
            // IRQ signal clearance is handled by CMikie::Update() as this
            // is the only source of interrupts

            // Push processor status
            PUSH(mPC>>8);
            PUSH(mPC&0xff);
            PUSH(PS()&0xef);		// Clear B flag on stack

            mI=TRUE;				// Stop further interrupts
            mD=FALSE;				// Clear decimal mode

            // Pick up the new PC
            mPC=CPU_PEEKW(IRQ_VECTOR);

            // Save the sleep state as an irq has possibly woken the processor
            mSystem.mSystemCPUSleep_Saved=mSystem.mSystemCPUSleep;
            mSystem.mSystemCPUSleep=FALSE;

            // Log the irq entry time
            mSystem.mIRQEntryCycle=mSystem.mSystemCycleCount;

            // Clear the interrupt status line
            mSystem.mSystemIRQ=FALSE;
         }

         //
         // If the CPU is asleep then skip to the next timer event
         //
         if(mSystem.mSystemCPUSleep) {
            return;
         }

         // Fetch opcode
         mOpcode=CPU_PEEK(mPC);
         mPC++;

         // Execute Opcode

         switch(mOpcode)
         {

            //
            // 0x00
            //
            case 0x00:
               mSystem.mSystemCycleCount+=(1+(6*CPU_RDWR_CYC));
               // IMPLIED
               xBRK();
               break;
            case 0x01:
               mSystem.mSystemCycleCount+=(1+(5*CPU_RDWR_CYC));
               xINDIRECT_X();
               xORA();
               break;
            case 0x02:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0x03:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0x04:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               xZEROPAGE();
               xTSB();
               break;
            case 0x05:
               mSystem.mSystemCycleCount+=(1+(2*CPU_RDWR_CYC));
               xZEROPAGE();
               xORA();
               break;
            case 0x06:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               xZEROPAGE();
               xASL();
               break;
            case 0x07:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;

            case 0x08:
               mSystem.mSystemCycleCount+=(1+(2*CPU_RDWR_CYC));
               // IMPLIED
               xPHP();
               break;
            case 0x09:
               mSystem.mSystemCycleCount+=(1+(2*CPU_RDWR_CYC));
               xIMMEDIATE();
               xORA();
               break;
            case 0x0A:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // IMPLIED
               xASLA();
               break;
            case 0x0B:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0x0C:
               mSystem.mSystemCycleCount+=(1+(5*CPU_RDWR_CYC));
               xABSOLUTE();
               xTSB();
               break;
            case 0x0D:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xABSOLUTE();
               xORA();
               break;
            case 0x0E:
               mSystem.mSystemCycleCount+=(1+(5*CPU_RDWR_CYC));
               xABSOLUTE();
               xASL();
               break;
            case 0x0F:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;

               //
               // 0x10
               //
            case 0x10:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // RELATIVE (IN FUNCTION)
               xBPL();
               break;
            case 0x11:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               xINDIRECT_Y();
               xORA();
               break;
            case 0x12:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               xINDIRECT();
               xORA();
               break;
            case 0x13:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0x14:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               xZEROPAGE();
               xTRB();
               break;
            case 0x15:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xZEROPAGE_X();
               xORA();
               break;
            case 0x16:
               mSystem.mSystemCycleCount+=(1+(5*CPU_RDWR_CYC));
               xZEROPAGE_X();
               xASL();
               break;
            case 0x17:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;

            case 0x18:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // IMPLIED
               xCLC();
               break;
            case 0x19:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xABSOLUTE_Y();
               xORA();
               break;
            case 0x1A:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // IMPLIED
               xINCA();
               break;
            case 0x1B:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0x1C:
               mSystem.mSystemCycleCount+=(1+(5*CPU_RDWR_CYC));
               xABSOLUTE();
               xTRB();
               break;
            case 0x1D:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xABSOLUTE_X();
               xORA();
               break;
            case 0x1E:
               mSystem.mSystemCycleCount+=(1+(6*CPU_RDWR_CYC));
               xABSOLUTE_X();
               xASL();
               break;
            case 0x1F:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;

               //
               // 0x20
               //
            case 0x20:
               mSystem.mSystemCycleCount+=(1+(5*CPU_RDWR_CYC));
               xABSOLUTE();
               xJSR();
               break;
            case 0x21:
               mSystem.mSystemCycleCount+=(1+(5*CPU_RDWR_CYC));
               xINDIRECT_X();
               xAND();
               break;
            case 0x22:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0x23:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0x24:
               mSystem.mSystemCycleCount+=(1+(2*CPU_RDWR_CYC));
               xZEROPAGE();
               xBIT();
               break;
            case 0x25:
               mSystem.mSystemCycleCount+=(1+(2*CPU_RDWR_CYC));
               xZEROPAGE();
               xAND();
               break;
            case 0x26:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               xZEROPAGE();
               xROL();
               break;
            case 0x27:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;

            case 0x28:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               // IMPLIED
               xPLP();
               break;
            case 0x29:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               xIMMEDIATE();
               xAND();
               break;
            case 0x2A:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // IMPLIED
               xROLA();
               break;
            case 0x2B:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0x2C:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xABSOLUTE();
               xBIT();
               break;
            case 0x2D:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xABSOLUTE();
               xAND();
               break;
            case 0x2E:
               mSystem.mSystemCycleCount+=(1+(5*CPU_RDWR_CYC));
               xABSOLUTE();
               xROL();
               break;
            case 0x2F:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;

               //
               // 0x30
               //
            case 0x30:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // RELATIVE (IN FUNCTION)
               xBMI();
               break;
            case 0x31:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               xINDIRECT_Y();
               xAND();
               break;
            case 0x32:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               xINDIRECT();
               xAND();
               break;
            case 0x33:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0x34:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xZEROPAGE_X();
               xBIT();
               break;
            case 0x35:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xZEROPAGE_X();
               xAND();
               break;
            case 0x36:
               mSystem.mSystemCycleCount+=(1+(5*CPU_RDWR_CYC));
               xZEROPAGE_X();
               xROL();
               break;
            case 0x37:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;

            case 0x38:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // IMPLIED
               xSEC();
               break;
            case 0x39:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xABSOLUTE_Y();
               xAND();
               break;
            case 0x3A:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // IMPLIED
               xDECA();
               break;
            case 0x3B:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0x3C:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xABSOLUTE_X();
               xBIT();
               break;
            case 0x3D:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xABSOLUTE_X();
               xAND();
               break;
            case 0x3E:
               mSystem.mSystemCycleCount+=(1+(6*CPU_RDWR_CYC));
               xABSOLUTE_X();
               xROL();
               break;
            case 0x3F:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;

               //
               // 0x40
               //
            case 0x40:
               mSystem.mSystemCycleCount+=(1+(5*CPU_RDWR_CYC));
               // Only clear IRQ if this is not a BRK instruction based RTI

               // B flag is on the stack cant test the flag
               int tmp;
               PULL(tmp);
               PUSH (tmp);
               if(!(tmp&0x10))
               {
                  mSystem.mSystemCPUSleep=mSystem.mSystemCPUSleep_Saved;

                  // If were in sleep mode then we need to push the
                  // wakeup counter along by the same number of cycles
                  // we have used during the sleep period
                  if(mSystem.mSystemCPUSleep)
                  {
                     mSystem.mCPUWakeupTime+=mSystem.mSystemCycleCount-mSystem.mIRQEntryCycle;
                  }
               }
               // IMPLIED
               xRTI();
               break;
            case 0x41:
               mSystem.mSystemCycleCount+=(1+(5*CPU_RDWR_CYC));
               xINDIRECT_X();
               xEOR();
               break;
            case 0x42:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0x43:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0x44:
               mSystem.mSystemCycleCount+=(1+(2*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0x45:
               mSystem.mSystemCycleCount+=(1+(2*CPU_RDWR_CYC));
               xZEROPAGE();
               xEOR();
               break;
            case 0x46:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               xZEROPAGE();
               xLSR();
               break;
            case 0x47:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;

            case 0x48:
               mSystem.mSystemCycleCount+=(1+(2*CPU_RDWR_CYC));
               // IMPLIED
               xPHA();
               break;
            case 0x49:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               xIMMEDIATE();
               xEOR();
               break;
            case 0x4A:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // IMPLIED
               xLSRA();
               break;
            case 0x4B:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0x4C:
               mSystem.mSystemCycleCount+=(1+(2*CPU_RDWR_CYC));
               xABSOLUTE();
               xJMP();
               break;
            case 0x4D:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xABSOLUTE();
               xEOR();
               break;
            case 0x4E:
               mSystem.mSystemCycleCount+=(1+(5*CPU_RDWR_CYC));
               xABSOLUTE();
               xLSR();
               break;
            case 0x4F:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;

               //
               // 0x50
               //
            case 0x50:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // RELATIVE (IN FUNCTION)
               xBVC();
               break;
            case 0x51:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               xINDIRECT_Y();
               xEOR();
               break;
            case 0x52:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               xINDIRECT();
               xEOR();
               break;
            case 0x53:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0x54:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0x55:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xZEROPAGE_X();
               xEOR();
               break;
            case 0x56:
               mSystem.mSystemCycleCount+=(1+(5*CPU_RDWR_CYC));
               xZEROPAGE_X();
               xLSR();
               break;
            case 0x57:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;

            case 0x58:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // IMPLIED
               xCLI();
               break;
            case 0x59:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xABSOLUTE_Y();
               xEOR();
               break;
            case 0x5A:
               mSystem.mSystemCycleCount+=(1+(2*CPU_RDWR_CYC));
               // IMPLIED
               xPHY();
               break;
            case 0x5B:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0x5C:
               mSystem.mSystemCycleCount+=(1+(7*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0x5D:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xABSOLUTE_X();
               xEOR();
               break;
            case 0x5E:
               mSystem.mSystemCycleCount+=(1+(6*CPU_RDWR_CYC));
               xABSOLUTE_X();
               xLSR();
               break;
            case 0x5F:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;

               //
               // 0x60
               //
            case 0x60:
               mSystem.mSystemCycleCount+=(1+(5*CPU_RDWR_CYC));
               // IMPLIED
               xRTS();
               break;
            case 0x61:
               mSystem.mSystemCycleCount+=(1+(5*CPU_RDWR_CYC));
               xINDIRECT_X();
               xADC();
               break;
            case 0x62:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0x63:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0x64:
               mSystem.mSystemCycleCount+=(1+(2*CPU_RDWR_CYC));
               xZEROPAGE();
               xSTZ();
               break;
            case 0x65:
               mSystem.mSystemCycleCount+=(1+(2*CPU_RDWR_CYC));
               xZEROPAGE();
               xADC();
               break;
            case 0x66:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               xZEROPAGE();
               xROR();
               break;
            case 0x67:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;

            case 0x68:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               // IMPLIED
               xPLA();
               break;
            case 0x69:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               xIMMEDIATE();
               xADC();
               break;
            case 0x6A:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // IMPLIED
               xRORA();
               break;
            case 0x6B:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0x6C:
               mSystem.mSystemCycleCount+=(1+(5*CPU_RDWR_CYC));
               xINDIRECT_ABSOLUTE();
               xJMP();
               break;
            case 0x6D:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xABSOLUTE();
               xADC();
               break;
            case 0x6E:
               mSystem.mSystemCycleCount+=(1+(5*CPU_RDWR_CYC));
               xABSOLUTE();
               xROR();
               break;
            case 0x6F:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;

               //
               // 0x70
               //
            case 0x70:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // RELATIVE (IN FUNCTION)
               xBVS();
               break;
            case 0x71:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               xINDIRECT_Y();
               xADC();
               break;
            case 0x72:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               xINDIRECT();
               xADC();
               break;
            case 0x73:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0x74:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xZEROPAGE_X();
               xSTZ();
               break;
            case 0x75:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xZEROPAGE_X();
               xADC();
               break;
            case 0x76:
               mSystem.mSystemCycleCount+=(1+(5*CPU_RDWR_CYC));
               xZEROPAGE_X();
               xROR();
               break;
            case 0x77:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;

            case 0x78:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // IMPLIED
               xSEI();
               break;
            case 0x79:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xABSOLUTE_Y();
               xADC();
               break;
            case 0x7A:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               // IMPLIED
               xPLY();
               break;
            case 0x7B:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0x7C:
               mSystem.mSystemCycleCount+=(1+(5*CPU_RDWR_CYC));
               xINDIRECT_ABSOLUTE_X();
               xJMP();
               break;
            case 0x7D:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xABSOLUTE_X();
               xADC();
               break;
            case 0x7E:
               mSystem.mSystemCycleCount+=(1+(6*CPU_RDWR_CYC));
               xABSOLUTE_X();
               xROR();
               break;
            case 0x7F:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;

               //
               // 0x80
               //
            case 0x80:
               mSystem.mSystemCycleCount+=(1+(2*CPU_RDWR_CYC));
               // RELATIVE (IN FUNCTION)
               xBRA();
               break;
            case 0x81:
               mSystem.mSystemCycleCount+=(1+(5*CPU_RDWR_CYC));
               xINDIRECT_X();
               xSTA();
               break;
            case 0x82:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0x83:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0x84:
               mSystem.mSystemCycleCount+=(1+(2*CPU_RDWR_CYC));
               xZEROPAGE();
               xSTY();
               break;
            case 0x85:
               mSystem.mSystemCycleCount+=(1+(2*CPU_RDWR_CYC));
               xZEROPAGE();
               xSTA();
               break;
            case 0x86:
               mSystem.mSystemCycleCount+=(1+(2*CPU_RDWR_CYC));
               xZEROPAGE();
               xSTX();
               break;
            case 0x87:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;

            case 0x88:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // IMPLIED
               xDEY();
               break;
            case 0x89:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               xIMMEDIATE();
               xBIT();
               break;
            case 0x8A:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // IMPLIED
               xTXA();
               break;
            case 0x8B:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0x8C:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xABSOLUTE();
               xSTY();
               break;
            case 0x8D:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xABSOLUTE();
               xSTA();
               break;
            case 0x8E:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xABSOLUTE();
               xSTX();
               break;
            case 0x8F:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;

               //
               // 0x90
               //
            case 0x90:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // RELATIVE (IN FUNCTION)
               xBCC();
               break;
            case 0x91:
               mSystem.mSystemCycleCount+=(1+(5*CPU_RDWR_CYC));
               xINDIRECT_Y();
               xSTA();
               break;
            case 0x92:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               xINDIRECT();
               xSTA();
               break;
            case 0x93:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0x94:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xZEROPAGE_X();
               xSTY();
               break;
            case 0x95:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xZEROPAGE_X();
               xSTA();
               break;
            case 0x96:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xZEROPAGE_Y();
               xSTX();
               break;
            case 0x97:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;

            case 0x98:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // IMPLIED
               xTYA();
               break;
            case 0x99:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               xABSOLUTE_Y();
               xSTA();
               break;
            case 0x9A:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // IMPLIED
               xTXS();
               break;
            case 0x9B:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0x9C:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xABSOLUTE();
               xSTZ();
               break;
            case 0x9D:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               xABSOLUTE_X();
               xSTA();
               break;
            case 0x9E:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               xABSOLUTE_X();
               xSTZ();
               break;
            case 0x9F:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;

               //
               // 0xA0
               //
            case 0xA0:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               xIMMEDIATE();
               xLDY();
               break;
            case 0xA1:
               mSystem.mSystemCycleCount+=(1+(5*CPU_RDWR_CYC));
               xINDIRECT_X();
               xLDA();
               break;
            case 0xA2:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               xIMMEDIATE();
               xLDX();
               break;
            case 0xA3:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0xA4:
               mSystem.mSystemCycleCount+=(1+(2*CPU_RDWR_CYC));
               xZEROPAGE();
               xLDY();
               break;
            case 0xA5:
               mSystem.mSystemCycleCount+=(1+(2*CPU_RDWR_CYC));
               xZEROPAGE();
               xLDA();
               break;
            case 0xA6:
               mSystem.mSystemCycleCount+=(1+(2*CPU_RDWR_CYC));
               xZEROPAGE();
               xLDX();
               break;
            case 0xA7:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;

            case 0xA8:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // IMPLIED
               xTAY();
               break;
            case 0xA9:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               xIMMEDIATE();
               xLDA();
               break;
            case 0xAA:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // IMPLIED
               xTAX();
               break;
            case 0xAB:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0xAC:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xABSOLUTE();
               xLDY();
               break;
            case 0xAD:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xABSOLUTE();
               xLDA();
               break;
            case 0xAE:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xABSOLUTE();
               xLDX();
               break;
            case 0xAF:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;

               //
               // 0xB0
               //
            case 0xB0:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // RELATIVE (IN FUNCTION)
               xBCS();
               break;
            case 0xB1:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               xINDIRECT_Y();
               xLDA();
               break;
            case 0xB2:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               xINDIRECT();
               xLDA();
               break;
            case 0xB3:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0xB4:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xZEROPAGE_X();
               xLDY();
               break;
            case 0xB5:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xZEROPAGE_X();
               xLDA();
               break;
            case 0xB6:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xZEROPAGE_Y();
               xLDX();
               break;
            case 0xB7:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;

            case 0xB8:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // IMPLIED
               xCLV();
               break;
            case 0xB9:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xABSOLUTE_Y();
               xLDA();
               break;
            case 0xBA:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // IMPLIED
               xTSX();
               break;
            case 0xBB:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0xBC:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xABSOLUTE_X();
               xLDY();
               break;
            case 0xBD:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xABSOLUTE_X();
               xLDA();
               break;
            case 0xBE:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xABSOLUTE_Y();
               xLDX();
               break;
            case 0xBF:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;

               //
               // 0xC0
               //
            case 0xC0:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               xIMMEDIATE();
               xCPY();
               break;
            case 0xC1:
               mSystem.mSystemCycleCount+=(1+(5*CPU_RDWR_CYC));
               xINDIRECT_X();
               xCMP();
               break;
            case 0xC2:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0xC3:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0xC4:
               mSystem.mSystemCycleCount+=(1+(2*CPU_RDWR_CYC));
               xZEROPAGE();
               xCPY();
               break;
            case 0xC5:
               mSystem.mSystemCycleCount+=(1+(2*CPU_RDWR_CYC));
               xZEROPAGE();
               xCMP();
               break;
            case 0xC6:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               xZEROPAGE();
               xDEC();
               break;
            case 0xC7:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;

            case 0xC8:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // IMPLIED
               xINY();
               break;
            case 0xC9:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               xIMMEDIATE();
               xCMP();
               break;
            case 0xCA:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // IMPLIED
               xDEX();
               break;
            case 0xCB:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // IMPLIED
               xWAI();
               break;
            case 0xCC:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xABSOLUTE();
               xCPY();
               break;
            case 0xCD:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xABSOLUTE();
               xCMP();
               break;
            case 0xCE:
               mSystem.mSystemCycleCount+=(1+(5*CPU_RDWR_CYC));
               xABSOLUTE();
               xDEC();
               break;
            case 0xCF:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;

               //
               // 0xD0
               //
            case 0xD0:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // RELATIVE (IN FUNCTION)
               xBNE();
               break;
            case 0xD1:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               xINDIRECT_Y();
               xCMP();
               break;
            case 0xD2:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               xINDIRECT();
               xCMP();
               break;
            case 0xD3:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0xD4:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0xD5:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xZEROPAGE_X();
               xCMP();
               break;
            case 0xD6:
               mSystem.mSystemCycleCount+=(1+(5*CPU_RDWR_CYC));
               xZEROPAGE_X();
               xDEC();
               break;
            case 0xD7:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;

            case 0xD8:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // IMPLIED
               xCLD();
               break;
            case 0xD9:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xABSOLUTE_Y();
               xCMP();
               break;
            case 0xDA:
               mSystem.mSystemCycleCount+=(1+(2*CPU_RDWR_CYC));
               // IMPLIED
               xPHX();
               break;
            case 0xDB:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // IMPLIED
               xSTP();
               break;
            case 0xDC:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0xDD:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xABSOLUTE_X();
               xCMP();
               break;
            case 0xDE:
               mSystem.mSystemCycleCount+=(1+(6*CPU_RDWR_CYC));
               xABSOLUTE_X();
               xDEC();
               break;
            case 0xDF:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;

               //
               // 0xE0
               //
            case 0xE0:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               xIMMEDIATE();
               xCPX();
               break;
            case 0xE1:
               mSystem.mSystemCycleCount+=(1+(5*CPU_RDWR_CYC));
               xINDIRECT_X();
               xSBC();
               break;
            case 0xE2:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0xE3:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0xE4:
               mSystem.mSystemCycleCount+=(1+(2*CPU_RDWR_CYC));
               xZEROPAGE();
               xCPX();
               break;
            case 0xE5:
               mSystem.mSystemCycleCount+=(1+(2*CPU_RDWR_CYC));
               xZEROPAGE();
               xSBC();
               break;
            case 0xE6:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               xZEROPAGE();
               xINC();
               break;
            case 0xE7:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;

            case 0xE8:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // IMPLIED
               xINX();
               break;
            case 0xE9:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               xIMMEDIATE();
               xSBC();
               break;
            case 0xEA:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // IMPLIED
               xNOP();
               break;
            case 0xEB:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0xEC:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xABSOLUTE();
               xCPX();
               break;
            case 0xED:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xABSOLUTE();
               xSBC();
               break;
            case 0xEE:
               mSystem.mSystemCycleCount+=(1+(5*CPU_RDWR_CYC));
               xABSOLUTE();
               xINC();
               break;
            case 0xEF:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;

               //
               // 0xF0
               //
            case 0xF0:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // RELATIVE (IN FUNCTION)
               xBEQ();
               break;
            case 0xF1:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               xINDIRECT_Y();
               xSBC();
               break;
            case 0xF2:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               xINDIRECT();
               xSBC();
               break;
            case 0xF3:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0xF4:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0xF5:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xZEROPAGE_X();
               xSBC();
               break;
            case 0xF6:
               mSystem.mSystemCycleCount+=(1+(5*CPU_RDWR_CYC));
               xZEROPAGE_X();
               xINC();
               break;
            case 0xF7:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;

            case 0xF8:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // IMPLIED
               xSED();
               break;
            case 0xF9:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xABSOLUTE_Y();
               xSBC();
               break;
            case 0xFA:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               // IMPLIED
               xPLX();
               break;
            case 0xFB:
               mSystem.mSystemCycleCount+=(1+(1*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0xFC:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
            case 0xFD:
               mSystem.mSystemCycleCount+=(1+(3*CPU_RDWR_CYC));
               xABSOLUTE_X();
               xSBC();
               break;
            case 0xFE:
               mSystem.mSystemCycleCount+=(1+(6*CPU_RDWR_CYC));
               xABSOLUTE_X();
               xINC();
               break;
            case 0xFF:
               mSystem.mSystemCycleCount+=(1+(4*CPU_RDWR_CYC));
               // *** ILLEGAL ***
               xILLEGAL();
               break;
         }
      }

      //		inline void SetBreakpoint(ULONG breakpoint) {mPcBreakpoint=breakpoint;};

      void C65C02::SetRegs(C6502_REGS &regs)
      {
         PS(regs.PS);
         mA=regs.A;
         mX=regs.X;
         mY=regs.Y;
         mSP=regs.SP;
         mOpcode=regs.Opcode;
         mOperand=regs.Operand;
         mPC=regs.PC;
         mSystem.mSystemCPUSleep=regs.WAIT;
         mSystem.mSystemNMI=regs.NMI;
         mSystem.mSystemIRQ=regs.IRQ;
      }

      void C65C02::GetRegs(C6502_REGS &regs)
      {
         regs.PS=PS();
         regs.A=mA;
         regs.X=mX;
         regs.Y=mY;
         regs.SP=mSP;
         regs.Opcode=mOpcode;
         regs.Operand=mOperand;
         regs.PC=mPC;
         regs.WAIT=(mSystem.mSystemCPUSleep)?true:false;
         regs.NMI=(mSystem.mSystemNMI)?true:false;
         regs.IRQ=(mSystem.mSystemIRQ)?true:false;
      }


