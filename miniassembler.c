#include "miniassembler.h"
#include <assert.h>
#include <stddef.h>

static void setField(unsigned int uiSrc, unsigned int uiSrcStartBit,
                    unsigned int *puiDest, unsigned int uiDestStartBit,
                    unsigned int uiNumBits)
{
    unsigned int bitIndex;
    for (bitIndex = 0; bitIndex < uiNumBits; bitIndex++) {
        unsigned int uiSrcBitPos = uiSrcStartBit + bitIndex;
        unsigned int uiDestBitPos = uiDestStartBit + bitIndex;
        if ((uiSrc >> uiSrcBitPos) & 1U) {
            *puiDest |= (1U << uiDestBitPos);
        }
    }
}

unsigned int MiniAssembler_mov(unsigned int uiReg, int iImmediate)
{
    unsigned int uiInstr = 0x52800000; /* Base MOV opcode */
    setField(uiReg, 0, &uiInstr, 0, 5);
    setField((unsigned int)iImmediate, 0, &uiInstr, 5, 16);
    return uiInstr;
}

unsigned int MiniAssembler_adr(unsigned int uiReg, unsigned long ulAddr,
                                unsigned long ulAddrOfThisInstr)
{
    unsigned int uiInstr = 0x10000000; /* Base ADR opcode */
    unsigned int uiDisp = (unsigned int)(ulAddr - ulAddrOfThisInstr);
    setField(uiReg, 0, &uiInstr, 0, 5);
    setField(uiDisp, 0, &uiInstr, 29, 2);
    setField(uiDisp, 2, &uiInstr, 5, 19);
    return uiInstr;
}

unsigned int MiniAssembler_strb(unsigned int uiFromReg, unsigned int uiToReg)
{
    unsigned int uiInstr = 0x39000000; /* Base STRB opcode */
    setField(uiFromReg, 0, &uiInstr, 0, 5);
    setField(uiToReg, 0, &uiInstr, 5, 5);
    return uiInstr;
}

unsigned int MiniAssembler_b(unsigned long ulAddr, unsigned long ulAddrOfThisInstr)
{
    unsigned int uiInstr = 0x14000000; /* Base B opcode */
    unsigned int uiBranchOffset = (unsigned int)((ulAddr - ulAddrOfThisInstr) >> 2);
    setField(uiBranchOffset, 0, &uiInstr, 0, 26);
    return uiInstr;
}