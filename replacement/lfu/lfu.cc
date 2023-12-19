#include <iostream>
#include <vector>
#include <algorithm>
#include <cassert>
#include <map>

#include "cache.h"

#define MAX_FREQ (1ULL << 4)-1

namespace LFU_Policy
{
        class LFU
        {
                private:
                        std::vector<uint32_t> frequencies;

                public:
                        LFU(uint32_t ways)
                        {
                                this->frequencies = std::vector<uint32_t>(ways);
                        }

                        void increment(uint32_t way)
                        {
                                // Run sanity check
                                assert(way < this->frequencies.size());

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
                                return static_cast<uint32_t>(std::distance(this->frequencies.begin(), min_way));
                        }
        };

        std::map<CACHE*, std::vector<LFU>> policies;
}

void CACHE::initialize_replacement()
{
        LFU_Policy::policies[this] = std::vector<LFU_Policy::LFU>();

        // Allocate the policies, one per cache set
        for(size_t idx = 0; idx < NUM_SET; idx++)
                LFU_Policy::policies[this].emplace_back(NUM_WAY);
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
        assert(set < LFU_Policy::policies[this].size());

        // Return the LFU way
        return LFU_Policy::policies[this][set].get_lfu();
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
        assert(set < LFU_Policy::policies[this].size());

        // Increment frequency
        if(!hit)        // Only update the state on cache fills
                LFU_Policy::policies[this][set].increment(way);
}

void CACHE::replacement_final_stats()
{
        // Do nothing
        return;
}
