import os
import re
import pandas as pd

def parse_champsim_output(file_path):
    """
    Parses a ChampSim output file and extracts specified parameters.
    Handles variations in cache naming (e.g., L1D vs. cpu0_L1D).

    Args:
        file_path (str): The path to the ChampSim output file.

    Returns:
        dict: A dictionary containing the extracted parameters, or None if file is empty.
    """
    try:
        with open(file_path, 'r') as f:
            content = f.read()
            if not content:
                print(f"Warning: File is empty, skipping: {file_path}")
                return None
    except FileNotFoundError:
        print(f"Error: File not found at {file_path}")
        return None

    # Helper function to safely find and convert values
    def get_value(pattern, text, group_num=1, value_type=float):
        match = re.search(pattern, text)
        if match:
            try:
                return value_type(match.group(group_num))
            except (ValueError, IndexError):
                return None
        return None

    # The number of simulation instructions is fixed
    instructions = 100000000

    # --- Overall Performance ---
    ipc = get_value(r"CPU 0 cumulative IPC: (\d+(?:\.\d+)?)", content)
    
    # --- Cache Misses (regex updated to make 'cpu0_' optional) ---
    llc_total_miss = get_value(r"LLC TOTAL\s+ACCESS:.*MISS:\s+(\d+)", content, value_type=int)
    llc_load_miss = get_value(r"LLC LOAD\s+ACCESS:.*MISS:\s+(\d+)", content, value_type=int)
    l2c_total_miss = get_value(r"(?:cpu0_)?L2C TOTAL\s+ACCESS:.*MISS:\s+(\d+)", content, value_type=int)
    l2c_load_miss = get_value(r"(?:cpu0_)?L2C LOAD\s+ACCESS:.*MISS:\s+(\d+)", content, value_type=int)
    l1d_total_miss = get_value(r"(?:cpu0_)?L1D TOTAL\s+ACCESS:.*MISS:\s+(\d+)", content, value_type=int)
    l1d_load_miss = get_value(r"(?:cpu0_)?L1D LOAD\s+ACCESS:.*MISS:\s+(\d+)", content, value_type=int)
    l1i_total_miss = get_value(r"(?:cpu0_)?L1I TOTAL\s+ACCESS:.*MISS:\s+(\d+)", content, value_type=int)
    l1i_load_miss = get_value(r"(?:cpu0_)?L1I LOAD\s+ACCESS:.*MISS:\s+(\d+)", content, value_type=int)

    # --- Average Miss Latencies (regex updated) ---
    # llc_latency = get_value(r"LLC AVERAGE MISS LATENCY: (\d+(?:\.\d+)?)", content)
    llc_latency = get_value(r"LLC AVERAGE MISS LATENCY: (\d+(?:\.\d+)?)", content)
    l2c_latency = get_value(r"(?:cpu0_)?L2C AVERAGE MISS LATENCY: (\d+(?:\.\d+)?)", content)
    l1d_latency = get_value(r"(?:cpu0_)?L1D AVERAGE MISS LATENCY: (\d+(?:\.\d+)?)", content)
    l1i_latency = get_value(r"(?:cpu0_)?L1I AVERAGE MISS LATENCY: (\d+(?:\.\d+)?)", content)

    # --- DRAM Statistics ---
    rq_misses = get_value(r"ROW_BUFFER_MISS:\s+(\d+)", content, value_type=int)
    wq_full_cycles = get_value(r"FULL:\s+(\d+)", content, value_type=int)
    
    # Helper for MPKI calculation
    def calculate_mpki(misses):
        if misses is not None:
            return (misses / instructions) * 1000
        return None

    # Assemble the dictionary of results
    return {
        "Trace": os.path.basename(file_path),
        "IPC": ipc,
        "LLC Total MPKI": calculate_mpki(llc_total_miss),
        "LLC Load MPKI": calculate_mpki(llc_load_miss),
        "LLC Latency": llc_latency,
        "L2C Total MPKI": calculate_mpki(l2c_total_miss),
        "L2C Load MPKI": calculate_mpki(l2c_load_miss),
        "L2C Latency": l2c_latency,
        "L1D Total MPKI": calculate_mpki(l1d_total_miss),
        "L1D Load MPKI": calculate_mpki(l1d_load_miss),
        "L1D Latency": l1d_latency,
        "L1I Total MPKI": calculate_mpki(l1i_total_miss),
        "L1I Load MPKI": calculate_mpki(l1i_load_miss),
        "L1I Latency": l1i_latency,
        "RQ Misses": rq_misses,
        "WQ Full cycles": wq_full_cycles,
    }

# --- Main Execution Logic ---
if __name__ == "__main__":
    # --- IMPORTANT: Change this to the directory containing your new result files ---
    results_dir = "./results/pacipv_policy/secret_traces/"  
    all_data = []

    # Check if the directory exists
    if not os.path.isdir(results_dir):
        print(f"Error: Directory not found at '{results_dir}'")
        print("Please create this directory and place your simulation output files in it.")
        exit()

    # Iterate over all files in the specified directory
    for filename in sorted(os.listdir(results_dir)):
        if filename.endswith((".txt", ".out", ".log")): # Handles common log file extensions
            file_path = os.path.join(results_dir, filename)
            data = parse_champsim_output(file_path)
            if data:
                all_data.append(data)

    # Convert the list of dictionaries to a pandas DataFrame and save to CSV
    if all_data:
        df = pd.DataFrame(all_data)
        # Set the order of columns for the CSV file
        column_order = [
            "Trace", "IPC", "LLC Total MPKI", "LLC Load MPKI", "LLC Latency",
            "L2C Total MPKI", "L2C Load MPKI", "L2C Latency", "L1D Total MPKI",
            "L1D Load MPKI", "L1D Latency", "L1I Total MPKI", "L1I Load MPKI",
            "L1I Latency", "RQ Misses", "WQ Full cycles"
        ]
        # Rename "LRU IPC" to just "IPC" for general use
        df.rename(columns={"LRU IPC": "IPC"}, inplace=True)
        df = df[column_order]
        
        output_csv_path = "secret_traces_pacipv_policy_stats.csv"
        df.to_csv(output_csv_path, index=False, float_format='%.6f')
        print(f"Successfully created CSV file: '{output_csv_path}'")
        print("\n--- First 5 rows of the generated data ---")
        print(df.head())
    else:
        print(f"No valid data files found or parsed in '{results_dir}'. The CSV file was not created.")