#!/bin/bash

# This script automates running ChampSim for a given set of trace files.
# It assumes it is being run from the root of the project directory (e.g., CS683-Project).

echo "🚀 Starting ChampSim simulation script..."

# --- Configuration ---
# You can change these simulation parameters if needed.
WARMUP_INST=30000000    # 30 Million instructions for warmup
SIM_INST=100000000      # 100 Million instructions for simulation

# --- Path Definitions ---
# Assumes the standard ChampSim project structure.
CHAMPSIM_BIN="./bin/champsim-pacipv"
TRACES_DIR="../traces/public_traces"
OUTPUT_DIR="./results/pacipv_policy/public_traces"

# --- Trace Categories ---
# An array of the directories containing the traces to run.
TRACE_SETS=("compute_int" "compute_fp" "srv")

# --- Pre-run Checks ---
# Check if the ChampSim binary exists and is executable.
if [ ! -x "$CHAMPSIM_BIN" ]; then
    echo "❌ Error: ChampSim binary not found or not executable at $CHAMPSIM_BIN"
    echo "Please build the project first using './build_champsim.sh' or similar."
    exit 1
fi

# Create the output directory. The '-p' flag prevents an error if it already exists.
echo "📁 Creating output directory: $OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"

# --- Main Simulation Loop ---
# Iterate over each trace set (e.g., compute_int, srv).
for set in "${TRACE_SETS[@]}"; do
    
    echo ""
    echo "================================================="
    echo "Processing trace set: $set"
    echo "================================================="
    
    # Define the full path to the current set of traces.
    current_trace_dir="$TRACES_DIR/$set"

    # Find all trace files in the current directory and loop through them.
    for trace_file in "$current_trace_dir"/*.champsimtrace.xz; do
        
        # Check if the file exists to avoid errors with empty directories.
        if [ -f "$trace_file" ]; then
            # Extract the base name of the trace file (e.g., 'compute_int_0').
            base_name=$(basename "$trace_file" .champsimtrace.xz)
            
            # Define the full path for the output file.
            output_file="$OUTPUT_DIR/${base_name}.txt"
            
            echo "▶️ Running simulation for: $base_name"
            
            # Execute the ChampSim command.
            # Redirects both standard output (stdout) and standard error (stderr) to the output file.
            # Run simulation with environment variables and nohup
            # env \
            # L1I_IPV="1_2_2_1_4#1_2_1_1_4" \
            # L1D_IPV="1_2_2_1_4#1_2_1_1_4" \
            # L2C_IPV="1_2_2_1_4#1_2_1_1_4" \
            # LLC_IPV="1_2_2_1_4#1_2_1_1_4" \
            # "$CHAMPSIM_BIN" \
            # --warmup-instructions "$WARMUP_INST" \
            # --simulation-instructions "$SIM_INST" \
            # "$trace_file" > "$output_file" 2>&1

            env \
            LLC_IPV="1_2_2_1_4#1_2_1_1_4" \
            "$CHAMPSIM_BIN" \
            --warmup-instructions "$WARMUP_INST" \
            --simulation-instructions "$SIM_INST" \
            "$trace_file" > "$output_file" 2>&1
            
            echo "✅ Finished. Results saved to: $output_file"
        fi
    done
done

echo ""
echo "🎉 All simulations are complete!"