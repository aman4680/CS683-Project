L1I_IPV=1_1_1_2_4#1_1_2_2_4 \
> L1D_IPV=1_1_1_2_4#1_1_2_2_4 \
> L2C_IPV=1_1_1_2_4#1_1_2_2_4 \
> LLC_IPV=1_1_1_2_4#1_1_2_2_4 \
> ./bin/champsim_test_PACIPV --warmup-instructions 200000000 --simulation-instructions 500000000 ../traces/public_traces/compute_int/compute_int_5.champsimtrace.xz



# OR 


In background
# For pacipv:
nohup env \
L1I_IPV=1_1_1_2_4#1_1_2_2_4 \
L1D_IPV=1_1_1_2_4#1_1_2_2_4 \
L2C_IPV=1_1_1_2_4#1_1_2_2_4 \
LLC_IPV=1_1_1_2_4#1_1_2_2_4 \
./bin/champsim_test_PACIPV --warmup-instructions 200000000 \
--simulation-instructions 500000000 ../traces/public_traces/compute_int/compute_int_5.champsimtrace.xz \
> results/output.txt 2>&1 &



# For duel:
nohup env \
L1I_IPV=1_1_1_2_4#1_1_2_2_4 \
L1D_IPV=1_1_1_2_4#1_1_2_2_4 \
L2C_IPV=1_1_1_2_4#1_1_2_2_4 \
LLC_IPV_INSTR=1_2_2_1_4#1_2_1_1_4 \
LLC_IPV_DATA=1_1_1_2_4#1_1_2_2_4 \
./bin/champsim_test_DUEL_IPV \
--warmup-instructions 200000000 \
--simulation-instructions 500000000 \
../traces/public_traces/compute_int/compute_int_5.champsimtrace.xz \
> results/output.txt 2>&1 &