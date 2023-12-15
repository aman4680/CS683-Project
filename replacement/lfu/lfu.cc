#include <iostream>
#include <vector>
#include <algorithm>
#include <cassert>

#include "cache.h"

#define MAX_FREQ (1ULL << 4)-1

namespace LFU_Policy
{
	class LFU
	{
		private:
			std::vector<uint32_t> frequencies;
			uint32_t num_ways = 0;

		public:
			LFU()
			{
				this->num_ways = 0;
			}

			void set_ways(uint32_t ways)
			{
				this->num_ways = ways;

				uint32_t idx = 0;
				for(idx = 0; idx < this->num_ways; idx++)
					this->frequencies.emplace_back(0);
			}

			void increment(uint32_t way)
			{
				// Run sanity check
				assert(way < this->num_ways);

				// Get the maximum frequency
				uint32_t max_freq = *std::max_element(this->frequencies.begin(), this->frequencies.end());

				// Divide the frequencies by 2 if the maximum frequency exceeds MAX_FREQ
				if(max_freq >= MAX_FREQ)
				{
					std::vector<uint32_t>::iterator it;
					for(it = this->frequencies.begin(); it != this->frequencies.end(); it++)
						*it >>= 1;
				}

				this->frequencies[way]++;
			}

			uint32_t get_lfu()
			{
				std::vector<uint32_t>::iterator min_way = std::min_element(this->frequencies.begin(), this->frequencies.end());
				return (uint32_t)(std::distance(this->frequencies.begin(), min_way));
			}
	};
}

LFU_Policy::LFU* policies;

void CACHE::initialize_replacement()
{
	policies = new LFU_Policy::LFU[NUM_SET]();	// Create the LFU policies for all the cache sets

	// Set the number of ways for all sets
	for(uint32_t i = 0; i < NUM_SET; i++)
		policies[i].set_ways(NUM_WAY);
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
	// Run sanity check
	assert(set < NUM_SET);

	// Return the LFU way
	return policies[set].get_lfu();
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
	// Run sanity checks
	assert(way < NUM_WAY);
	assert(set < NUM_SET);

	// Increment frequency
	if(!hit)	// Only update the state on cache fills
		policies[set].increment(way);
}

void CACHE::replacement_final_stats()
{
	delete[] policies;

	// Nothing needs to be done
	return;
}
