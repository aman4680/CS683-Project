#include <iostream>
#include <cassert>
#include <vector>
#include <algorithm>
#include <map>

#include "cache.h"

namespace LIP
{
        std::map<CACHE*, std::vector<std::vector<uint32_t>>> last_used;
}

void CACHE::initialize_replacement()
{
        LIP::last_used[this] = std::vector<std::vector<uint32_t>>();

        // Initialize the last use cycles for each set
	for(size_t idx = 0; idx < NUM_SET; idx++)
        {
                LIP::last_used[this].emplace_back();
                for(size_t jdx = 0; jdx < NUM_WAY; jdx++)
                        LIP::last_used[this].back().push_back(0);
        }
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
	assert(set < LIP::last_used[this].size());

	// Find the way which was accesed most futher back in the past
	std::vector<uint32_t>::iterator lru_it = std::min_element(LIP::last_used[this][set].begin(), LIP::last_used[this][set].end());
	return static_cast<uint32_t>(std::distance(LIP::last_used[this][set].begin(), lru_it));
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
	assert(set < LIP::last_used[this].size());
	assert(way < LIP::last_used[this][set].size());

	// Update the last used cycle
	if(hit)
		LIP::last_used[this][set][way] = current_cycle;
	else
	{
		// Get the current LRU cycle
		uint32_t lru_cycle = *std::min_element(LIP::last_used[this][set].begin(), LIP::last_used[this][set].end());

		// Insert at the LRU position
                LIP::last_used[this][set][way] = lru_cycle == 0 ? 0 : lru_cycle - 1;
	}
}

void CACHE::replacement_final_stats()
{
        // Do nothing
	return;
}
