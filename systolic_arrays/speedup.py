import csv
from collections import defaultdict
import matplotlib.pyplot as plt

def read_data(filename):
    data = []
    with open(filename, newline='') as csvfile:
        reader = csv.DictReader(csvfile)
        for row in reader:
            data.append({
                'processes': int(row['processes']),
                'matrix_size': int(row['matrix_size']),
                'elapsed_time_sec': float(row['elapsed_time_sec']),
                # Removed user_cpu_sec, sys_cpu_sec, max_rss_kb, context_switch since not needed
            })
    return data

def mean(values):
    return sum(values) / len(values) if values else 0

def analyze(data):
    groups = defaultdict(list)
    for entry in data:
        key = (entry['processes'], entry['matrix_size'])
        groups[key].append(entry)
    
    results = {}
    for key, entries in groups.items():
        processes, matrix_size = key
        elapsed_times = [e['elapsed_time_sec'] for e in entries]

        mean_elapsed = mean(elapsed_times)

        results[key] = {
            'mean_elapsed': mean_elapsed,
            'processes': processes,
            'matrix_size': matrix_size
        }
    
    output = []
    for (proc, size), stats in results.items():
        baseline = results.get((1, size))
        if baseline:
            speedup = baseline['mean_elapsed'] / stats['mean_elapsed'] if stats['mean_elapsed'] > 0 else 0
            efficiency = speedup / proc if proc > 0 else 0
            print(f"Processes: {proc}, Matrix Size: {size}, Speedup: {speedup:.2f}, Efficiency: {efficiency:.2f}")
        else:
            speedup = None
            efficiency = None

        output.append({
            'processes': proc,
            'matrix_size': size,
            'speedup': speedup,
            'efficiency': efficiency,
        })

    return output

def plot_metrics(data):
    # Organize data by matrix size
    sizes = sorted(set(d['matrix_size'] for d in data))
    metrics = ['speedup', 'efficiency']
    colors = ['b', 'g', 'r', 'c', 'm', 'y', 'k']

    fig, axs = plt.subplots(2, 1, figsize=(10, 10))
    
    for i, metric in enumerate(metrics):
        ax = axs[i]
        for idx, size in enumerate(sizes):
            subset = [d for d in data if d['matrix_size'] == size]
            subset.sort(key=lambda x: x['processes'])

            procs = [d['processes'] for d in subset]
            values = [d[metric] for d in subset]

            ax.plot(procs, values, marker='o', label=f'Matrix size: {size}', color=colors[idx % len(colors)])

        ax.set_xlabel('Number of processes')
        ax.set_ylabel(metric.replace('_', ' ').title())
        ax.grid(True, which="both", ls="--")
        ax.legend()
        ax.set_title(f'{metric.replace("_", " ").title()} vs Number of Processes')

    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    filename = "C:\\Users\\dipad\\Desktop\\performance_log_8cpu.csv"
    data = read_data(filename)
    analyzed = analyze(data)
    plot_metrics(analyzed)