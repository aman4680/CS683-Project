#include <iostream>
#include <cassert>
#include <cstdlib>
#include <sstream>
#include <map>
#include <time.h>

#include "cache.h"

#define DENOMINATOR (1ull<<8)

namespace BIP
{
        uint32_t NUMERATOR = 0; // The higher the NUMERATOR, the more likely it is that inserts will be made at the LRU position
        std::map<CACHE*, std::vector<std::vector<uint32_t>>> last_used;
}

void CACHE::initialize_replacement()
{
        // Read the numerator of epsilon from environment variable
        const char* numerator = std::getenv("EPS");
        if(numerator != nullptr)
        {
                std::stringstream strValue;
                strValue << numerator;
                strValue >> BIP::NUMERATOR;
        }

        // Create the LRU stacks for each cache set
        BIP::last_used[this] = std::vector<std::vector<uint32_t>>();
        for(size_t idx = 0; idx < NUM_SET; idx++)
        {
                BIP::last_used[this].emplace_back();
                for(size_t jdx = 0; jdx < NUM_WAY; jdx++)
                        BIP::last_used[this].back().push_back(current_cycle);   // Set the last used position of every way to current cycle
        }

        // Seed the PRNG
        srand(time(NULL));
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
        // Sanity checks
        assert(set < BIP::last_used[this].size());

        // Return the LRU way
        std::vector<uint32_t>::iterator min_it = std::min_element(BIP::last_used[this][set].begin(), BIP::last_used[this][set].end());
        return (uint32_t)std::distance(BIP::last_used[this][set].begin(), min_it);
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
        assert(set < BIP::last_used[this].size());
        assert(way < BIP::last_used[this][set].size());

        // Get a random number
        uint32_t rand_val = rand() % DENOMINATOR;

        if(rand_val > BIP::NUMERATOR)  // Insert at MRU
        {
                BIP::last_used[this][set][way] = current_cycle;
        }
        else    // Insert at LRU
        {
                uint32_t min_val = *std::min_element(BIP::last_used[this][set].begin(), BIP::last_used[this][set].end());
                BIP::last_used[this][set][way] = min_val == 0 ? 0 : min_val - 1;
        }
}

void CACHE::replacement_final_stats()
{
        // Do nothing
        return;
}
