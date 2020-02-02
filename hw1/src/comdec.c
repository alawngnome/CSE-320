#include "const.h"
#include "sequitur.h"
#include "debug.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

/*
 * You may modify this file and/or move the functions contained here
 * to other source files (except for main.c) as you wish.
 *
 * IMPORTANT: You MAY NOT use any array brackets (i.e. [ and ]) and
 * you MAY NOT declare any arrays or allocate any storage with malloc().
 * The purpose of this restriction is to force you to use pointers.
 * Variables to hold the pathname of the current file or directory
 * as well as other data have been pre-declared for you in const.h.
 * You must use those variables, rather than declaring your own.
 * IF YOU VIOLATE THIS RESTRICTION, YOU WILL GET A ZERO!
 *
 * IMPORTANT: You MAY NOT use floating point arithmetic or declare
 * any "float" or "double" variables.  IF YOU VIOLATE THIS RESTRICTION,
 * YOU WILL GET A ZERO!
 */

/**
 * Main compression function.
 * Reads a sequence of bytes from a specified input stream, segments the
 * input data into blocks of a specified maximum number of bytes,
 * uses the Sequitur algorithm to compress each block of input to a list
 * of rules, and outputs the resulting compressed data transmission to a
 * specified output stream in the format detailed in the header files and
 * assignment handout.  The output stream is flushed once the transmission
 * is complete.
 *
 * The maximum number of bytes of uncompressed data represented by each
 * block of the compressed transmission is limited to the specified value
 * "bsize".  Each compressed block except for the last one represents exactly
 * "bsize" bytes of uncompressed data and the last compressed block represents
 * at most "bsize" bytes.
 *
 * @param in  The stream from which input is to be read.
 * @param out  The stream to which the block is to be written.
 * @param bsize  The maximum number of bytes read per block.
 * @return  The number of bytes written, in case of success,
 * otherwise EOF.
 */
int compress(FILE *in, FILE *out, int bsize) {
    // To be implemented.
    return EOF;
}

/**
 * Main decompression function.
 * Reads a compressed data transmission from an input stream, expands it,
 * and and writes the resulting decompressed data to an output stream.
 * The output stream is flushed once writing is complete.
 *
 * @param in  The stream from which the compressed block is to be read.
 * @param out  The stream to which the uncompressed data is to be written.
 * @return  The number of bytes written, in case of success, otherwise EOF.
 */
int decompress(FILE *in, FILE *out) {
    // To be implemented.
    return EOF;
}
/**Helper Function - calculates length of a string
**/
int strLen(char *str) //calculates the length of a function
{
    int i = 0;
    while(*str != '\0'){
        i++;
        str++;
    }
    return i;
}
/**Helper Function - calculates string equality
**/
int strEq(char *str1, char *str2) {
    if(strLen(str1) != strLen(str2)){
        return 1; //return false if lengths are not the same
    }
    for(int i=0; i<strLen(str1); i++){
        if(*str1 != *str2){
            return 1; //return false if a character in the string does not match
        }
        str1++;
        str2++;
    }
    return 0;
}
/**Helper Function - turns string number to int
**/
int strToInt(char *str) {
    int total = 0;
    for(int i = 0; i <= strLen(str); i++) {
        if('0' > *str || *str > '9') {
            return -1; //-1 is our error code for invalid input
        }
        total += *str - 48;
        total *= 10;
        str++;
    }
    total += *str - 48;
    return total;
}


/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the selected program options will be set in the
 * global variable "global_options", where they will be accessible
 * elsewhere in the program.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * Refer to the homework document for the effects of this function on
 * global variables.
 * @modifies global variable "global_options" to contain a bitmap representing
 * the selected options.
 */
int validargs(int argc, char **argv)
{
    //printf("argv[0] is %s\n", argv[0]); TESTING ONLY
    // To be implemented.
    if(argc <= 1)
        return EXIT_FAILURE; //EXIT_FAILURE if no flags are provided
    char *firstArg = *(argv + 1);
    //checking the first character
    if(strEq(firstArg, "-h") == 0) {
        return EXIT_SUCCESS; //EXIT_SUCCESS if the first argument is -h
    } //if -h is not the first character, it should not exist
    char *secondArg = *(argv + 2);
    if(strEq(firstArg, "-c") == 0) {
        if(strEq(secondArg, "-d") == 0)
            return EXIT_FAILURE; //EXIT_FAILURE if -c -d flags
        else if(strEq(secondArg, "-b") == 0) { //checking that the second flag is -b
            char *thirdArg = *(argv + 3);
            if(*thirdArg == '\0') {
                return EXIT_FAILURE; //EXIT_FAILURE if BLOCKSIZE does not exist following -b
            }
            else if(strToInt(thirdArg) == -1) {
                return EXIT_FAILURE; //EXIT_FAILURE if BLOCKSIZE is not valid
            }
            //DO GLOBAL OPTIONS THING HERE
        }
    }
    else if(strEq(firstArg, "-d") == 0) {
        if(*secondArg != '\0') {
            return EXIT_FAILURE; //EXIT_FAILURE if a second flag after -d exists
        }
    }
    return -1;
}
