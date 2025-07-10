#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mpi.h"
#include <sys/resource.h>
#include <malloc.h>

#define PRINTS 0
#define MAX_LINE_LENGTH 10000 // Maximum length of a line in the CSV file
#ifndef MATRIX_DIM  // Dimension of the original matrix (MATRIX_DIM x MATRIX_DIM)
#define MATRIX_DIM 100
#endif

#ifndef MAX_PROCESSES   // Maximum number of processes for the Cartesian grid
#define MAX_PROCESSES 4
#endif

#define MATRIX_SIZE (MATRIX_DIM * MATRIX_DIM) 

void print_matrix(int* matrix, int dim) {
    for (int r = 0; r < dim; r++) {
        for (int c = 0; c < dim; c++) {
           printf("%d ", matrix[r * dim + c]);
        }
       printf("\n");
    }
   printf("\n");
}

// Function to multiply two sub-matrices and add the result to a third sub-matrix
// A, B are the input matrices, C is the result matrix
// subm_dimensions is the dimension of the sub-matrices
void multiply_add(int* A, int* B, int* C, int subm_dimensions) {
    for (int i = 0; i < subm_dimensions; i++) {
        for (int j = 0; j < subm_dimensions; j++) {
            for (int k = 0; k < subm_dimensions; k++) {
                C[i * subm_dimensions + j] += A[i * subm_dimensions + k] * B[k * subm_dimensions + j];
            }
        }
    }
}

int compute_start_index(int total, int coord, int dims) {
    // Calculate the start index for the sub-matrix based on the coordinate and dimensions
    return (total / dims) * coord;
}

//returns 0 on success, -1 on failure
// Reads a matrix from a CSV file and fills the provided matrix array
// The matrix is expected to be of size MATRIX_DIM x MATRIX_DIM
// Each line in the CSV file represents a row of the matrix
int read_matrix_from_csv(int* matrix, const char* filename, int total_dim) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
    }

    char line[MAX_LINE_LENGTH];
    int row = 0;
    while (fgets(line, sizeof(line), file) && row < MATRIX_DIM) {
        char* token = strtok(line, ",");
        int col = 0;
    
        #if PRINTS == 1
            printf("line %d: %s", row, line);
        #endif

        // Fill the first original_dim columns with actual values
        while (col < MATRIX_DIM && token) {

            matrix[row * total_dim + col] = atoi(token);
            token = strtok(NULL, ",");
            col++;
        }
        row++;
    }
    // Check if the matrix is fully filled
    if (row < MATRIX_DIM) {
        fprintf(stderr, "Error: The matrix in %s is not fully filled. Expected %d rows, got %d.\n", filename, MATRIX_DIM, row);
        return -1;
    }
    fclose(file);
    return 0;
}

int output_matrix_to_csv(int* matrix, const char* filename, int total_dim) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        perror("Failed to open file for writing");
        return -1;
    }   
    for (int r = 0; r < MATRIX_DIM; r++) {
        for (int c = 0; c < MATRIX_DIM; c++) {
            fprintf(file, "%d", matrix[r * total_dim  + c]);
            if (c < MATRIX_DIM - 1) {
                fprintf(file, ",");
            }

        }
        fprintf(file, "\n");
    }
    fclose(file);
    return 0;
}

int main(int argc, char** argv) {
    /**
     * definition of the Communication world
     */
    
    // Initialize the resource usage structure and timing variables
    // This will be used to measure the resource usage of the program
    struct rusage usage_start, usage_end;
    double time_end_local, time_start_local;
    double elapsed_local, user_time_local, sys_time_local;
    long rss_local, ctx_switch_local;

    // Get the initial resource usage
    // This will capture the resource usage at the start of the program
    getrusage(RUSAGE_SELF, &usage_start);

    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    MPI_Barrier(MPI_COMM_WORLD);
    if (rank < MAX_PROCESSES) {
        time_start_local = MPI_Wtime();
        getrusage(RUSAGE_SELF, &usage_start);
    }

    MPI_Status Stat;
    // check if the number of processes is less than or equal to MAX_PROCESSES
    if (size < MAX_PROCESSES) {
        if (rank == 0) {
           printf("Error: The number of processes must be at least %d.\n", MAX_PROCESSES);
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
        return -1;
    }
    // Check if the number of processes is a perfect square
    if(sqrt(MAX_PROCESSES) != (int)sqrt(MAX_PROCESSES)) {
        if (rank == 0) {
           printf("Error: The number of processes must be a perfect square.\n");
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
        return -1;
    }
    //Dimension of the processes matrix
    int P =  sqrt(MAX_PROCESSES);
    
    //padding to make the matrix dimensions divisible by P
    int pad = 0;
    //Dimensions of the sub-matrix after the padding of the original matrix
    int subm_dim = 0;
    
    if(MATRIX_DIM % MAX_PROCESSES != 0) {
        pad = ((MATRIX_DIM + P - 1) / P) * P - MATRIX_DIM; // Calculate padding to make it divisible by p_dim
    }
    #if PRINTS == 1
        printf("Padding: %d\n", pad);
    #endif

    int total_dim = MATRIX_DIM + pad; // Total dimensions after padding
    #if PRINTS == 1
        printf("Total dimensions after padding: %d\n", total_dim);
    #endif

    subm_dim = (MATRIX_DIM + pad) / P; // Calculate the dimensions of the sub-matrix

    #if PRINTS == 1
        printf("Sub-matrix dimensions: %d\n", subm_dim);
    #endif


    //creation of the sub-matrices for each process
    int *tempA = calloc(subm_dim * subm_dim, sizeof(int));
    int *tempB = calloc(subm_dim * subm_dim, sizeof(int));
    int *tempC = calloc(subm_dim * subm_dim, sizeof(int));

    //coordinates needed for the shifting of the sub-matrices
    int left, right, up, down;

    /*
     *Creation of the Cartesian grid 
     */
    MPI_Comm cart_comm;
    int dims[2] = {P, P}; // Dimensions of the Cartesian grid
    int periods[2] = {1,1}; // Periodicity in each dimension (0 for non-periodic)
    int reorder = 0;
    int code = MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, reorder, &cart_comm);
    if (code != MPI_SUCCESS) {
       printf("Error creating Cartesian communicator\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
        return -1;
    }
    int coords[2];
    if(rank == 0){
        int* matrixA = calloc(total_dim * total_dim, sizeof(int));

        #if PRINTS == 1
            printf("0. Matrix A reading from CSV:\n");
            print_matrix(matrixA, total_dim);
        #endif

        int* matrixB = calloc(total_dim * total_dim, sizeof(int));

        // Read the matrices from CSV files
        // The matrices are expected to be at least of size MATRIX_DIM x MATRIX_DIM

        if (read_matrix_from_csv(matrixA, "matrixA.csv", total_dim) == -1 || read_matrix_from_csv(matrixB, "matrixB.csv", total_dim) == -1) {
            fprintf(stderr, "Error reading matrices from CSV files.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
            free(matrixA);
            free(matrixB);
            return -1;
        }

        #if PRINTS == 1
            printf("1. Matrix A after reading from CSV:\n");
            print_matrix(matrixA, total_dim);
            printf("2. Matrix B after reading from CSV:\n");
            print_matrix(matrixB, total_dim);
        #endif

        //Send the sub-matrices to each process
        for(int i = 1; i < MAX_PROCESSES; i++) {
            MPI_Cart_coords(cart_comm, i, 2, coords);

            // Calculate the starting row and column indices for the sub-matrix
            // The starting indices are computed based on the coordinates of the process in the Cartesian grid
            // This ensures that each process gets the correct sub-matrix from the original matrix
            int start_row = compute_start_index(total_dim, coords[0], P);
            int start_column = compute_start_index(total_dim, coords[1], P);

            //copy the sub-matrix for each process
            for (int i = 0; i < subm_dim; i++) {
                for (int j = 0; j < subm_dim; j++) {
                tempA[i * subm_dim + j] = matrixA[(start_row + i) * (total_dim) + (start_column + j)];
                tempB[i * subm_dim + j] = matrixB[(start_row + i) * (total_dim) + (start_column + j)];
                }
            }

            // Send the submatrix structure to the process
            MPI_Send(tempA, subm_dim * subm_dim, MPI_INT, i, 1, MPI_COMM_WORLD);
            MPI_Send(tempB, subm_dim * subm_dim, MPI_INT, i, 1, MPI_COMM_WORLD);

            #if PRINTS == 1
            printf("Process 0 sending submatrix to process %d:\n", i);
            #endif
        }

        // Copy the sub-matrix for process 0
        MPI_Cart_coords(cart_comm, 0, 2, coords);
        int start_row = compute_start_index(total_dim, coords[0], P);
        int start_column = compute_start_index(total_dim, coords[1], P);
        for (int i = 0; i < subm_dim; i++) {
            for (int j = 0; j < subm_dim; j++) {
                tempA[i * subm_dim + j] = matrixA[(start_row + i) * (total_dim) + (start_column + j)];
                tempB[i * subm_dim + j] = matrixB[(start_row + i) * (total_dim) + (start_column + j)];
            }
        }

        free(matrixA);
        free(matrixB);
    }
    else if(rank < MAX_PROCESSES){
        // Receive the sub-matrices for each process
        MPI_Recv(tempA, subm_dim * subm_dim, MPI_INT, 0, 1, MPI_COMM_WORLD, &Stat);
        MPI_Recv(tempB, subm_dim * subm_dim, MPI_INT, 0, 1, MPI_COMM_WORLD, &Stat);
        #if PRINTS == 1
            printf("4. Process %d received submatrix\n", rank);
        #endif
    }

    if(rank < MAX_PROCESSES){
        // Get the coordinates of the process in the Cartesian grid
        MPI_Cart_coords(cart_comm, rank, 2, coords);
        #if PRINTS == 1
            printf("5. Rank %d: dims = (%d, %d), periods = (%d, %d), coords = (%d, %d)\n",
                rank, dims[0], dims[1], periods[0], periods[1], coords[0], coords[1]);
        #endif

        for (int i = 0; i < coords[0]; i++) {
            int left, right;
            MPI_Cart_shift(cart_comm, 1, -1, &right, &left);
            MPI_Sendrecv_replace(tempA, subm_dim * subm_dim, MPI_INT, left, 0, right, 0, cart_comm, MPI_STATUS_IGNORE);
        }

        #if PRINTS == 1
            printf("Submatrix A, rank %d before shift:\n", rank);
            print_matrix(tempA, subm_dim);
            printf("Submatrix B, rank %d before shift:\n", rank);
            print_matrix(tempB, subm_dim);
            printf("6. Process %d: shifting left from %d to %d\n", rank, left, right);
            MPI_Sendrecv_replace(tempA, subm_dim * subm_dim, MPI_INT, left, 0, right, 0, cart_comm, MPI_STATUS_IGNORE);
            printf("Submatrix A, rank %d after shift:\n", rank);
            print_matrix(tempA, subm_dim);
            printf("Submatrix B, rank %d after shift:\n", rank);
            print_matrix(tempB, subm_dim);
        #endif

        for (int i = 0; i < coords[1]; i++) {
            int up, down;
            MPI_Cart_shift(cart_comm, 0, -1, &down, &up);
            MPI_Sendrecv_replace(tempB, subm_dim * subm_dim, MPI_INT, up, 0, down, 0, cart_comm, MPI_STATUS_IGNORE);
        }
        
        #if PRINTS == 1
            printf("7. Process %d: shifting up from %d to %d\n", rank, up, down);
        #endif
        
        // Cannon Algorithm main loop
        // Each step involves shifting the sub-matrices and performing the multiplication
        
        for (int step = 0; step < P; step++) {
            multiply_add(tempA, tempB, tempC, subm_dim);

            // Shift A left
            MPI_Cart_shift(cart_comm, 1, -1, &right, &left);
            MPI_Sendrecv_replace(tempA, subm_dim * subm_dim, MPI_INT, left, 0, right, 0, cart_comm, MPI_STATUS_IGNORE);

            // Shift B up
            MPI_Cart_shift(cart_comm, 0, -1, &down, &up);
            MPI_Sendrecv_replace(tempB, subm_dim * subm_dim, MPI_INT, up, 0, down, 0, cart_comm, MPI_STATUS_IGNORE);
        }

        //Send the result back to process 0
        
        if (rank != 0 && rank < MAX_PROCESSES) {
            MPI_Send(tempC, subm_dim * subm_dim, MPI_INT, 0, 2, MPI_COMM_WORLD);
            
            #if PRINTS == 1
            printf("8. I am process %d, sending submatrix to process 0\n", rank);
            #endif
        }
        else{
            // Process 0 will gather the results from all processes
            // and construct the final matrix C

            int* matrixC = calloc(total_dim * total_dim, sizeof(int));
            for (int p = 0; p < MAX_PROCESSES; p++) {
                MPI_Cart_coords(cart_comm, p, 2, coords);

                // Allocate memory for the temporary matrix to receive data
                int* temp_buffer = calloc(subm_dim * subm_dim, sizeof(int));
                if(p != 0){                    
                    MPI_Recv(temp_buffer, 
                    subm_dim * subm_dim,
                    MPI_INT, p, 2, MPI_COMM_WORLD, &Stat);
                }
                else {
                    // Copy the result from process 0's tempC to the matrixC
                    memcpy(temp_buffer, tempC, subm_dim * subm_dim * sizeof(int));
                }
                int start_row = compute_start_index(MATRIX_DIM + pad, coords[0], P);
                int start_column = compute_start_index(MATRIX_DIM + pad, coords[1], P);
                //copy temp into the matrix
                for (int r = 0; r < subm_dim; r++) {
                    for (int c = 0; c < subm_dim; c++) {
                        matrixC[(start_row + r) * total_dim + start_column + c] = temp_buffer[r * subm_dim + c];
                    }
                }


                free(temp_buffer);
            }

            #if PRINTS == 1
                printf("Matrix C:\n");
                print_matrix(matrixC, total_dim);
            #endif

            //store the result in a CSV file
            output_matrix_to_csv(matrixC, "matrixC.csv", total_dim);
            printf("Matrix C written to matrixC.csv\n");
            free(matrixC);

        }
    }
    
    MPI_Barrier(MPI_COMM_WORLD);

    // Finalize the performance logging
    // This will log the performance metrics to a CSV file
    if (rank < MAX_PROCESSES) {
        time_end_local = MPI_Wtime();
        getrusage(RUSAGE_SELF, &usage_end);
        
        user_time_local = (usage_end.ru_utime.tv_sec - usage_start.ru_utime.tv_sec) +
            (usage_end.ru_utime.tv_usec - usage_start.ru_utime.tv_usec) / 1e6;

        sys_time_local = (usage_end.ru_stime.tv_sec - usage_start.ru_stime.tv_sec) +
            (usage_end.ru_stime.tv_usec - usage_start.ru_stime.tv_usec) / 1e6;
        //print the resource usage
        elapsed_local = time_end_local - time_start_local;
        rss_local = usage_end.ru_maxrss; // Maximum resident set size in KB 
        ctx_switch_local = usage_end.ru_nivcsw; // Number of involuntary context switches

        printf("Process %d: User time: %.6f sec, System time: %.6f sec, Max RSS: %ld KB, Context switches: %ld\n",
            rank, user_time_local, sys_time_local, usage_end.ru_maxrss, usage_end.ru_nivcsw);
    
        double elapsed_max, user_time_max, sys_time_max;
        long rss_max, ctx_switch_max;   
        
        // Reduce the performance metrics across all processes
        // This will find the maximum values across all processes
        MPI_Reduce(&elapsed_local, &elapsed_max, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
        MPI_Reduce(&user_time_local, &user_time_max, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
        MPI_Reduce(&sys_time_local, &sys_time_max, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
        MPI_Reduce(&rss_local, &rss_max, 1, MPI_LONG, MPI_MAX, 0, MPI_COMM_WORLD);
        MPI_Reduce(&ctx_switch_local, &ctx_switch_max, 1, MPI_LONG, MPI_MAX, 0, MPI_COMM_WORLD);

        if (rank == 0)
        {
            
            FILE* f = fopen("performance_log.csv", "a");
            if (!f) {
                fprintf(stderr, "Error opening performance_log.csv for writing.\n");
                MPI_Abort(MPI_COMM_WORLD, 1);
            }
    
            // Write header if file is empty
            fseek(f, 0, SEEK_END);
            if (ftell(f) == 0) {
                fprintf(f, "processes,matrix size,elapsed_time_sec,user_cpu_sec,sys_cpu_sec,max_rss_kb,#context_switch\n");
            }
            fprintf(f, "%d,%d,%.6f,%.6f,%.6f,%ld,%ld\n",MAX_PROCESSES,MATRIX_DIM, elapsed_max, user_time_max, sys_time_max, rss_max, ctx_switch_max);
            fclose(f);
        }
        

    }

    MPI_Finalize();

    free(tempA);
    free(tempB);
    free(tempC);
    return 0;
}
