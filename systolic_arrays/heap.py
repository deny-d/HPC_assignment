import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.ticker import ScalarFormatter
from tabulate import tabulate

# Assuming df is your DataFrame with columns:
# 'MATRIX_SIZE', 'PROCS', 'PEAK_HEAP_BYTES'
df = pd.read_csv("C:\\Users\\dipad\\Desktop\\massif_summary_8cpu.csv")
# Group by MATRIX_SIZE and PROCS, compute average peak heap memory
avg_peak_memory = df.groupby(['MATRIX_SIZE', 'PROCS'])['PEAK_HEAP_BYTES'].mean().reset_index()
data_500 = avg_peak_memory[avg_peak_memory['MATRIX_SIZE'] == 500]
print(tabulate(data_500, headers='keys', tablefmt='grid'))
print(avg_peak_memory)
# Plot for each matrix size
sizes = sorted(avg_peak_memory['MATRIX_SIZE'].unique())
print("Unique Matrix Sizes:", sizes)
plt.figure(figsize=(10, 6))
for size in sizes:
    subset = avg_peak_memory[avg_peak_memory['MATRIX_SIZE'] == size]
    plt.plot(subset['PROCS'], subset['PEAK_HEAP_BYTES'], marker='o', label=f'Matrix Size {size}')

plt.xlabel('Number of Processes')
plt.ylabel('Average Peak Heap Memory per process (bytes)')
plt.title('Average Peak Heap Memory  per process vs Number of Processes')
plt.legend()
plt.grid(True)
plt.gca().yaxis.set_major_formatter(ScalarFormatter(useMathText=True))
plt.ticklabel_format(axis='y', style='sci', scilimits=(0,0))
plt.show()
