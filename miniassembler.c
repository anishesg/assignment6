/*--------------------------------------------------------------------*/
/* miniassembler.c                                                    */
/* Author: Bob Dondero, Donna Gabai, Anish K                          */
/*--------------------------------------------------------------------*/

#include "miniassembler.h"
#include <assert.h>
#include <stddef.h>

/*--------------------------------------------------------------------*/
/* Modify *puiDest in place,
   setting uiNumBits starting at uiDestStartBit (where 0 indicates
   the least significant bit) with bits taken from uiSrc,
   starting at uiSrcStartBit.
   uiSrcStartBit indicates the rightmost bit in the field.
   setField sets the appropriate bits in *puiDest to 1.
   setField never unsets any bits in *puiDest.                        */
static void setField(unsigned int uiSrc, unsigned int uiSrcStartBit,
                     unsigned int *puiDest, unsigned int uiDestStartBit,
                     unsigned int uiNumBits)
{
   for (unsigned int bitIndex = 0; bitIndex < uiNumBits; bitIndex++) {
      unsigned int uiSrcBitPos = uiSrcStartBit + bitIndex;
      unsigned int uiDestBitPos = uiDestStartBit + bitIndex;

      /* if the corresponding bit in uiSrc is set, reflect it in *puiDest */
      if ((uiSrc >> uiSrcBitPos) & 1U) {
         *puiDest |= (1U << uiDestBitPos);
      }
   }
}

/*--------------------------------------------------------------------*/
/* Generate machine code for "mov wReg, #iImmediate".
   uiReg is the W register number (0-31).
   iImmediate is the immediate value to load into the register.

   Base opcode: 0x52800000 for MOV.
   Bits [0:4] = destination register (uiReg)
   Bits [5:20] = immediate value (iImmediate)                         */
unsigned int MiniAssembler_mov(unsigned int uiReg, int iImmediate)
{
   unsigned int uiInstr = 0x52800000; /* Base MOV opcode */

   /* Insert the register into [0:4] */
   setField(uiReg, 0, &uiInstr, 0, 5);

   /* Insert the 16-bit immediate into [5:20] */
   setField((unsigned int)iImmediate, 0, &uiInstr, 5, 16);

   return uiInstr;
}

unsigned int MiniAssembler_adr(unsigned int uiReg, unsigned long ulAddr,
   unsigned long ulAddrOfThisInstr)
{
   unsigned int uiInstr;
   unsigned int uiDisp;

   /* Base Instruction Code */
   uiInstr = 0x10000000;

   /* register to be inserted in instruction */
   setField(uiReg, 0, &uiInstr, 0, 5);

   /* displacement to be split into immlo and immhi and inserted */
   uiDisp = (unsigned int)(ulAddr - ulAddrOfThisInstr);

   setField(uiDisp, 0, &uiInstr, 29, 2);
   setField(uiDisp, 2, &uiInstr, 5, 19);

   return uiInstr;
}

/*--------------------------------------------------------------------*/
/* Generate machine code for "strb wFromReg, [xToReg]".
   uiFromReg: W register number with the byte to store.
   uiToReg: X register number containing the address.

   Base opcode: 0x39000000 for STRB.
   Bits [0:4] = wFromReg
   Bits [5:9] = xToReg                                                 */
unsigned int MiniAssembler_strb(unsigned int uiFromReg, unsigned int uiToReg)
{
   unsigned int uiInstr = 0x39000000; /* Base STRB opcode */

   /* Insert wFromReg in [0:4] */
   setField(uiFromReg, 0, &uiInstr, 0, 5);

   /* Insert xToReg in [5:9] */
   setField(uiToReg, 0, &uiInstr, 5, 5);

   return uiInstr;
}

/*--------------------------------------------------------------------*/
/* Generate machine code for "b addr".
   ulAddr: branch target address.
   ulAddrOfThisInstr: address of this B instruction.

   B uses a 26-bit signed offset representing (branch_addr - instr_addr)/4.
   Base opcode: 0x14000000 for B.
   Bits [0:25] = 26-bit offset                                         */
unsigned int MiniAssembler_b(unsigned long ulAddr,
   unsigned long ulAddrOfThisInstr)
{
   unsigned int uiInstr = 0x14000000; /* Base B opcode */
   unsigned int uiBranchOffset = (unsigned int)((ulAddr - ulAddrOfThisInstr) >> 2);

   /* Insert the 26-bit offset into [0:25] */
   setField(uiBranchOffset, 0, &uiInstr, 0, 26);

   return uiInstr;
}