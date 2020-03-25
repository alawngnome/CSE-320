#include <stdio.h>
#include "sfmm.h"

int main(int argc, char const *argv[]) {
    sf_mem_init();

    /*double* ptr = sf_malloc(sizeof(double));
    // *ptr = 320320320e-320;
    *ptr = 1234.5678;

    sf_free(ptr);

    sf_show_heap();*/
    void *x = sf_malloc(sizeof(int) * 20);
    printf("\n\n");
    void *y = sf_realloc(x, sizeof(int) * 16);
    if(y == NULL){
        //wow
    }
    sf_show_heap();

    sf_mem_fini();

    return EXIT_SUCCESS;
}
