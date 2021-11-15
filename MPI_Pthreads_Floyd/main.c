#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <pthread.h>
#include "utils.h"

#define ROOT 0

// Thread parameters structure
typedef struct _Args{
    long thread_id;
    int N;
    int num_tasks;
    int num_threads;
    int rank_start;
    int rank_stop;
    int ***graph;
    pthread_barrier_t *barrier;
} Args;

void* mpi_root_run_thread_Floyd(void *arg) {
    long thread_id              = (*(Args *)arg).thread_id;
    long N                      = (*(Args *)arg).N;
    long num_tasks              = (*(Args *)arg).num_tasks;
    long num_threads            = (*(Args *)arg).num_threads;
    long rank_start             = (*(Args *)arg).rank_start;
    long rank_stop              = (*(Args *)arg).rank_stop;
    int ***graph                = (*(Args *)arg).graph;
    pthread_barrier_t *barrier  = (*(Args *)arg).barrier;

    int thread_start = (int) (thread_id * (double)N / num_threads);
    int thread_stop  = MIN((int) ((thread_id + 1) * (double)N / num_threads), N);

    for (int k = 0; k < N; ++k) { // Step. Can't modify this.
        for (int i = rank_start; i < rank_stop; ++i) {
            if (i == k) {
              // Distance from i to j through i (when k = i) is not changing  
              continue;
            }
            for (int j = thread_start; j < thread_stop; ++j) {
                if (j == k || i == j) {
                    // Distance from i to j through j (when k = j) is not changing
                    // Distance to the node itself is always 0.
                    continue;
                }
                (*graph)[i][j] = MIN((*graph)[i][k] + (*graph)[k][j], (*graph)[i][j]);
            }
        }

        // We need the graph intact while there is still calculus to do.
        pthread_barrier_wait(barrier);
        if (thread_id == 0) {
            // Receive partial results from workers.
            for (int i = 0; i < num_tasks; ++i) {
                if (i == ROOT) continue;
                int recv_start = (int) (i * (double)N / num_tasks);
                int recv_stop  = MIN((int) ((i + 1) * (double)N / num_tasks), N);

                // Receive partial result from worker i.
                for (int j = recv_start; j < recv_stop; ++j) {
                    MPI_Recv((*graph)[j], N, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
            }

            // Send entire updated graph to workers to have it used in calculus for next step (k).
            for (int i = 0; i < num_tasks; ++i) {
                if (i == ROOT) continue;
                for (int j = 0; j < N; ++j) {
                    MPI_Send((*graph)[j], N, MPI_INT, i, 0, MPI_COMM_WORLD);
                }
            }
        }
        // So we know we have the updated graph to be used for next step.
        pthread_barrier_wait(barrier);
    }

    pthread_exit(NULL);
}


void* mpi_worker_run_thread_Floyd(void *arg) {
    long thread_id              = (*(Args *)arg).thread_id;
    long N                      = (*(Args *)arg).N;
    long num_threads            = (*(Args *)arg).num_threads;
    long rank_start             = (*(Args *)arg).rank_start;
    long rank_stop              = (*(Args *)arg).rank_stop;
    int ***graph                = (*(Args *)arg).graph;
    pthread_barrier_t *barrier  = (*(Args *)arg).barrier;

    int thread_start = (int) (thread_id * (double)N / num_threads);
    int thread_stop  = MIN((int) ((thread_id + 1) * (double)N / num_threads), N);

    for (int k = 0; k < N; ++k) { // Step. Can't modify this.
        for (int i = rank_start; i < rank_stop; ++i) {
            if (i == k) {
              // Distance from i to j through i (when k = i) is not changing  
              continue;
            }
            for (int j = thread_start; j < thread_stop; ++j) {
                if (j == k || i == j) {
                    // Distance from i to j through j (when k = j) is not changing
                    // Distance to the node itself is always 0.
                    continue;
                }
                (*graph)[i][j] = MIN((*graph)[i][k] + (*graph)[k][j], (*graph)[i][j]);
            }
        }

        // We need this in order to send correct results.
        pthread_barrier_wait(barrier);
        if (thread_id == 0) {
            // Send partial result to ROOT.
            for (int i = rank_start; i < rank_stop; ++i) {
                MPI_Send((*graph)[i], N, MPI_INT, ROOT, 0, MPI_COMM_WORLD);
            }

            // Receive entire updated graph to be used in calculus for next step (k).
            for (int i = 0; i < N; ++i) {
                MPI_Recv((*graph)[i], N, MPI_INT, ROOT, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }
        // We need this to have the graph updated for the next k step.
        pthread_barrier_wait(barrier);
    }

    pthread_exit(NULL);
}

void mpi_send_graph_to_workers(int num_tasks, int N, int **graph) {
    // Send N to all workers.
    for (int i = 0; i < num_tasks; ++i) {
        if (i == ROOT) continue;
        MPI_Send(&N, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
    }

    // Send graph to all workers.
    for (int i = 0; i < num_tasks; ++i) {
        if (i == ROOT) continue;
        for (int j = 0; j < N; ++j) {
            MPI_Send(graph[j], N, MPI_INT, i, 0, MPI_COMM_WORLD);
        }
    }
}

void mpi_recv_graph_from_root(int *N, int ***graph) {
    // Worker waits to receive N.
    MPI_Recv(N, 1, MPI_INT, ROOT, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    if (*(graph) != NULL) {
        printf("[main.c, %d] Error: graph was previously allocated!\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    (*graph) = malloc(*N * sizeof(int*));
    if (*graph == NULL) {
        printf("[main.c, %d] Error: malloc failed to allocate graph!\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < *N; ++i) {
        (*graph)[i] = malloc(*N * sizeof(int));
        if ((*graph)[i] == NULL) {
            printf("[main.c, %d] Error: malloc failed to allocate graph!\n", __LINE__);
            exit(EXIT_FAILURE);
        }
    }

    // Worker waits to receive graph.
    for (int i = 0; i < *N; ++i) {
        MPI_Recv((*graph)[i], *N, MPI_INT, ROOT, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("[main.c, %d] Error run syntax: \"./main $test_file_path $number_threads\"\n"
            "Example: ./main tests/test_1.out 4\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    int num_tasks;
    int rank;
    int N;
    int **graph = NULL;
    int provided;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_SERIALIZED, &provided);
    MPI_Comm_size(MPI_COMM_WORLD, &num_tasks);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);

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
    
    
    if (rank == ROOT) {  // Root code
        char *results_full_path = NULL;

        read_graph(argv[1], &N, &graph);

        int rank_start = (int) (rank * (double)N / num_tasks);
        int rank_stop  = MIN((int) ((rank + 1) * (double)N / num_tasks), N);

        if (num_tasks > N) {
            printf("Number of workers must be <= number of nodes (N).\n");
            exit(-1);
        }

        mpi_send_graph_to_workers(num_tasks, N, graph);

        // Start mpi parallel problem for root.
        for (long i = 0; i < num_threads; ++i) {
            args[i].thread_id = i;
            args[i].N = N;
            args[i].num_tasks = num_tasks;
            args[i].num_threads = num_threads;
            args[i].rank_start = rank_start;
            args[i].rank_stop = rank_stop;
            args[i].graph = &graph;
            args[i].barrier = &barrier;
            r = pthread_create(&threads[i], NULL, mpi_root_run_thread_Floyd, &args[i]);

            if (r) {
                printf("[main.c, %d] Error: thread create %ld\n", __LINE__, i);
                exit(-1);
            }
        }

        // Wait for mpi parallel problem to finish.
        for (long i = 0; i < num_threads; ++i) {
            r = pthread_join(threads[i], &status);

            if (r) {
                printf("[main.c, %d] Error: thread join %ld\n", __LINE__, i);
                exit(-1);
            }
        }

        results_full_path = get_results_path(argv[1]);
        write_graph(results_full_path, N, graph);
        free(results_full_path);
    } else {  // Worker code
        mpi_recv_graph_from_root(&N, &graph);

        int rank_start = (int) (rank * (double)N / num_tasks);
        int rank_stop  = MIN((int) ((rank + 1) * (double)N / num_tasks), N);

        // Start mpi parallel problem for worker.
        for (long i = 0; i < num_threads; ++i) {
            args[i].thread_id = i;
            args[i].N = N;
            args[i].num_tasks = num_tasks;
            args[i].num_threads = num_threads;
            args[i].rank_start = rank_start;
            args[i].rank_stop = rank_stop;
            args[i].graph = &graph;
            args[i].barrier = &barrier;
            r = pthread_create(&threads[i], NULL, mpi_worker_run_thread_Floyd, &args[i]);

            if (r) {
                printf("[main.c, %d] Error: thread create %ld\n", __LINE__, i);
                exit(-1);
            }
        }

        // Wait for mpi parallel problem to finish.
        for (long i = 0; i < num_threads; ++i) {
            r = pthread_join(threads[i], &status);

            if (r) {
                printf("[main.c, %d] Error: thread join %ld\n", __LINE__, i);
                exit(-1);
            }
        }
    }

    // Clean up.
    r = pthread_barrier_destroy(&barrier);
    if (r) {
        printf("[main.c, %d] Error: barrier\n", __LINE__);
        exit(-1);
    }
    free_graph(N, &graph);
    MPI_Finalize();
    return 0;
}

