#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "utils.h"


int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("[main.c, %d] Error run syntax: \"./main $test_file_path $number_threads\"\n"
            "Example: ./main tests/test_1.out 4\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    // Declare and initialise graph data.
    int N;
    int **graph = NULL;
    char *results_full_path = NULL;

    read_graph(argv[1], &N, &graph);


    // Declare omp data.
    int num_threads = atoi(argv[2]);
    omp_set_num_threads(num_threads);


    // Parallel solution for floyd.
    #pragma omp parallel shared(graph, N)
    {
        for (int k = 0; k < N; ++k) {
            #pragma omp for
            for (int i = 0; i < N; ++i) {
                if (i == k) {
                    // Distance from i to j through i (when k = i) is not changing  
                    continue;
                }
                for (int j = 0; j < N; ++j) {
                    if (j == k || i == j) {
                        // Distance from i to j through j (when k = j) is not changing
                        // Distance to the node itself is always 0.
                        continue;
                    }
                    graph[i][j] = MIN(graph[i][k] + graph[k][j], graph[i][j]);
                }
            }
            #pragma omp barrier
        }
    }


    // Write result.
    results_full_path = get_results_path(argv[1]);
    write_graph(results_full_path, N, graph);


    // Clean up.
    free(results_full_path);
    free_graph(N, &graph);

    return 0;
}
