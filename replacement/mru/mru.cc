#include <iostream>
#include <cassert>

#include "cache.h"

uint32_t* mru_ways;

void CACHE::initialize_replacement()
{
	// Initialize the MRU position
	mru_ways = new uint32_t[NUM_SET];
	for(uint32_t idx = 0; idx < NUM_SET; idx++)
		mru_ways[idx] = 0;
}

uint32_t CACHE::find_victim(
		[[maybe_unused]] uint32_t triggering_cpu,
		[[maybe_unused]] uint64_t instr_id,
		uint32_t set,
		[[maybe_unused]] const BLOCK* current_set,
		[[maybe_unused]] uint64_t ip,
		[[maybe_unused]] uint64_t full_addr,
		[[maybe_unused]] uint32_t type
		)
{
	// Sanity check
	assert(set < NUM_SET);

	// The victim is the most recently used way
	return mru_ways[set];
}

void CACHE::update_replacement_state(
		[[maybe_unused]] uint32_t triggering_cpu,
		uint32_t set,
		uint32_t way,
		[[maybe_unused]] uint64_t full_addr,
		[[maybe_unused]] uint64_t ip,
		[[maybe_unused]] uint64_t victim_addr,
		[[maybe_unused]] uint32_t type,
		[[maybe_unused]] uint8_t hit
		)
{
	// Sanity checks
	assert(set < NUM_SET);
	assert(way < NUM_WAY);

	// Set the MRU way on cache hits and cache fills
	mru_ways[set] = way;
}

void CACHE::replacement_final_stats()
{
	delete[] mru_ways;
	return;
}
