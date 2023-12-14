#include <iostream>
#include <stdlib.h>
#include <time.h>

#include "cache.h"

void CACHE::initialize_replacement()
{
	srand(time(NULL));	// See the PRNG
}

uint32_t CACHE::find_victim(
		[[maybe_unused]] uint32_t triggering_cpu,
		[[maybe_unused]] uint64_t instr_id,
		[[maybe_unused]] uint32_t set,
		[[maybe_unused]] const BLOCK* current_set,
		[[maybe_unused]] uint64_t ip,
		[[maybe_unused]] uint64_t full_addr,
		[[maybe_unused]] uint32_t type
		)
{
	// Randomly pick a victim
	return (uint32_t)(rand() % NUM_WAY);
}

void CACHE::update_replacement_state(
		[[maybe_unused]] uint32_t triggering_cpu,
		[[maybe_unused]] uint32_t set,
		[[maybe_unused]] uint32_t way,
		[[maybe_unused]] uint64_t full_addr,
		[[maybe_unused]] uint64_t ip,
		[[maybe_unused]] uint64_t victim_addr,
		[[maybe_unused]] uint32_t type,
		[[maybe_unused]] uint8_t hit
		)
{
	// Nothing neeeds to be done
	return;
}

void CACHE::replacement_final_stats()
{
	// Nothing needs to be done
	return;
}
