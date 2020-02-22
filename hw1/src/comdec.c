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

/** Helper Function for outputting a single character at a time
    Returns number of bytes written to fputc in the single character
**/
int utfOutput(int codePoint, FILE *out) {
    if(0x0 <= codePoint && codePoint <= 0x7f){ //one byte
        fputc(codePoint&0x7f, out);
        return 1;
    }
    else if(0x80 <= codePoint && codePoint <= 0x7ff){ //two bytes
        int output = (codePoint>>6)&0x1f;
        fputc(output|0xC0, out);

        output = codePoint&0x3f;
        fputc(output|0x80, out);

        return 2;
    }
    else if(0x800 <= codePoint && codePoint <= 0xffff){ //three bytes
        int output = (codePoint>>12)&0xf;
        fputc(output|0xE0, out);

        output = (codePoint>>6)&0x3f;
        fputc(output|0x80, out);

        output = codePoint&0x3f;
        fputc(output|0x80, out);

        return 3;
    }
    else if(0x10000 <= codePoint && codePoint <= 0x10ffff){ //four bytes
        int output = (codePoint>>18)&0x7;
        fputc(output|0xF0, out);

        output = (codePoint>>12)&0x3f;
        fputc(output|0x80, out);

        output = (codePoint>>6)&0x3f;
        fputc(output|0x80, out);

        output = codePoint&0x3f;
        fputc(output|0x80, out);

        return 4;
    }else {
        return 0;
    }
}


/** Helper Function for outputting blocks with markers 0x83, 0x84, and 0x85
    Returns amount of bytes processed;
**/
int compressOutput(FILE *out) {
    int blockByteCount = 0;

    fputc(0x83, out); //start of block 0x83
    blockByteCount++;

    SYMBOL *rulePointer = main_rule;

    debug("*****main_rule->value is %x\n", main_rule->value);
    debug("*****main_rule->nextr->rule->value is %x\n", main_rule->nextr->rule->value);

    while(1) {
        SYMBOL* s = rulePointer;

        while(1) {
            blockByteCount += utfOutput(s->value, out);
            s = s->next;

            if (s == rulePointer)
                break;
        }
        rulePointer = rulePointer->nextr;

        if (rulePointer == main_rule)
            break;

        fputc(0x85, out); //end of rule, insert rule delimeter
        blockByteCount++;
    }

    fputc(0x84, out); //end of block
    blockByteCount++;

    return blockByteCount;

}
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
    if(in == NULL || out == NULL)
        return EOF;

    fputc(0x81, out); //start transmission with 0x81

    int byteCount= 1;
    int rulePointer = 0; //begins with first character of in

    while(rulePointer != EOF){
        //at the start of every block loop, reinitialize symbols, rules, and digram_hash
        init_symbols();
        init_rules();
        init_digram_hash();

        SYMBOL *ruleAdd = new_rule(next_nonterminal_value);
        add_rule(ruleAdd);
        next_nonterminal_value++;

        int blockCounter = 0;

        int kilo = 0;
        kilo = bsize; // Added
        while(blockCounter < kilo) {//while input still remains
            debug("blockCounter: %d", blockCounter);
            rulePointer = fgetc(in);

            if (rulePointer == EOF)
                break;

            debug("main_rule->prev is %lu", SYMBOL_INDEX(main_rule->prev));

            SYMBOL *new_terminal = new_symbol(rulePointer, NULL);
            debug("main_rule->prev is %lu", SYMBOL_INDEX(main_rule->prev));
            insert_after(main_rule->prev, new_terminal); //append to end

            check_digram(new_terminal->prev); //check second to last symbol
            debug("main_rule->prev is %lu", SYMBOL_INDEX(main_rule->prev));
            debug("wackeroni");

            blockCounter++;
        }
        debug("huh.");


        if(rulePointer != EOF && blockCounter == kilo) { //if whole block filled up but other blocks remaining
            blockCounter = 0; //reset block counter

            int byteToAdd = compressOutput(out);
            if(byteToAdd < 0)
                return EOF;
            byteCount += byteToAdd;
        }
        else if(rulePointer == EOF && blockCounter < kilo){ //if incomplete block left when EOF reached
            debug("Incomplete block found\n");
            int byteToAdd = compressOutput(out);

            if(byteToAdd < 0)
                return EOF;
            byteCount += byteToAdd;
        }
    }
    fputc(0x82, out);
    byteCount += 1;

    fflush(out); // Added
    //return byteCount;
    debug("byteCount is %d", byteCount);
    return byteCount;
}


//Testing function that prints out the rule map
void printList(SYMBOL *rulePointer){
    SYMBOL *main_ruleCopy = rulePointer;

    debug("*****rulePointer->rule->value is %x\n", rulePointer->rule->value);
    //debug("rulePointer->nextr->value is %x\n", rulePointer->nextr->value);
    while(rulePointer->nextr->rule != main_ruleCopy) {

        rulePointer = rulePointer-> next;
        //debug("rulePointer's second value is %x\n", rulePointer->value);
        while(rulePointer->rule != rulePointer) {

            debug("The value of this symbol is %x\n", rulePointer->value);
            rulePointer = rulePointer->next;
        }
        rulePointer = rulePointer->nextr;
        debug("*****rulePointer->rule->value is %x\n", rulePointer->rule->value);
    }
}


/**Helper Function: Expands a rule set in a block into the uncompressed string and returns total bytes processed
**/
int ruleExpand(SYMBOL *rulePointer, FILE *out){;
    if(rulePointer == NULL){
        return EOF;
    }

    int byteCount = 0;

    rulePointer = rulePointer->next; //skip head character
    //debug("rulePointer->value is %c\n", rulePointer->value);
    while(rulePointer->rule != rulePointer){
        if(IS_TERMINAL(rulePointer)){ //terminal symbol
            fputc(rulePointer->value, out);
            byteCount++;
        }else { //non-terminal symbol
            byteCount += ruleExpand(*(rule_map+(rulePointer->value)), out);
            //printf("non-terminal symbol in ruleExpand\n");
        }
        rulePointer = rulePointer->next;
    }

    return byteCount;
}

/** Helper Function that parses a single character
    Only use in decompressBlock()

    READS THE CHARACTER RIGHT AFTER THE POINTER
**/
int decompressChar(FILE *in) {
    int utfByte = fgetc(in);

    //if 0x85, 0x84... return unmodified char
    if(utfByte == 0x85 || utfByte == 0x84){
        return utfByte;
    }

    int codePoint;
    if(utfByte>>7 == 0){ //1 byte
        codePoint = utfByte & 127;
    }
    else if(utfByte>>5 == 6){ //2 bytes
        codePoint = utfByte&31; //the final codepoint in UTF-8 we want to represent
        codePoint <<= 6; //Right shifting 6 to make room
        int secondHeaderByte = fgetc(in);
        codePoint |= secondHeaderByte&63;
    }
    else if(utfByte>>4 == 14){ //3 bytes
        codePoint = utfByte&15;
        codePoint <<= 6;
        int secondHeaderByte = fgetc(in);
        codePoint |= secondHeaderByte&63;

        secondHeaderByte = fgetc(in);
        codePoint <<= 6;
        codePoint |= secondHeaderByte&63;
    }
    else if(utfByte>>3 == 30){ //4 bytes
        codePoint = utfByte&7;
        int secondHeaderByte = fgetc(in);
        codePoint <<= 6;
        codePoint |= secondHeaderByte&63;

        secondHeaderByte = fgetc(in);
        codePoint <<= 6;
        codePoint |= secondHeaderByte&63;

        secondHeaderByte = fgetc(in);
        codePoint <<= 6;
        codePoint |= secondHeaderByte&63;
    }else {
        return EOF; //EOF if not in proper UTF-8
    }
    return codePoint;
}

/** Helper Function that parses a single block
    READS STARTING RIGHT AFTER THE POINTER
    CALL AFTER 0X83 IS READ
    ENDS HAVING READ 0X84 CHARACTER
**/
SYMBOL *decompressBlock(FILE *in) {
    init_symbols();
    init_rules();

    SYMBOL *brand_new_rule; //To be returned

    int utfByte = decompressChar(in); //utfByte will signify whether block is reading main_rule or child_rule

    if(utfByte < 256 || utfByte == EOF){
        debug("Byte value: %x", utfByte);
        debug("Terminal as first symbol");
        return NULL;
    } else {
        brand_new_rule = new_rule(utfByte);
        add_rule(brand_new_rule);
        *(rule_map + brand_new_rule->value) = brand_new_rule;
    }

    int ruleDelimiterSkip = 0; //one iteration skip over creating a new symbol

    utfByte = decompressChar(in);
    while(utfByte != 0x84) { //definite stop when we reach end-of-block

        //debug("utfByte is %x\n", utfByte);
        if(utfByte == EOF)
            return NULL;
        else if(utfByte == 0x85) { //skip current character(0x85) and skip adding symbol
            //create head of child rule
            utfByte = decompressChar(in); //fuck im stupid

        if(utfByte < 256 || utfByte == EOF) {
            debug("Byte value: %x", utfByte);
            debug("Terminal as first symbol");
            return NULL;
        } else {
            brand_new_rule = new_rule(utfByte);
            add_rule(brand_new_rule);
            *(rule_map + brand_new_rule->value) = brand_new_rule;
        }

            ruleDelimiterSkip = 1;
        }
        if(ruleDelimiterSkip == 0) {

            SYMBOL *symbolAdd = new_symbol(utfByte, brand_new_rule);
            brand_new_rule->prev->next = symbolAdd;
            symbolAdd->next = brand_new_rule;
            symbolAdd->prev = brand_new_rule->prev;
            brand_new_rule->prev = symbolAdd;

        } else {
            ruleDelimiterSkip = 0;
        }
        utfByte = decompressChar(in);
    } //end of while loop
    return brand_new_rule;
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
    if(in == NULL || out == NULL){
        return EOF;
    }

    if(fgetc(in) != 0x81){
        return EOF;
    }

    int utfByte = fgetc(in); //should be initialized to be 0x83 at first

    if(utfByte != 0x83) {
        return EOF;
    }

    debug("got here");

    int byteCount = 0;
    while(utfByte != 0x82) {
        SYMBOL *ruleBlock = decompressBlock(in);

        if(ruleBlock == NULL){
            debug("Invalid block");
            return EOF;
        }

        byteCount += ruleExpand(main_rule, out);
        //printList(main_rule);

        utfByte = fgetc(in);
    }
    if(utfByte != 0x82) {
        return EOF;
    }
    //0x82 should be read to end loop
    debug("decompress byteCount is %d", byteCount);
    fflush(out);
    return byteCount;
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
    //printf("str1 is %c%c\n", *str1, *(str1+1));
    //printf("str2 is %c%c\n", *str2, *(str2+1));
    if(strLen(str1) != strLen(str2)){
        //printf("The lengths were not the same\n\n");
        return 1; //return false if lengths are not the same
    }
    int length = strLen(str1);
    for(int i=0; i<length; i++){
        if(*str1 != *str2){
            //printf("there was a mismatch: %d %d\n\n", *str1, *str2);
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
    //printf("strLen returned %d\n", strLen(str));
    /*while('0' < *str && *str < '9'){
        total *= 10;
        total += *str - '0';
        str++;
    }*/
    int length = strLen(str);
    for(int i = 0; i < length; i++) {
        if('0' > *str || *str > '9') {
            //printf("The str is %d\n", *str);
            return -1; //-1 is our error code for invalid input
        }
        total = total * 10 + (*str - '0');
        str++;
        //printf("i = %d: %d\n", i, total);
    }
    //printf("Total: %d", total);
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
        return -1; //EXIT_FAILURE if no flags are provided
    char *firstArg = *(argv + 1);
    //printf("%d and %d",*firstArg, *(firstArg+1));
    //checking the first character
    if(strEq(firstArg, "-h") == 0) {
        global_options = 1 | global_options; //if -h flag, set LSB to 1
        return 0; //EXIT_SUCCESS if the first argument is -h
    } //if -h is not the first character, it should not exist
    if(argc > 4)
        return -1; //if no -h flag, maximum 4 arguments
    if(strEq(firstArg, "-c") == 0) {
        if(argc == 2) {
            int extendedBlockSize = 0x0400<<16; //default block_size
            global_options = extendedBlockSize | global_options;//BLOCKSIZE in global_options if -b flag
            return 0; //EXIT_SUCCESS if only -c flag exists
        }
        char *secondArg = *(argv + 2); //otherwise test the second argument
        if(strEq(secondArg, "-d") == 0 || strEq(secondArg, "-h") == 0 || strEq(secondArg, "-c") == 0)
            return -1; //EXIT_FAILURE if -c -d or -c -h flags);
        else if(strEq(secondArg, "-b") == 0) { //checking that the second flag is -b
            if(argc != 4) {
                //printf("The number of args is: %d\n", argc);
                return -1; //EXIT FAILURE if not in format sequitur -c -b BLOCKSIZE
            }
            char *thirdArg = *(argv + 3);
            if(strToInt(thirdArg) == -1){
                printf("BLOCKSIZE was not valid\n");
                return -1; //EXIT_FAILURE if BLOCKSIZE is not valid
            }
            else if(1 > strToInt(thirdArg) || strToInt(thirdArg) > 1024) {
                //printf("the failed number was %d\n", strToInt(thirdArg));
                return -1; //EXIT_FAILURE if BLOCKSIZE is not valid
            }
            //printf("the successful number was %d\n", strToInt(thirdArg));
            int extendedBlockSize = strToInt(thirdArg)<<16;
            global_options = extendedBlockSize | global_options;//BLOCKSIZE in global_options if -b flag
        }
        global_options = 2 | global_options; //if -c flag, set second LSB to 1
    }
    else if(strEq(firstArg, "-d") == 0) {
        if(argc > 2) {
            return -1; //EXIT_FAILURE if a second flag after -d exists
        }
        global_options = 4 | global_options; //if -d flag, set third LSB to 1
    }
    else if(strEq(firstArg, "-b") == 0) {
        return -1; //EXIT_FAILURE if the first flag is -b
    }
    return 0;
}


