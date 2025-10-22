#!/bin/bash

# This script runs ChampSim on a specific list of traces provided in a file.

echo "🚀 Starting ChampSim simulation script for selected traces..."

# --- Configuration ---
# CHOOSE WHICH POLICY TO RUN BY UNCOMMENTING THE CORRECT BLOCK

# === BLOCK 1: Configuration for LRU ===
CHAMPSIM_BIN="./bin/champsim_test_LRU"
OUTPUT_DIR="./results/lru_policy/secret_traces"
POLICY_TYPE="LRU"

# === BLOCK 2: Configuration for PACIPV ===
# CHAMPSIM_BIN="./bin/champsim_test_PACIPV"
# OUTPUT_DIR="./results/pacipv_policy"
# POLICY_TYPE="PACIPV"


# --- Common Simulation Parameters ---
WARMUP_INST=25000000    # 25 Million instructions for warmup
SIM_INST=100000000      # 100 Million instructions for simulation

# --- Path and File Definitions ---
# IMPORTANT: This now points to the directory containing compute_fp, compute_int, etc.
BASE_TRACES_DIR="../traces/secret_traces"
# The file containing the list of traces to run.
TRACE_LIST_FILE="selected_secret_traces.txt"

# --- Pre-run Checks ---
if [ ! -x "$CHAMPSIM_BIN" ]; then
    echo "❌ Error: ChampSim binary not found or not executable at $CHAMPSIM_BIN"
    exit 1
fi
if [ ! -f "$TRACE_LIST_FILE" ]; then
    echo "❌ Error: Trace list file '$TRACE_LIST_FILE' not found."
    exit 1
fi

echo "📁 Creating output directory: $OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"
echo "⚙️  Running with policy: $POLICY_TYPE"

# --- Main Simulation Loop ---
# Reads the trace list file line by line.
while IFS= read -r trace_filename || [[ -n "$trace_filename" ]]; do
    
    # Determine the sub-folder based on the trace filename's prefix
    SUB_FOLDER=""
    if [[ "$trace_filename" == srv* ]]; then
        SUB_FOLDER="srv"
    elif [[ "$trace_filename" == compute_int* ]]; then
        SUB_FOLDER="compute_int"
    elif [[ "$trace_filename" == compute_fp* ]]; then
        SUB_FOLDER="compute_fp"
    elif [[ "$trace_filename" == crypto* ]]; then
        SUB_FOLDER="crypto"
    else
        echo "⚠️ Warning: Could not determine folder for '$trace_filename'. Skipping."
        continue
    fi

    full_trace_path="$BASE_TRACES_DIR/$SUB_FOLDER/$trace_filename"

    # Check if the full trace file path actually exists
    if [ -f "$full_trace_path" ]; then
        base_name=$(basename "$trace_filename" .champsimtrace.xz)
        output_file="$OUTPUT_DIR/${base_name}.txt"
        
        echo "▶️ Running simulation for: $base_name"

        # Execute the correct command based on the policy type
        if [ "$POLICY_TYPE" == "LRU" ]; then
            "$CHAMPSIM_BIN" --warmup-instructions $WARMUP_INST --simulation-instructions $SIM_INST "$full_trace_path" > "$output_file" 2>&1
            # echo "$CHAMPSIM_BIN --warmup-instructions $WARMUP_INST --simulation-instructions $SIM_INST "$full_trace_path" > "$output_file" 2>&1"
        elif [ "$POLICY_TYPE" == "PACIPV" ]; then
            echo "This is for PACIPV block"
        fi

        echo "✅ Finished. Results saved to: $output_file"
    else
        echo "❌ Error: Trace file not found at '$full_trace_path'. Skipping."
    fi

done < "$TRACE_LIST_FILE"

echo ""
echo "🎉 All simulations are complete!"