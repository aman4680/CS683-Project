#include <iostream>
#include <cassert>
#include <vector>
#include <algorithm>

#include "cache.h"

std::vector<uint32_t>* last_used;

void CACHE::initialize_replacement()
{
	// Initialize the last use cycles for each set
	last_used = new std::vector<uint32_t>[NUM_SET]();
	for(uint32_t idx = 0; idx < NUM_SET; idx++)
		for(uint32_t way = 0; way < NUM_WAY; way++)
			last_used[idx].push_back(0);
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

	// Find the way which was accesed most futher back in the past
	std::vector<uint32_t>::iterator lru_it = std::min_element(last_used[set].begin(), last_used[set].end());
	return (uint32_t)(std::distance(last_used[set].begin(), lru_it));
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

	// Update the last used cycle
	if(hit)
		last_used[set][way] = current_cycle;
	else
	{
		// Get the current LRU cycle
		uint32_t lru_cycle = *std::min_element(last_used[set].begin(), last_used[set].end());

		// Insert at the LRU position
		last_used[set][way] = lru_cycle == 0 ? 0 : lru_cycle - 1;
	}
}

void CACHE::replacement_final_stats()
{
	delete[] last_used;
	return;
}
