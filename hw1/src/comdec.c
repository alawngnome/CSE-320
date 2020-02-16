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

/**Helper Function: Expands a rule set in a block into the uncompressed string and returns total bytes processed
**/
int ruleExpand(SYMBOL *rulePointer, FILE *out){
    //printf("ruleExpand called\n");
    if(rulePointer == NULL){
        return EOF;
    }

    int byteCount = 0;
    SYMBOL *rule_head = rulePointer;

    do{
        rulePointer = rulePointer->next; //increment through the body of a rule
        byteCount++; //counter for decompress return value
        if(IS_TERMINAL(rulePointer)){ //terminal symbol
            fputc(rulePointer->value, out);
        }else { //non-terminal symbol
            byteCount += ruleExpand(*(rule_map+(rulePointer->value)), out);
            //printf("non-terminal symbol in ruleExpand\n");
        }
        //printf("rulePointer's value is %x\n", rulePointer->value);
    }while(rulePointer != rule_head);
    //printf("byteCount returns %d\n", byteCount);
    return byteCount;
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
    //printf("decompress runs\n");
    //printf("The first fgetc(in) is %x\n",fgetc(in));
    if(fgetc(in) != 0x81){ //Start of transmission
        printf("Failing at 0x81 if case\n");
        return EOF; //Failure if not start of transmission
    }
    int utfByte = fgetc(in); //first initialized to be 0x83
    //printf("utfByte before looping is %x\n", utfByte);
    //printf("The one that follows is %x\n", fgetc(in));
    int byteTotal = 0;

    //SYMBOL *main_rule = NULL; //flush in the beginning of every new block, but not for rule delimiters
    while(utfByte != 0x82){ //should iterate every block
        //printf("Start of block/rule: utfByte is %x\n", utfByte);
        if(utfByte != 0x83){ //Head of rule outside every inner loop
            return EOF; //failure if block sign does not follow transmission sign
        }

        if (utfByte == 0x84) {
            init_symbols(); //new block means new set of symbols
            init_rules(); //and also making a new rule_map
        }


        int headerByte = fgetc(in);//headerByte represents the first byte of the head of a rule
        //printf("headerByte is %x\n", headerByte);
        int codePoint = 0; //codePoint is the UTF-8 character codepoint in binary
        if(headerByte>>7 == 0){ //1 byte
            printf("headerByte was not bigger than 1 byte\n");
            return EOF; //headerByte must be bigger than 1 byte at least
        }
        else if(headerByte>>5 == 6) {//2 bytes
            codePoint = headerByte&31; //the final codepoint in UTF-8 we want to represent
            codePoint <<= 6; //Right shifting 6 to make room
            int secondHeaderByte = fgetc(in);
            codePoint |= secondHeaderByte&63;
        }
        else if(headerByte>>4 == 14){ //3 bytes
            codePoint = headerByte;
            int secondHeaderByte = fgetc(in);
            codePoint <<= 6;
            secondHeaderByte &= 63;
            codePoint |= secondHeaderByte;

            secondHeaderByte = fgetc(in);
            codePoint <<= 6;
            secondHeaderByte &= 63;
            codePoint |= secondHeaderByte;
        }
        else if(headerByte>>3 == 30){ //4 bytes
            codePoint = headerByte;
            int secondHeaderByte = fgetc(in);
            codePoint <<= 6;
            secondHeaderByte &= 63;
            codePoint |= secondHeaderByte;

            secondHeaderByte = fgetc(in);
            codePoint <<= 6;
            secondHeaderByte &= 63;
            codePoint |= secondHeaderByte;

            secondHeaderByte = fgetc(in);
            codePoint <<= 6;
            secondHeaderByte &= 63;
            codePoint |= secondHeaderByte;
        }
        else {
            printf("Not valid UTF-8 header char\n");
            return EOF; //if not valid UTF-8 header, return EOF;
        } //end of header codePoint selection

        //selecting type of rule(main or child)
        //printf("utfByte at type of rule selection is %x\n", utfByte);
        SYMBOL *brand_new_rule = new_rule(codePoint);
        /*if(utfByte == 0x83){ //if new block
            add_rule(brand_new_rule);
            //main_rule = new_rule(codePoint); //main_rule was reset
            //child_rule = NULL; //cleaning up memory SHOULD I RECYCLE THIS????????????????????????????????????????????????
            // *(rule_map + main_rule->value) = main_rule; //adding main_rule to rule_map
        }
        else if(utfByte == 0x85){ //if new rule in block
            //SYMBOL *rule = new_rule(codePoint);
            add_rule(brand_new_rule);
            *(rule_map + brand_new_rule->value) = brand_new_rule; //adding child rule to rule_map
        }*/
        add_rule(brand_new_rule);

        int utfByteCopy = utfByte; //copy for whether main rule or child rule

        //decompressing body of rule
        utfByte = fgetc(in); //will be initialized as the char right after the header, and either 0x84 or 0x85 after inner loop finishes
        while(utfByte != 0x84 && utfByte != 0x85){
            //printf("utfByte at start of iteration of inner loop is %x\n", utfByte);

            if(utfByte>>7 == 0){ //1 byte
                codePoint = utfByte & 127;
                //printf("The current 1 byte utf-8 char is %x\n", utfByte);
            }
            else if(utfByte>>5 == 6){ //2 bytes
                codePoint = utfByte&31; //the final codepoint in UTF-8 we want to represent
                codePoint <<= 6; //Right shifting 6 to make room
                int secondHeaderByte = fgetc(in);
                codePoint |= secondHeaderByte&63;
                //printf("The current 2 byte utf-8 char starts off with %x and is followed by %x\n", utfByte, secondHeaderByte);
            }
            else if(utfByte>>4 == 14){ //3 bytes
                codePoint = utfByte;
                int secondHeaderByte = fgetc(in);
                codePoint <<= 6;
                secondHeaderByte &= 63;
                codePoint |= secondHeaderByte;

                secondHeaderByte = fgetc(in);
                codePoint <<= 6;
                secondHeaderByte &= 63;
                codePoint |= secondHeaderByte;
            }
            else if(utfByte>>3 == 30){ //4 bytes
                codePoint = utfByte;
                int secondHeaderByte = fgetc(in);
                codePoint <<= 6;
                secondHeaderByte &= 63;
                codePoint |= secondHeaderByte;

                secondHeaderByte = fgetc(in);
                codePoint <<= 6;
                secondHeaderByte &= 63;
                codePoint |= secondHeaderByte;

                secondHeaderByte = fgetc(in);
                codePoint <<= 6;
                secondHeaderByte &= 63;
                codePoint |= secondHeaderByte;
            }else {
                return EOF; //EOF if not in proper UTF-8
            }

            SYMBOL *symbolAdd = new_symbol(codePoint, brand_new_rule);
            brand_new_rule->prev->next = symbolAdd;
            symbolAdd->next = brand_new_rule;
            symbolAdd->prev = brand_new_rule->prev;
            brand_new_rule->prev = symbolAdd;
            utfByte = fgetc(in);
        } //end of inner loop
        //printf("utfByte at end of inner loop is %x\n", utfByte);

        //By the end of the inner loop, utfByte should be pointing at either 0x85 or 0x84
        if(utfByte == 0x84) { //must check 0x85 in beginning of the loop
            //printf("utfByte at 0x84 end of inner loop\n");
            byteTotal += ruleExpand(main_rule, out); //call byteTotal on current block before starting new one
            //printf("byteTotal was updated to %d\n", byteTotal);

            utfByte = fgetc(in); //move from 0x84 to 0x83 for next outer loop iteration
            //printf("utfByte after 0x84 is %x\n", utfByte);
        }
        else if(utfByte == 0x85){
            //add_rule(new_rule(codePoint)); //if rule delimiter, add child rule
            //printf("utfByte at 0x85 end of inner loop\n");
        }
        //fputc(*ruleExpand(main_rule), out); //sends uncompressed string to out
        //byteTotal += ruleExpand(main_rule, out); //call byteTotal on new block before starting new one
        //printf("byteTotal is currently %d\n", byteTotal);

    } //end of outer loop
    printf("last byte: %x\n", utfByte);
    //if(utfByte == 0x82){ //final block must be ruleExpand outside the loop
    //    if(main_rule == NULL){
    //        printf("YOU'RE FUCKING KIDDING ME\n");
    //    }
    //    byteTotal += ruleExpand(main_rule, out); //call byteTotal on new block before starting new one
    //}
    return byteTotal;
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
        return EXIT_FAILURE; //EXIT_FAILURE if no flags are provided
    char *firstArg = *(argv + 1);
    //printf("%d and %d",*firstArg, *(firstArg+1));
    //checking the first character
    if(strEq(firstArg, "-h") == 0) {
        global_options = 1 | global_options; //if -h flag, set LSB to 1
        return EXIT_SUCCESS; //EXIT_SUCCESS if the first argument is -h
    } //if -h is not the first character, it should not exist
    if(argc > 4)
        return EXIT_FAILURE; //if no -h flag, maximum 4 arguments
    if(strEq(firstArg, "-c") == 0) {
        if(argc == 2)
            return EXIT_SUCCESS; //EXIT_SUCCESS if only -c flag exists
        char *secondArg = *(argv + 2); //otherwise test the second argument
        if(strEq(secondArg, "-d") == 0 || strEq(secondArg, "-h") == 0)
            return EXIT_FAILURE; //EXIT_FAILURE if -c -d or -c -h flags);
        else if(strEq(secondArg, "-b") == 0) { //checking that the second flag is -b
            if(argc != 4) {
                //printf("The number of args is: %d\n", argc);
                return EXIT_FAILURE; //EXIT FAILURE if not in format sequitur -c -b BLOCKSIZE
            }
            char *thirdArg = *(argv + 3);
            if(strToInt(thirdArg) == -1){
                printf("BLOCKSIZE was not valid\n");
                return EXIT_FAILURE; //EXIT_FAILURE if BLOCKSIZE is not valid
            }
            else if(1 > strToInt(thirdArg) || strToInt(thirdArg) > 1024) {
                //printf("the failed number was %d\n", strToInt(thirdArg));
                return EXIT_FAILURE; //EXIT_FAILURE if BLOCKSIZe is not valid
            }
            //printf("the successful number was %d\n", strToInt(thirdArg));
            int extendedBlockSize = strToInt(thirdArg)<<16;
            global_options = extendedBlockSize | global_options;//BLOCKSIZE in global_options if -b flag
        }
        global_options = 2 | global_options; //if -c flag, set second LSB to 1
    }
    else if(strEq(firstArg, "-d") == 0) {
        if(argc > 2) {
            return EXIT_FAILURE; //EXIT_FAILURE if a second flag after -d exists
        }
        global_options = 4 | global_options; //if -d flag, set third LSB to 1
    }
    else if(strEq(firstArg, "-b") == 0) {
        return EXIT_FAILURE; //EXIT_FAILURE if the first flag is -b
    }
    return 0;
}
