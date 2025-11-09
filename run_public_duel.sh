#!/bin/bash

# This script automates running ChampSim for a given set of trace files.
# It assumes it is being run from the root of the project directory (e.g., CS683-Project).

echo "🚀 Starting ChampSim simulation script..."

# --- Configuration ---
WARMUP_INST=30000000    # 30 Million instructions for warmup
SIM_INST=100000000      # 100 Million instructions for simulation

# --- Path Definitions ---
CHAMPSIM_BIN="./bin/champsim-duel-ipv"
TRACES_DIR="../traces/public_traces"
OUTPUT_DIR="./results/duel_policy/public_traces"

# --- Trace Categories ---
TRACE_SETS=("compute_int" "compute_fp" "srv")

# --- Pre-run Checks ---
if [ ! -x "$CHAMPSIM_BIN" ]; then
    echo "❌ Error: ChampSim binary not found or not executable at $CHAMPSIM_BIN"
    echo "Please build the project first using './build_champsim.sh' or similar."
    exit 1
fi

# Create the base output directory
echo "📁 Creating base output directory: $OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"

# --- Main Simulation Loop ---
for set in "${TRACE_SETS[@]}"; do
    
    echo ""
    echo "================================================="
    echo "Processing trace set: $set"
    echo "================================================="

    current_trace_dir="$TRACES_DIR/$set"
    current_output_dir="$OUTPUT_DIR/$set"

    # Create subdirectory for this trace set
    echo "📁 Creating output directory for $set: $current_output_dir"
    mkdir -p "$current_output_dir"

    # Loop through trace files in this set
    for trace_file in "$current_trace_dir"/*.champsimtrace.xz; do
        
        if [ -f "$trace_file" ]; then
            base_name=$(basename "$trace_file" .champsimtrace.xz)
            output_file="$current_output_dir/${base_name}.txt"

            echo "▶️ Running simulation for: $base_name"
            
            # Run ChampSim with environment variables
            # env \
            #     L1I_IPV="1_1_1_2_4#1_1_2_2_4" \
            #     L1D_IPV="1_1_1_2_4#1_1_2_2_4" \
            #     L2C_IPV="1_1_1_2_4#1_1_2_2_4" \
            #     LLC_IPV_INSTR="1_2_2_1_4#1_2_1_1_4" \
            #     LLC_IPV_DATA="1_1_1_2_4#1_1_2_2_4" \
            #     "$CHAMPSIM_BIN" \
            #     --warmup-instructions "$WARMUP_INST" \
            #     --simulation-instructions "$SIM_INST" \
            #     "$trace_file" > "$output_file" 2>&1 &

            env \
                LLC_IPV_INSTR="1_2_2_1_4#1_2_1_1_4" \
                LLC_IPV_DATA="1_1_1_2_4#1_1_2_2_4" \
                "$CHAMPSIM_BIN" \
                --warmup-instructions "$WARMUP_INST" \
                --simulation-instructions "$SIM_INST" \
                "$trace_file" > "$output_file" 2>&1 &
                
            echo "✅ Finished duel_ipv simulation. Results saved to: $output_file"
        fi
    done
done

echo ""
echo "🎉 All simulations are complete!"

# nohup ./run_public_duel.sh > output_logs/duel_public_trace.log 2>&1 &