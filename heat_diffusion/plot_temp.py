import os
import numpy as np
import matplotlib.pyplot as plt

#Global definitions
MAX_ITER = 10000
N = 1024
SAVE_MAP_INTERVAL = 1000
ITERATIONS = [0, MAX_ITER // 4, MAX_ITER // 2, MAX_ITER - 1]
T_HOT = {"A": 250.0, "B": 540.0}
T_AMBIENT = 15.0
LINE_INDEX = N // 2
OUTPUT_DIR = "graphs"
os.makedirs(OUTPUT_DIR, exist_ok=True)

def load_profile(config, axis, iteration):
    filename = f"temp_profile_config{config}_{axis}{LINE_INDEX}_iter{iteration}.txt"
    if not os.path.exists(filename):
        print(f"[WARNING] Missing file: {filename}")
        return None
    return np.loadtxt(filename, skiprows=1)

def plot_profiles(config):
    print(f"\nPlotting temperature profiles for configuration {config}...")

    # Row profile (center row)
    plt.figure(figsize=(10, 6))
    for i in ITERATIONS:
        data = load_profile(config, 'r', i)
        if data is not None:
            plt.plot(data[:, 0], data[:, 1], label=f'Iteration {i}')
    plt.title(f"Temperature Profile Along Center Row (Config {config})")
    plt.xlabel("Column Index")
    plt.ylabel("Temperature [°C]")
    plt.legend()
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(f"{OUTPUT_DIR}/temp_profile_row_config{config}.png")
    plt.close()
    print(f"Saved: temp_profile_row_config{config}.png")
    if config == "B":
        plt.figure(figsize=(10, 6))
        for i in ITERATIONS:
            col_data = load_profile(config, 'c', i)
            if col_data is not None:
                plt.plot(col_data[:, 0], col_data[:, 1], label=f'Iteration {i}')
        plt.title("Temperature Profile Along Center Column (Config B)")
        plt.xlabel("Row Index")
        plt.ylabel("Temperature [°C]")
        plt.legend()
        plt.grid(True)
        plt.tight_layout()
        plt.savefig(f"{OUTPUT_DIR}/temp_profile_col_config{config}.png")
        plt.close()
        print(f"Saved: temp_profile_col_config{config}.png")

    # Column + row for every iteration ( only for B)
    if config == "B":
        for i in ITERATIONS:
            row_data = load_profile(config, 'r', i)
            col_data = load_profile(config, 'c', i)
            if row_data is not None and col_data is not None:
                plt.figure(figsize=(10, 6))
                plt.plot(row_data[:, 0], row_data[:, 1], label='Center Row')
                plt.plot(col_data[:, 0], col_data[:, 1], label='Center Column')
                plt.title(f"Temperature Profiles at Iteration {i} (Config B)")
                plt.xlabel("Grid Index")
                plt.ylabel("Temperature [°C]")
                plt.legend()
                plt.grid(True)
                plt.tight_layout()
                plt.savefig(f"{OUTPUT_DIR}/temp_profile_row_col_configB_iter{i}.png")
                plt.close()
                print(f"Saved: temp_profile_row_col_configB_iter{i}.png")

def plot_center_temp(config):
    filename = f"point_evolution_config{config}.txt"
    if not os.path.exists(filename):
        print(f"[WARNING] Missing center temperature file: {filename}")
        return
    data = np.loadtxt(filename, skiprows=1)
    plt.figure(figsize=(10, 6))
    plt.plot(data[:, 0], data[:, 1])
    plt.title(f"Temperature Evolution at Grid Center (Config {config})")
    plt.xlabel("Iteration")
    plt.ylabel("Temperature [°C]")
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(f"{OUTPUT_DIR}/center_temperature_config{config}.png")
    plt.close()
    print(f"Saved: center_temperature_config{config}.png")

def plot_maps(config):
    print(f"Plotting temperature maps for configuration {config}...")
    for iter_num in range(SAVE_MAP_INTERVAL, MAX_ITER + 1, SAVE_MAP_INTERVAL):
        filename = f"temp_map_config{config}_iter{iter_num}.txt"
        if not os.path.exists(filename):
            print(f"[WARNING] Map file not found: {filename}")
            continue
        data = np.loadtxt(filename, skiprows=1)
        if len(data) != N * N:
            print(f"[ERROR] Incorrect data length in: {filename}")
            continue
        temp_map = data[:, 2].reshape((N, N))
        plt.figure(figsize=(8, 7))
        plt.imshow(temp_map, cmap='hot', origin='lower', extent=[0, N, 0, N], vmin=T_AMBIENT, vmax=T_HOT[config])
        plt.colorbar(label="Temperature [°C]")
        plt.title(f"Temperature Map (Config {config}, Iteration {iter_num})")
        plt.xlabel("X")
        plt.ylabel("Y")
        plt.tight_layout()
        out_file = f"{OUTPUT_DIR}/temp_map_config{config}_iter{iter_num}.png"
        plt.savefig(out_file)
        plt.close()
        print(f"Saved: {os.path.basename(out_file)}")

#esecution
for config in ["A", "B"]:
    plot_profiles(config)
    plot_center_temp(config)
    plot_maps(config)

print("\nAll plots generated and saved in the 'graphs' folder.")
