#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "life.h"
#include <time.h>

int main(int argc, char** argv) {
    clock_t start, end;
    double cpu_time_used;

    start = clock();

    int steps = 0;
    char input_file[256];
    if (argc == 3) {
        steps = atoi(argv[1]);
        strcpy(input_file, argv[2]);
    }
    if (argc != 3) {
        printf("Usage:\n"
               "  %s STEPS FILENAME\n"
               "    Run serial computation and print out result\n", argv[0]);
        return 1;
    }
    LifeBoard* board = malloc(sizeof(LifeBoard));
    FILE* in = fopen(input_file, "r");
    read_life_board(in, board);
    //simulate_life_serial(board, steps);
    simulate_life_parallel(100, board, steps);
    print_life_board(board);
    destroy_life_board(board);

    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("程序运行时间: %f 秒\n", cpu_time_used);
    return 0;
}