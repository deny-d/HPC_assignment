#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include <string.h>

// Global definitions
#define N 1024       // Grid size NxN
#define MAX_ITER 10000 // Maximum number of iterations
#define T_HOT_A 250.0  // Initial hot temperature for Configuration A [°C]
#define T_HOT_B 540.0  // Initial hot temperature for Configuration B [°C]
#define T_AMBIENT 15.0 // Initial ambient/cold temperature [°C]
#define WX 0.3         // Diffusion coefficient in X-direction (for anisotropy)
#define WY 0.2         // Diffusion coefficient in Y-direction (for anisotropy)


//Functions definitions

// Allocates and returns a 2D grid of size NxN
double **allocate_grid() {
    double **grid = malloc(N * sizeof(double *));
    if (grid == NULL) { perror("Failed to allocate grid rows"); exit(EXIT_FAILURE); }
    for (int i = 0; i < N; i++) {
        grid[i] = malloc(N * sizeof(double));
        if (grid[i] == NULL) { perror("Failed to allocate grid columns"); exit(EXIT_FAILURE); }
    }
    return grid;
}

// Frees the memory allocated for the grid
void free_grid(double **grid) {
    if (grid == NULL) return;
    for (int i = 0; i < N; i++) {
        free(grid[i]);
    }
    free(grid);
}

// Copies the contents of the source grid to the destination grid
// This function copies the values of one grid into another.
// It is used to save the current grid state before applying updates,
// so that the updates do not affect each other during the loop.
void copy_grid(double **src, double **dest) {
    #pragma omp parallel for collapse(2)
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            dest[i][j] = src[i][j];
}

// Initializes Configuration A: half hot, half ambient
void init_config_a(double **grid) {
    #pragma omp parallel for collapse(2)
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (j < N / 2) { 
                grid[i][j] = T_HOT_A;
            } else { 
                grid[i][j] = T_AMBIENT;
            }
        }
    }
}

// Initializes Configuration B: central hot square, rest ambient
void init_config_b(double **grid) {
    int start = N / 4;
    int end = 3 * N / 4;

    #pragma omp parallel for collapse(2)
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (i >= start && i < end && j >= start && j < end) {
                grid[i][j] = T_HOT_B;
            } else {
                grid[i][j] = T_AMBIENT;
            }
        }
    }
}
 
// Saves the temperature profile along a specific row or column
void save_temperature_profile(double **grid, int iteration, const char *config_name, char axis, int line_index) {
    char filename[200];
    sprintf(filename, "temp_profile_%s_%c%d_iter%d.txt", config_name, axis, line_index, iteration);
    FILE *f = fopen(filename, "w"); 
    if (f == NULL) {
        perror("Error opening file for temperature profile");
        return;
    }

    if (axis == 'r') { // Profile for row
        fprintf(f, "Column_Index Temperature\n");
        for (int j = 0; j < N; j++) {
            fprintf(f, "%d %.4f\n", j, grid[line_index][j]);
        }
    } else if (axis == 'c') { // Profile for column
        fprintf(f, "Row_Index Temperature\n");
        for (int i = 0; i < N; i++) {
            fprintf(f, "%d %.4f\n", i, grid[i][line_index]);
        }
    }
    
    fclose(f);
    printf("Temperature profile for %s, %c%d at iteration %d saved to %s\n", config_name, axis, line_index, iteration, filename);
}

// Saves the temperature of the center point of the grid
void save_specific_points_evolution(FILE *f, double **grid, int iter) {
    fprintf(f, "%d %.4f\n", iter, grid[N/2][N/2]);
}


// Saves a complete temperature map of the grid
void save_temperature_map(double **grid, int iteration, const char *config_name) {
    char filename[100];
    sprintf(filename, "temp_map_%s_iter%d.txt", config_name, iteration);
    FILE *f = fopen(filename, "w"); 
    if (f == NULL) {
        perror("Error opening file for temperature map");
        return;
    }
    fprintf(f, "Row Column Temperature\n");
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            fprintf(f, "%d %d %.4f\n", i, j, grid[i][j]);
        }
    }
    fclose(f);
    printf("Temperature map for %s at iteration %d saved to %s\n", config_name, iteration, filename);
}

// Saves the execution time for a given configuration and number of threads
void save_execution_time(FILE *f, double exec_time, int num_threads) {
    fprintf(f, "%d %.4f\n", num_threads, exec_time);
    printf("Execution time with %d threads saved.\n", num_threads);
}

// Simulates isotropic heat diffusion
void simulate_isotropic(double **grid, double **new_grid, FILE *point_file, FILE *exec_file, const char *config_name, int save_temp, int save_time, int num_threads) {
     
    // Start timer
    double start_time = omp_get_wtime();
   
    for (int iter = 0; iter < MAX_ITER; iter++) {
        copy_grid(grid, new_grid); // Copy entire grid (including boundaries)

        #pragma omp parallel for collapse(2)
        for (int i = 1; i < N - 1; i++) { // Loop for internal cells
            for (int j = 1; j < N - 1; j++) {
                new_grid[i][j] = 0.25 * (grid[i+1][j] + grid[i-1][j] + grid[i][j+1] + grid[i][j-1]);
            }
        }
        
        copy_grid(new_grid, grid); // Update the main grid
        
        if (save_temp) {
            
            // Saves temperature profiles at specific iterations (0, MAX_ITER/4, MAX_ITER/2, MAX_ITER-1)
            if (iter == 0 || iter == MAX_ITER / 4 || iter == MAX_ITER / 2 || iter == MAX_ITER - 1) {
                save_temperature_profile(grid, iter, config_name, 'r', N/2);
            }
            // Saves evolution of the central point (at the 0 iteration and then every 100 iterations)
            if (iter % 100 == 0) { 
                save_specific_points_evolution(point_file, grid, iter);
            }

            // Save temperature map (every 1000 iterations)
            if ((iter + 1) % 1000 == 0) {
                save_temperature_map(grid, iter + 1, config_name);
            }
        }
    }
       
    // Stop timer after computation steps
    double end_time = omp_get_wtime(); 
    double exec_time = end_time - start_time; 
    if (save_time) {
            // Saves execution time
            save_execution_time(exec_file, exec_time, num_threads); 
        }
    
}

// Simulates anisotropic heat diffusion
void simulate_anisotropic(double **grid, double **new_grid, FILE *point_file, FILE *exec_file, const char *config_name, int save_temp, int save_time, int num_threads) {
    
    // Start timer
    double start_time = omp_get_wtime();
    
    for (int iter = 0; iter < MAX_ITER; iter++) {
        copy_grid(grid, new_grid);

        #pragma omp parallel for collapse(2)
        for (int i = 1; i < N - 1; i++) {
            for (int j = 1; j < N - 1; j++) {
                new_grid[i][j] = WX * (grid[i][j-1] + grid[i][j+1]) + WY * (grid[i-1][j] + grid[i+1][j]);
            }
        }

        copy_grid(new_grid, grid);

        if (save_temp) {
            
            // Save temperature profiles at specific iterations (0, MAX_ITER/4, MAX_ITER/2, MAX_ITER-1)
            // For Configuration B: Both central row and central column
            if (iter == 0 || iter == MAX_ITER / 4 || iter == MAX_ITER / 2 || iter == MAX_ITER - 1) {
                save_temperature_profile(grid, iter, config_name, 'r', N/2); 
                save_temperature_profile(grid, iter, config_name, 'c', N/2);
            }

            // Saves evolution of specific point at 0 iteration and then every 100 iterations
            if (iter % 100 == 0) { 
                save_specific_points_evolution(point_file, grid, iter);
            }

            // Save temperature map (every 1000 iterations)
            if ((iter + 1) % 1000 == 0) {
                save_temperature_map(grid, iter + 1, config_name); 
            }
        }
    }
    // Stop timer after computation steps
    double end_time = omp_get_wtime(); 
    double exec_time = end_time - start_time; 
    if (save_time) {
            // Saves execution time
            save_execution_time(exec_file, exec_time, num_threads); 
        }

} 

// 

int main(int argc, char *argv[]) {
    // Command line argument parsing
    if (argc != 4) {
        fprintf(stderr, "Usage: %s [a|b|both] [temp|time] [num_threads]\n", argv[0]);
        return 1;
    }

    int run_config_A = (strcmp(argv[1], "a") == 0 || strcmp(argv[1], "both") == 0);
    int run_config_B = (strcmp(argv[1], "b") == 0 || strcmp(argv[1], "both") == 0);
    int save_temp_data = (strcmp(argv[2], "temp") == 0); // Flag to save temperature-related data files
    int save_time_data = (strcmp(argv[2], "time") == 0); // Flag to save execution time files

    int num_threads = atoi(argv[3]);
    if (num_threads <= 0) {
        fprintf(stderr, "Error: number of threads must be a positive integer.\n");
        return 1;
    }
    omp_set_num_threads(num_threads);

    printf("Simulation started with %d threads.\n", num_threads);

    //Execute Configuration A
    if (run_config_A) {
        double **grid_a = allocate_grid();
        double **new_grid_a = allocate_grid();
        
        FILE *point_file_a = NULL;
        FILE *exec_file_a = NULL;

        if (save_temp_data) {
            
            point_file_a = fopen("point_evolution_configA.txt", "w"); 
            if (point_file_a == NULL) { 
                perror("Error opening point_evolution_configA.txt"); 
                free_grid(grid_a); 
                free_grid(new_grid_a);
                return 1;
            }
            fprintf(point_file_a, "Iteration Temp_Center\n");
            
        }

        if (save_time_data) {
            
            exec_file_a = fopen("exec_time_configA.txt", "a");
            if (exec_file_a == NULL) {
             perror("Error opening exec_time_configA.txt");
             free_grid(grid_a);
             free_grid(new_grid_a);
             return 1;
           }
           fseek(exec_file_a, 0, SEEK_END);
           if (ftell(exec_file_a) == 0) {
           fprintf(exec_file_a, "Num_Threads Execution_Time_Seconds\n");
           }
            
       }

        init_config_a(grid_a);

        printf("\nStarting configuration A (isotropic diffusion)\n");
        simulate_isotropic(grid_a, new_grid_a, point_file_a, exec_file_a, "configA", save_temp_data, save_time_data, num_threads);

        if (save_temp_data) {
            if (point_file_a) fclose(point_file_a);
        }

        if (save_time_data) {
            if (exec_file_a) fclose(exec_file_a);
        }

        free_grid(grid_a);
        free_grid(new_grid_a);
    }

    //Execute Configuration B
    if (run_config_B) {
        double **grid_b = allocate_grid();
        double **new_grid_b = allocate_grid();
        
        FILE *point_file_b = NULL;
        FILE *exec_file_b = NULL;

        if (save_temp_data) {
            
            point_file_b = fopen("point_evolution_configB.txt", "w"); 
            if (point_file_b == NULL) { 
                perror("Error opening point_evolution_configB.txt"); 
                free_grid(grid_b); free_grid(new_grid_b); 
                return 1;
            }
            fprintf(point_file_b, "Iteration Temp_Center\n");
        }
        if (save_time_data) {
            
            exec_file_b = fopen("exec_time_configB.txt", "a");
            if (exec_file_b == NULL) {
             perror("Error opening exec_time_configB.txt");
             free_grid(grid_b);
             free_grid(new_grid_b);
             return 1;
           }
           fseek(exec_file_b, 0, SEEK_END);
           if (ftell(exec_file_b) == 0) {
           fprintf(exec_file_b, "Num_Threads Execution_Time_Seconds\n");
           } 
       }


        init_config_b(grid_b);

        printf("\nStarting configuration B (anisotropic diffusion)\n");
        simulate_anisotropic(grid_b, new_grid_b, point_file_b, exec_file_b, "configB", save_temp_data, save_time_data, num_threads);
    
        if (save_temp_data) {
            if (point_file_b) fclose(point_file_b);
        }

        if (save_time_data) {
            if (exec_file_b) fclose(exec_file_b);
        }

        free_grid(grid_b);
        free_grid(new_grid_b);
    }

    return 0;
}
