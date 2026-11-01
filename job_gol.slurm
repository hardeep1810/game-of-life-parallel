#!/bin/bash

#SBATCH --job-name=gol_scaling
#SBATCH --output=gol_%n_%j.out
#SBATCH --error=gol_%n_%j.err
#SBATCH -n 16
#SBATCH --time=00:20:00

module load game-of-life

echo "Running game of life with $SLURM_NTASKS processes"

for i in {1..5}
do 
	echo "Run $i:"
	# -f "%e" outputs only the 'Elapsed' real time in seconds
	{ time prun game_of_life -f input.txt -i 2000 -c 4096 -r 4096 ; } 2>&1
done 

echo "End Time: $(date)"
