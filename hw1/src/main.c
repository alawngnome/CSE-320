#include <stdio.h>
#include <stdlib.h>

#include "const.h"
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

int main(int argc, char **argv)
{
    //printf("strEq returned for -c and -d: %d", strEq("-c", "-d"));
    //printf("validargs returned: %d\n", validargs(argc, argv));

    if(validargs(argc, argv) == -1)
        USAGE(*argv, EXIT_FAILURE);
    debug("Options: 0x%x", global_options);
    if((global_options & 1) == 1) //help flag called here
        USAGE(*argv, EXIT_SUCCESS);
    /**If validargs returns 0, then your program must read data from stdin,
    either compressing it or decompressing it as specified by the values of
    global_options and block_size, and writing the result to stdout.
    **/
    else if(validargs(argc, argv) == 0){
        //printf("global_options is: %x\n", global_options);
        if((global_options & 4)>>2 == 1){ //third LSB is decompress
            //printf("decompress is being called");
            if(decompress(stdin, stdout) == EOF){
                return EXIT_FAILURE;
            }else {
                return EXIT_SUCCESS;
            }
        }
        else if((global_options & 2)>>1 == 1){} //second LSB in compress
            if(compress(stdin, stdout, ((global_options>>16)*1024)) == EOF){
                return EXIT_FAILURE;
            }else {
                return EXIT_SUCCESS;
            }
    }

}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
