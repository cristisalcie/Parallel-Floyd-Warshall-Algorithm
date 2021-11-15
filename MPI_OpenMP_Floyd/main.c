#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <omp.h>
#include "utils.h"

#define ROOT 0

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

void mpi_run_floyd_root(int rank, int num_tasks, int N, int ***graph) {
    int start = (int) (rank * (double)N / num_tasks);
    int stop  = MIN((int) ((rank + 1) * (double)N / num_tasks), N);

    #pragma omp parallel shared(graph, N, start, stop)
    {
        for (int k = 0; k < N; ++k) {
            for (int i = start; i < stop; ++i) {
                if (i == k) {
                    // Distance from i to j through i (when k = i) is not changing
                    continue;
                }
                #pragma omp for
                for (int j = 0; j < N; ++j) {
                    if (j == k || i == j) {
                        // Distance from i to j through j (when k = j) is not changing.
                        // Distance to the node itself is always 0.
                        continue;
                    }
                    (*graph)[i][j] = MIN((*graph)[i][k] + (*graph)[k][j], (*graph)[i][j]);
                }
            }

            // We need the graph intact while there is still calculus to do.
            #pragma omp barrier
            #pragma omp single
            {
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

                // Send entire updated graph to workers to have it used in calculus for next step.
                for (int i = 0; i < num_tasks; ++i) {
                    if (i == ROOT) continue;
                    for (int j = 0; j < N; ++j) {
                        MPI_Send((*graph)[j], N, MPI_INT, i, 0, MPI_COMM_WORLD);
                    }
                }
            }
            // So we know we have the updated graph to be used for next step.
            #pragma omp barrier
        }
    }
}

void mpi_run_floyd_worker(int rank, int num_tasks, int N, int ***graph) {
    int start = (int) (rank * (double)N / num_tasks);
    int stop  = MIN((int) ((rank + 1) * (double)N / num_tasks), N);

    #pragma omp parallel shared(graph, N, start, stop)
    {
        for (int k = 0; k < N; ++k) { // Step. Can't modify this.
            for (int i = start; i < stop; ++i) {
                if (i == k) {
                // Distance from i to j through i (when k = i) is not changing.
                continue;
                }
                #pragma omp for
                for (int j = 0; j < N; ++j) {
                    if (j == k || i == j) {
                        // Distance from i to j through j (when k = j) is not changing.
                        // Distance to the node itself is always 0.
                        continue;
                    }
                    (*graph)[i][j] = MIN((*graph)[i][k] + (*graph)[k][j], (*graph)[i][j]);
                }
            }

            #pragma omp barrier  // We need this in order to send correct results.
            #pragma omp single
            {
                // Send partial result to ROOT.
                for (int i = start; i < stop; ++i) {
                    MPI_Send((*graph)[i], N, MPI_INT, ROOT, 0, MPI_COMM_WORLD);
                }

                // Receive entire updated graph to be used in calculus for next step (k).
                for (int i = 0; i < N; ++i) {
                    MPI_Recv((*graph)[i], N, MPI_INT, ROOT, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
            }
            #pragma omp barrier // We need this to have the graph updated for the next k step.
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("[main.c, %d] Error run syntax: \"./main $test_file_path $number_threads\"\n"
            "Example: ./main tests/test_1.out 4\n", __LINE__);
        exit(EXIT_FAILURE);
    }

    int num_tasks;
    int num_threads = atoi(argv[2]);
    int rank;
    int N;
    int **graph = NULL;
    int provided;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_SERIALIZED, &provided);
    MPI_Comm_size(MPI_COMM_WORLD, &num_tasks);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);

    omp_set_num_threads(num_threads);

    if (rank == ROOT) {  // Root code
        char *results_full_path = NULL;

        read_graph(argv[1], &N, &graph);
        if (num_tasks > N) {
            printf("Number of workers must be <= number of nodes (N).\n");
            exit(-1);
        }

        mpi_send_graph_to_workers(num_tasks, N, graph);
        mpi_run_floyd_root(rank, num_tasks, N, &graph);

        results_full_path = get_results_path(argv[1]);
        write_graph(results_full_path, N, graph);
        free(results_full_path);
    } else {  // Worker code
        mpi_recv_graph_from_root(&N, &graph);
        mpi_run_floyd_worker(rank, num_tasks, N, &graph);
    }

    free_graph(N, &graph);
    MPI_Finalize();
    return 0;
}

