#!/bin/bash
#SBATCH -J Heat_Timing
#SBATCH --time=0:15:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=32
#SBATCH --exclusive
#SBATCH --output=heat_timing_%j.out
#SBATCH --error=heat_timing_%j.err
#SBATCH --mail-type=END,FAIL

echo "Starting Heat Diffusion Simulation Timing Job"
echo "Job ID: $SLURM_JOB_ID"
echo "Run on host: $(hostname)"
echo "Current directory: $(pwd)"

HEAT_EXEC="./heat"
THREAD_COUNTS=(1 2 4 6 8 10 12 14 16 18 20 22 24 26 28 30 32)
CONFIGS=("a" "b")

rm -f exec_time_configA.txt
rm -f exec_time_configB.txt

for config in "${CONFIGS[@]}"; do
    echo "--- Testing Configuration: $config ---"
    
    for num_threads in "${THREAD_COUNTS[@]}"; do
        echo "Running config $config with $num_threads threads for timing..."
        $HEAT_EXEC "$config" "time" "$num_threads"
        if [ $? -ne 0 ]; then
            echo "Error running heat executable for config $config with $num_threads threads."
        fi
        echo ""
    done
    echo "Timing results for config $config completed"
done

echo "Job finished."
