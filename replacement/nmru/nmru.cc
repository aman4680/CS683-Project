#include <iostream>
#include <cassert>
#include <map>

#include "cache.h"

namespace NMRU
{
        std::map<CACHE*, std::vector<uint32_t>> mru_ways;
}


void CACHE::initialize_replacement()
{
	// Initialize the MRU position
        NMRU::mru_ways[this] = std::vector<uint32_t>();
	for(size_t idx = 0; idx < NUM_SET; idx++)
		NMRU::mru_ways[this].push_back(0);
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
	assert(set < NMRU::mru_ways[this].size());

	// Randomly pick a victim which does not match the MRU way
	uint32_t victim = NMRU::mru_ways[this][set];
       	while((victim = static_cast<uint32_t>(rand() % NUM_WAY)) == NMRU::mru_ways[this][set]);
       	return victim;
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
	assert(set < NMRU::mru_ways[this].size());
	assert(way < NUM_WAY);

	// Set the MRU way on cache hits and cache fills
        NMRU::mru_ways[this][set] = way;
}

void CACHE::replacement_final_stats()
{
	// Do nothing
        return;
}
