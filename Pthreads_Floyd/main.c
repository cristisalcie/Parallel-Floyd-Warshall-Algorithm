#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "utils.h"


// Thread parameters structure
typedef struct _Args{
    long thread_id;
    int N;
    int num_threads;
    int ***graph;
    pthread_barrier_t *barrier;
} Args;


void* run_thread_Floyd(void *arg) {
    long thread_id              = (*(Args *)arg).thread_id;
    long N                      = (*(Args *)arg).N;
    long num_threads            = (*(Args *)arg).num_threads;
    int ***graph                = (*(Args *)arg).graph;
    pthread_barrier_t *barrier  = (*(Args *)arg).barrier;

    int start = (int) (thread_id * (double)N / num_threads);
    int stop  = MIN((int) ((thread_id + 1) * (double)N / num_threads), N);

    for (int k = 0; k < N; ++k) { // Step. Can't modify this.
        for (int i = start; i < stop; ++i) {
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
                (*graph)[i][j] = MIN((*graph)[i][k] + (*graph)[k][j], (*graph)[i][j]);
            }
        }
        pthread_barrier_wait(barrier);
    }

    pthread_exit(NULL);
}


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


    // Declare pthreads data.
    int num_threads = atoi(argv[2]);
    pthread_t threads[num_threads];
    int r;
    void *status;
    Args args[num_threads];
    pthread_barrier_t barrier;
    r = pthread_barrier_init(&barrier, NULL, (unsigned int)num_threads);
    if (r) {
        printf("[main.c, %d] Error: barrier\n", __LINE__);
        exit(-1);
    }


    // Start parallel problem.
    for (long i = 0; i < num_threads; ++i) {
        args[i].thread_id = i;
        args[i].N = N;
        args[i].num_threads = num_threads;
        args[i].graph = &graph;
        args[i].barrier = &barrier;
        r = pthread_create(&threads[i], NULL, run_thread_Floyd, &args[i]);

		if (r) {
	  		printf("[main.c, %d] Error: thread create %ld\n", __LINE__, i);
	  		exit(-1);
		}
  	}


    // Wait for parallel problem to finish.
  	for (long i = 0; i < num_threads; ++i) {
		r = pthread_join(threads[i], &status);

		if (r) {
	  		printf("[main.c, %d] Error: thread join %ld\n", __LINE__, i);
	  		exit(-1);
		}
  	}


    // Write result.
    results_full_path = get_results_path(argv[1]);
    write_graph(results_full_path, N, graph);


    // Clean up.
    r = pthread_barrier_destroy(&barrier);
    if (r) {
        printf("[main.c, %d] Error: barrier\n", __LINE__);
        exit(-1);
    }
    free(results_full_path);
    free_graph(N, &graph);

    return 0;
}
