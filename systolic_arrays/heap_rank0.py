import pandas as pd
import matplotlib.pyplot as plt

# Load the CSV
df = pd.read_csv("C:\\Users\\dipad\\Desktop\\massif_summary_complete.csv")


# Split data
rank0 = df[df['RANK'] == 0]
others = df[df['RANK'] != 0]

# Group by PROCS and MATRIX_SIZE
rank0_avg = rank0.groupby(['PROCS', 'MATRIX_SIZE'])['PEAK_HEAP_BYTES'].mean().reset_index()
others_avg = others.groupby(['PROCS', 'MATRIX_SIZE'])['PEAK_HEAP_BYTES'].mean().reset_index()

# Plot setup
plt.figure(figsize=(12, 7))

# Unique matrix sizes
matrix_sizes = sorted(df['MATRIX_SIZE'].unique())
colors = ['blue', 'green', 'red', 'orange', 'purple']

for idx, size in enumerate(matrix_sizes):
    r0 = rank0_avg[rank0_avg['MATRIX_SIZE'] == size]
    ot = others_avg[others_avg['MATRIX_SIZE'] == size]
    
    color = colors[idx % len(colors)]
    
    # Rank 0 line
    plt.plot(r0['PROCS'], r0['PEAK_HEAP_BYTES'], marker='o', linestyle='-', color=color,
             label=f'Matrix {size} - Rank 0')
    
    # Other ranks line
    plt.plot(ot['PROCS'], ot['PEAK_HEAP_BYTES'], marker='x', linestyle='--', color=color,
             label=f'Matrix {size} - Other Ranks')

# Labels and styling
plt.xlabel("Number of Threads")
plt.ylabel("Average Peak Heap (bytes)")
plt.title("Average Peak Heap Memory Usage vs Processes\n(Rank 0 vs Other Ranks)")
plt.grid(True, linestyle='--', alpha=0.5)
plt.legend()
plt.tight_layout()
plt.show()
