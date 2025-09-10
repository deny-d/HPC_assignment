import pandas as pd
import matplotlib.pyplot as plt
from tabulate import tabulate

# Load your data
df = pd.read_csv("C:\\Users\\dipad\\Desktop\\performance_log_8cpu.csv")
# Group by matrix size and number of processes, then compute mean
grouped = df.groupby(['matrix_size', 'processes']).mean().reset_index()
data_500 = grouped[grouped['matrix_size'] == 500]
print(tabulate(data_500, headers='keys', tablefmt='grid'))

# Metrics to plot
metrics = {
    'elapsed_time_sec': 'Elapsed Time (sec)',
    'user_cpu_sec': 'User CPU Time (sec)',
    'sys_cpu_sec': 'System CPU Time (sec)',
    'max_rss_kb': 'Max RSS (KB)',
    '#context_switch': 'Context Switches'
}

# Get unique matrix sizes
matrix_sizes = grouped['matrix_size'].unique()

# Generate one plot per metric
for metric, label in metrics.items():
    plt.figure(figsize=(10, 6))
    for size in matrix_sizes:
        data = grouped[grouped['matrix_size'] == size]
        plt.plot(data['processes'], data[metric], marker='o', label=size)
    
    plt.title(f'{label} vs Number of Processes')
    plt.xlabel('Number of Processes')
    plt.ylabel(f'Average {label}')
    plt.legend(title='Matrix Size')
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(f'{metric}_vs_processes.png')
    plt.show()
