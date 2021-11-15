#ifndef __UTILS_H
#define __UTILS_H

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

void read_graph(char *path, int *N, int ***graph) {
    if (path == NULL) {
        printf("[utils.h, %d] Error: read path to file not set!\n", __LINE__);
        exit(EXIT_FAILURE);
    }
    
    FILE *fp = fopen(path, "r");

    fscanf(fp, "%d", N);

    if (*(graph) != NULL) {
        printf("[utils.h, %d] Error: graph was previously allocated!\n", __LINE__);
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    (*graph) = malloc(*N * sizeof(int*));
    if (*graph == NULL) {
        printf("[utils.h, %d] Error: malloc failed to allocate graph!\n", __LINE__);
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < *N; ++i) {
        (*graph)[i] = malloc(*N * sizeof(int));
        if ((*graph)[i] == NULL) {
            printf("[utils.h, %d] Error: malloc failed to allocate graph!\n", __LINE__);
            fclose(fp);
            exit(EXIT_FAILURE);
        }

        for (int j = 0; j < *N; ++j) {
            fscanf(fp, "%d", &((*graph)[i][j]));
        }
    }
    
    fclose(fp);
}

void write_graph(char* path, int N, int **graph) {
    if (graph == NULL || *graph == NULL) return;

    FILE *fp = fopen(path, "w");

    fprintf(fp, "%d\n", N);
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            fprintf(fp, "%d ", graph[i][j]);
        }
    }
    
    fclose(fp);
}

void print_graph(int N, int **graph) {
    if (graph == NULL || *graph == NULL) {
        printf("[utils.h, %d] Error: graph not initialised for printing!", __LINE__);
        return;
    }

    printf("\n");
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            printf("%3d ", graph[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

void free_graph(int N, int ***graph) {
    if (graph == NULL || *graph == NULL) return;

    for (int i = 0; i < N; ++i) {
        if ((*graph)[i] == NULL) continue;
        free((*graph)[i]);
    }
    free(*graph);
}

char* get_results_path(char *test_path) {
    char *result = malloc(32 * sizeof(char));
    int test_path_len = strlen(test_path);
    int start_copy = 0;
    char test_num[16];
    int j = 0;  // At the end it will have the strlen of numbers

    for (int i = 0; i < test_path_len; ++i) {
        if (test_path[i] == '_') {
            start_copy = 1;
            continue;
        }
        else if (test_path[i] == '.') {
            start_copy = 0;
            test_num[j] = '\0';
            break;
        }
        if (start_copy) {
            test_num[j] = test_path[i];
            ++j;
        }
    }
    sprintf(result, "results/result_%s.out", test_num);
    return result;
}

#endif
