#include <stdio.h>
#include "sfmm.h"

int main(int argc, char const *argv[]) {
    sf_mem_init();

    /*double* ptr = sf_malloc(sizeof(double));
    // *ptr = 320320320e-320;
    *ptr = 1234.5678;

    sf_free(ptr);

    sf_show_heap();*/
    /*double *ptr = sf_malloc(300 + 64 + 512);
    *ptr = 1234.5678;
    sf_realloc(ptr, 256);*/
    sf_memalign(300, 512);
    sf_show_heap();

    sf_mem_fini();

    return EXIT_SUCCESS;
}
