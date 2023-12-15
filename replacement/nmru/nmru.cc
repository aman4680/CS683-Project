#include <iostream>
#include <stdlib.h>
#include <time.h>

#include "cache.h"

uint32_t mru_way = -1;

void CACHE::initialize_replacement()
{
	// Nothing needs to be done
	return;
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
	// Randomly pick a victim which does not match the MRU way
	uint32_t victim = mru_way;
       while((victim = (uint32_t)(rand() % NUM_WAY)) == mru_way);
       return victim;
}

void CACHE::update_replacement_state(
		[[maybe_unused]] uint32_t triggering_cpu,
		[[maybe_unused]] uint32_t set,
		uint32_t way,
		[[maybe_unused]] uint64_t full_addr,
		[[maybe_unused]] uint64_t ip,
		[[maybe_unused]] uint64_t victim_addr,
		[[maybe_unused]] uint32_t type,
		[[maybe_unused]] uint8_t hit
		)
{
	// Set the MRU way on cache hits and cache fills
	mru_way = way;
}

void CACHE::replacement_final_stats()
{
	// Nothing needs to be done
	return;
}
