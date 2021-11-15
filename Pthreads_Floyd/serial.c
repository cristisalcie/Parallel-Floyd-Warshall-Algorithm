#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("[serial.c, %d] Error run syntax: \"./serial $test_number\"\n"
            "Example: ./serial 1\n", __LINE__);
        exit(EXIT_FAILURE);
    }
    char test_full_path[128];
    char ref_full_path[128];

    sprintf(test_full_path, "tests/test_%s.out", argv[1]);
    sprintf(ref_full_path, "refs/ref_%s.out", argv[1]);
    
    // ----------------------- Read graph from file ---------------------------
    int N;
    int **graph = NULL;
    read_graph(test_full_path, &N, &graph);
    // ------------------------------------------------------------------------
    

    // ---------------------- Floyd Warshall Algorithm ------------------------
    for (int k = 0; k < N; k++) {
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                graph[i][j] = MIN(graph[i][k] + graph[k][j], graph[i][j]);
            }
        }
    }
    // ------------------------------------------------------------------------


    // ---------------------------- Print result ------------------------------
    // print_graph(N, graph);
    // ------------------------------------------------------------------------


    // ---------------------------- Write result ------------------------------
    write_graph(ref_full_path, N, graph);
    // ------------------------------------------------------------------------


    // ---------------------------- Free memory -------------------------------
    for (int i = 0; i < N; ++i) {
        free(graph[i]);
    }
    free(graph);
    // ------------------------------------------------------------------------
    return 0;
}
