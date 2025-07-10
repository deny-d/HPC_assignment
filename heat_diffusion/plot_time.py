import matplotlib.pyplot as plt
import numpy as np
import sys
import os

if len(sys.argv) < 3:
    print("Usage: python plot_time.py <exec_time_file> <config_name>")
    print("Example for Config A: python plot_time.py exec_time_configA.txt configA")
    print("Example for Config B: python plot_time.py exec_time_configB.txt configB")
    sys.exit(1)

exec_time_file = sys.argv[1]
config_name = sys.argv[2]

output_dir = "graphs"
os.makedirs(output_dir, exist_ok=True)

print(f"Generating execution time plot for {config_name} from {exec_time_file}...")

#Check if the execution time file exists before proceeding
if os.path.exists(exec_time_file):
    try:
        data = np.loadtxt(exec_time_file, skiprows=1)
        num_threads = data[:, 0]
        exec_times = data[:, 1]

        # Plot Execution Time
        plt.figure(figsize=(10, 6))
        plt.plot(num_threads, exec_times, marker='o', linestyle='-')
        plt.xlabel('Number of Threads')
        plt.ylabel('Execution Time (seconds)')
        plt.title(f'Execution Time vs. Threads ({config_name})')
        plt.grid(True)
        plt.xticks(num_threads)
        plt.tight_layout()
        plt.savefig(os.path.join(output_dir, f'exec_time_{config_name}.png'))
        plt.close()
        print(f"Saved: exec_time_{config_name}.png")

        #Speedup vs. Numero di Thread
        if 1 in num_threads:
            time_seq = exec_times[num_threads == 1][0]
            speedup = time_seq / exec_times

            plt.figure(figsize=(10, 6))
            plt.plot(num_threads, speedup, marker='o', linestyle='-', color='green', label='Speedup')
            plt.plot(num_threads, num_threads, linestyle='--', color='red', label='Ideal Speedup')
            plt.xlabel('Number of Threads')
            plt.ylabel('Speedup')
            plt.title(f'Speedup vs. Threads ({config_name})')
            plt.grid(True)
            plt.xticks(num_threads)
            plt.legend()
            plt.tight_layout()
            plt.savefig(os.path.join(output_dir, f'speedup_{config_name}.png'))
            plt.close()
            print(f"Saved: speedup_{config_name}.png")
        else:
            print(f"Warning: Cannot calculate speedup for {config_name}. Data for 1 thread not found in {exec_time_file}.")

    except Exception as e:
        print(f"Error generating Execution Time/Speedup plot for {config_name}: {e}")
else:
    print(f"Skipping Execution Time plot for {config_name}: File not found ({exec_time_file}).")

print(f"Finished generating execution time plot for {config_name}.")
