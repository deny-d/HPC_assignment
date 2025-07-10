#!/bin/bash
#SBATCH -J Heat_Plots
#SBATCH --time=0:05:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=32
#SBATCH --output=heat_plots_%j.out
#SBATCH --error=heat_plots_%j.err
#SBATCH --mail-type=END,FAIL

echo "Starting Heat Diffusion Simulation Plotting Job"
echo "Job ID: $SLURM_JOB_ID"
echo "Run on host: $(hostname)"
echo "Current directory: $(pwd)"

HEAT_EXEC="./heat"
OPTIMAL_THREADS=32

echo "--- Processing Configuration A (Isotropic Diffusion) ---"
echo "Running Config A with $OPTIMAL_THREADS threads to save temperature data and generate plots..."
$HEAT_EXEC "a" "temp" "$OPTIMAL_THREADS"
if [ $? -ne 0 ]; then
    echo "Error running heat executable for Config A. Check C code for plot generation errors."
fi


echo "--- Processing Configuration B (Anisotropic Diffusion) ---"
echo "Running Config B with $OPTIMAL_THREADS threads to save temperature data and generate plots..."
$HEAT_EXEC "b" "temp" "$OPTIMAL_THREADS"
if [ $? -ne 0 ]; then
    echo "Error running heat executable for Config B. Check C code for plot generation errors."
fi


echo "Job finished."