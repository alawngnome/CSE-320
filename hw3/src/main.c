#include <stdio.h>
#include "sfmm.h"

int main(int argc, char const *argv[]) {
    sf_mem_init();

    /*void *x = sf_malloc(1);
    void *y =  sf_realloc(x, 0);*/


    sf_mem_fini();

    return EXIT_SUCCESS;
}
