#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("[generator.c, %d] Error run syntax: \"./generator $test_number\"\n"
            "Example: ./generator 1\n", __LINE__);
        exit(EXIT_FAILURE);
    }
    
    int N;
    int **graph;
    char dir_path[] = "tests/";
    char full_path[128];

    sprintf(full_path, "%stest_%s.out", dir_path, argv[1]);
    printf("Enter number of nodes (N): ");
    scanf("%d", &N);
    printf("\n");
    

    graph = malloc(N * sizeof(int*));
    for (int i = 0; i < N; ++i) {
        graph[i] = malloc(N * sizeof(int));
    }

    srand(time(NULL));

    // ----------------------- Generate random graph --------------------------
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (i == j) {
                graph[i][j] = 0;  // The node itself hence cost is 0
                continue;
            }
            graph[i][j] = 1 + rand() % 999;
        }
    }
    // ------------------------------------------------------------------------

    // ------------------------- Start file writing ---------------------------
    FILE *fp = fopen(full_path, "w");

    fprintf(fp, "%d\n", N);

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            fprintf(fp, "%d ", graph[i][j]);
            // printf("%2d ", graph[i][j]);
        }
        // printf("\n");
    }
    // ------------------------------------------------------------------------

    fclose(fp);
    for (int i = 0; i < N; ++i) {
        free(graph[i]);
    }
    free(graph);
    return 0;
}

