# import os
# import re
# import matplotlib.pyplot as plt
# import numpy as np

# # Dictionary to hold: { num_processes: [list_of_5_times] }
# data = {}

# # Regex patterns
# # Finds the process number from the folder name (e.g., 'process64' -> 64)
# folder_pattern = re.compile(r"process(\d+)")
# # Finds 'real 0m12.345s' or 'real 12.345'
# time_pattern = re.compile(r"real\s+(?:(\d+)m)?(\d+\.\d+)s?", re.IGNORECASE)

# print("Scanning subdirectories for scaling data...")

# # 1. WALK THROUGH ALL FOLDERS
# for root, dirs, files in os.walk('.'):
#     folder_name = os.path.basename(root)
#     match = folder_pattern.search(folder_name)
    
#     if match:
#         n_proc = int(match.group(1))
        
#         for filename in files:
#             if filename.endswith(".out"):
#                 file_path = os.path.join(root, filename)
                
#                 with open(file_path, 'r') as f:
#                     content = f.read()
#                     # Find all 5 time measurements in the file
#                     time_matches = time_pattern.findall(content)
                    
#                     if time_matches:
#                         if n_proc not in data:
#                             data[n_proc] = []
                        
#                         for m_min, m_sec in time_matches:
#                             minutes = float(m_min) if m_min else 0
#                             seconds = float(m_sec)
#                             total_seconds = (minutes * 60) + seconds
#                             data[n_proc].append(total_seconds)
                        
#                         print(f"✅ Found {len(time_matches)} runs for {n_proc} processes in {file_path}")

# # 2. CALCULATE SPEEDUP
# if not data:
#     print("\n[ERROR] No data found. Ensure you are running this in the 'Assignment_02' folder.")
# else:
#     sorted_procs = sorted(data.keys())
#     # Calculate averages for each process count
#     avg_times = [np.mean(data[p]) for p in sorted_procs]
    
#     # Baseline is the time for 1 process
#     if 1 not in data:
#         print(f"Warning: No data for process 1 found. Available: {sorted_procs}")
#         t1 = avg_times[0] # Fallback to smallest available
#     else:
#         t1 = avg_times[sorted_procs.index(1)]
    
#     speedups = [t1 / tn for tn in avg_times]

#     # 3. PRINT TABLE
#     print(f"\n{'Processes':<10} | {'Avg Time (s)':<12} | {'Speedup':<8}")
#     print("-" * 38)
#     for p, t, s in zip(sorted_procs, avg_times, speedups):
#         print(f"{p:<10} | {t:<12.3f} | {s:<8.2f}")

#     # 4. GENERATE PLOT
#     plt.figure(figsize=(10, 6))
#     plt.plot(sorted_procs, sorted_procs, 'r--', alpha=0.6, label='Ideal Speedup (Linear)')
#     plt.plot(sorted_procs, speedups, 'bo-', linewidth=2, markersize=8, label='Measured Speedup')
    
#     plt.xlabel('Number of Processes ($n$)', fontsize=12)
#     plt.ylabel('Speedup ($S_n = T_1 / T_n$)', fontsize=12)
#     plt.title('Game of Life Strong Scaling (4096 x 4096)', fontsize=14)
#     plt.legend()
#     plt.grid(True, which="both", ls="-", alpha=0.3)
#     plt.xticks(sorted_procs)
    
#     plt.savefig('assignment_02_speedup.png')
#     print("\nGraph saved as 'assignment_02_speedup.png'")

import os
import re
import csv
import matplotlib.pyplot as plt
import numpy as np

# Dictionary to hold: { num_processes: [list_of_5_times] }
data = {}

# Regex patterns
folder_pattern = re.compile(r"process(\d+)")
time_pattern = re.compile(r"real\s+(?:(\d+)m)?(\d+\.\d+)s?", re.IGNORECASE)

print("Scanning subdirectories for scaling data...")

# 1. SCAN FOLDERS AND COLLECT RAW DATA
for root, dirs, files in os.walk('.'):
    folder_name = os.path.basename(root)
    match = folder_pattern.search(folder_name)
    
    if match:
        n_proc = int(match.group(1))
        for filename in files:
            if filename.endswith(".out"):
                file_path = os.path.join(root, filename)
                with open(file_path, 'r') as f:
                    content = f.read()
                    time_matches = time_pattern.findall(content)
                    if time_matches:
                        if n_proc not in data:
                            data[n_proc] = []
                        for m_min, m_sec in time_matches:
                            minutes = float(m_min) if m_min else 0
                            seconds = float(m_sec)
                            data[n_proc].append((minutes * 60) + seconds)

# 2. SAVE RAW DATA TO CSV
if data:
    sorted_procs = sorted(data.keys())
    with open('data.csv', 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        # Header: Processes, Run1, Run2, Run3, Run4, Run5
        writer.writerow(['Processes', 'Run 1', 'Run 2', 'Run 3', 'Run 4', 'Run 5'])
        
        for p in sorted_procs:
            # Writes the process count followed by all timing points collected for it
            writer.writerow([p] + data[p])
    print("✅ Raw data saved to 'data.csv'")

# 3. CALCULATION & PLOTTING
if not data:
    print("\n[ERROR] No data found. Ensure you are running this in 'Assignment_02'.")
else:
    avg_times = [np.mean(data[p]) for p in sorted_procs]
    t1 = avg_times[sorted_procs.index(1)] if 1 in data else avg_times[0]
    speedups = [t1 / tn for tn in avg_times]

    # Print Table to console
    print(f"\n{'Processes':<10} | {'Avg Time (s)':<12} | {'Speedup':<8}")
    print("-" * 38)
    for p, t, s in zip(sorted_procs, avg_times, speedups):
        print(f"{p:<10} | {t:<12.3f} | {s:<8.2f}")

    # Generate Plot
    plt.figure(figsize=(10, 6))
    plt.plot(sorted_procs, sorted_procs, 'r--', alpha=0.6, label='Ideal Speedup')
    plt.plot(sorted_procs, speedups, 'bo-', linewidth=2, label='Measured Speedup')
    plt.xlabel('Number of Processes ($n$)')
    plt.ylabel('Speedup ($T_1 / T_n$)')
    plt.title('Game of Life Strong Scaling')
    plt.legend()
    plt.grid(True, alpha=0.3)
    plt.savefig('assignment_02_speedup.png')
    print("✅ Graph saved as 'assignment_02_speedup.png'")