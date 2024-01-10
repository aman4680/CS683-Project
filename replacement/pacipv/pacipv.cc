#include <iostream>
#include <cstdlib>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cassert>
#include <map>
#include <unordered_set>

#include "cache.h"

namespace PACIPV_Policy
{
        enum class cache_type
        {
                UNDEFINED,
                L1I,
                L1D,
                L2C,
                LLC
        };

        class PACIPV
        {
                private:
                        uint32_t num_ways;                              // Number of ways in this set
                        const std::vector<uint32_t> demand_vector;      // Demand IPV
                        const std::vector<uint32_t> prefetch_vector;    // Prefetch IPV
                        std::vector<uint32_t> rrpvs;                    // Current RRPV of all the ways

                public:
                        PACIPV(uint32_t ways, decltype(demand_vector)& dv, decltype(prefetch_vector)& pv): num_ways(ways), demand_vector(dv), prefetch_vector(pv)
                {
                        // Initialize the RRPVs of all the ways
                        uint32_t max_valid_rrpv = demand_vector.size() - 1;
                        rrpvs.resize(num_ways, max_valid_rrpv);
                }

                        void demand_insert(uint32_t way)
                        {
                                // Sanity check
                                assert(way < num_ways);

                                // Update the RRPV to the insertion RRPV
                                rrpvs.at(way) = demand_vector.at(demand_vector.size() - 1);
                        }

                        void demand_promote(uint32_t way)
                        {
                                // Sanity check
                                assert(way < num_ways);

                                // Update RRPV
                                uint32_t old_rrpv = rrpvs.at(way);
                                uint32_t new_rrpv = demand_vector.at(old_rrpv - 1);
                                rrpvs.at(way) = new_rrpv;
                        }

                        void prefetch_insert(uint32_t way)
                        {
                                // Sanity check
                                assert(way < num_ways);

                                // Update the RRPV to the insertion RRPV
                                rrpvs.at(way) = prefetch_vector.at(prefetch_vector.size() - 1);
                        }

                        void prefetch_promote(uint32_t way)
                        {
                                // Sanity check
                                assert(way < num_ways);

                                // Update RRPV
                                uint32_t old_rrpv = rrpvs.at(way);
                                uint32_t new_rrpv = prefetch_vector.at(old_rrpv - 1);
                                rrpvs.at(way) = new_rrpv;
                        }

                        uint32_t find_victim()
                        {
                                // Find the maximum valid RRPV
                                uint32_t max_valid_rrpv = demand_vector.size() - 1;

                                // Increment all RRPVs for all ways until at least one way is assigned the maximum valid RRPV
                                uint32_t max_rrpv = *std::max_element(rrpvs.cbegin(), rrpvs.cend());
                                while(max_valid_rrpv != max_rrpv)
                                {
                                        std::transform(rrpvs.cbegin(), rrpvs.cend(), rrpvs.begin(), [](uint32_t rrpv){ return rrpv + 1; });
                                        max_rrpv = *std::max_element(rrpvs.cbegin(), rrpvs.cend());
                                }

                                // Create a set of all the ways which have the maximum RRPV
                                std::unordered_set<std::size_t> victims;
                                for(decltype(rrpvs)::const_iterator it = rrpvs.cbegin(); it != rrpvs.cend(); it++)
                                {
                                        if(*it == max_rrpv)
                                                victims.insert(std::distance(rrpvs.cbegin(), it));
                                }

                                // Run sanity check
                                assert(victims.size() > 0 && victims.size() <= num_ways);

                                // Randomly pick one of the ways in the victims set
                                std::size_t idx = rand() % victims.size();
                                decltype(victims)::const_iterator it = victims.cbegin();
                                std::advance(it, idx);
                                return static_cast<uint32_t>(*it);
                        }
        };

        std::map<CACHE*, std::vector<PACIPV>> policies;
}

void CACHE::initialize_replacement()
{
        PACIPV_Policy::policies[this] = std::vector<PACIPV_Policy::PACIPV>();

        // Get the cache type
        PACIPV_Policy::cache_type cache;
        if(this->NAME.find("L1I") != std::string::npos)
                cache = PACIPV_Policy::cache_type::L1I;
        else if(this->NAME.find("L1D") != std::string::npos)
                cache = PACIPV_Policy::cache_type::L1D;
        else if(this->NAME.find("L2C") != std::string::npos)
                cache = PACIPV_Policy::cache_type::L2C;
        else if(this->NAME.find("LLC") != std::string::npos)
                cache = PACIPV_Policy::cache_type::LLC;
        else
                cache = PACIPV_Policy::cache_type::UNDEFINED;

        if(cache == PACIPV_Policy::cache_type::UNDEFINED)
        {
                std::cerr << "[ERROR (" << this->NAME << ")] Could not infer cache type from name." << std::endl;
                std::exit(-1);
        }

        // Read the IPV specification from the environment varibles
        const char* ipv_string;
        std::vector<uint32_t> demand_vector, prefetch_vector;
        switch(cache)
        {
                case PACIPV_Policy::cache_type::L1I:
                        ipv_string = std::getenv("L1I_IPV");
                        break;
                case PACIPV_Policy::cache_type::L1D:
                        ipv_string = std::getenv("L1D_IPV");
                        break;
                case PACIPV_Policy::cache_type::L2C:
                        ipv_string = std::getenv("L2C_IPV");
                        break;
                case PACIPV_Policy::cache_type::LLC:
                        ipv_string = std::getenv("LLC_IPV");
                        break;
                default:
                        std::cerr << "[ERROR (" << this->NAME << ")] Unknown cache type" << std::endl;
                        std::exit(-1);
        }

        if(ipv_string == nullptr)
        {
                std::cerr << "[ERROR (" << this->NAME << ")] IPV not specified" << std::endl;
                std::exit(-1);
        }

        // Convert to C++ string
        std::string ipv_vals(ipv_string);

        // Split the string into demand and prefetch strings
        std::size_t split_point = ipv_vals.find(std::string("#"));
        if(split_point ==  std::string::npos)
        {
                std::cerr << "[ERROR (" << this->NAME << ")] Illegal IPV specified. Please provide both demand and prefetch IPVs." << std::endl;
                std::exit(-1);
        }
        const std::string demand = ipv_vals.substr(0, split_point++);
        const std::string prefetch = ipv_vals.substr(split_point);

        // Populate the demand and prefetch vectors based on the IPV strings
        std::istringstream demand_stream(demand);
        std::istringstream prefetch_stream(prefetch);
        uint32_t val;
        while(demand_stream >> val)
        {
                demand_vector.push_back(val);
                demand_stream.ignore();
        }
        while(prefetch_stream >> val)
        {
                prefetch_vector.push_back(val);
                prefetch_stream.ignore();
        }

        // Check if the provided IPVs are valid
        if(demand_vector.size() != prefetch_vector.size())
        {
                std::cerr << "[ERROR (" << this->NAME << ")] Illegal IPV specified. The sizes of demand and prefetch IPVs are not same." << std::endl;
                std::exit(-1);
        }

        const uint32_t min_demand_rrpv = *std::min_element(demand_vector.cbegin(), demand_vector.cend());
        const uint32_t max_demand_rrpv = *std::max_element(demand_vector.cbegin(), demand_vector.cend());
        const uint32_t min_prefetch_rrpv = *std::min_element(prefetch_vector.cbegin(), prefetch_vector.cend());
        const uint32_t max_prefetch_rrpv = *std::max_element(prefetch_vector.cbegin(), prefetch_vector.cend());
        if(max_demand_rrpv >= demand_vector.size() || min_demand_rrpv < 1 || max_prefetch_rrpv >= prefetch_vector.size() || min_prefetch_rrpv < 1)
        {
                std::cerr << "[ERROR (" << this->NAME << ")] Illegal IPV specified. Illegal RRPV value(s) found in IPVs." << std::endl;
                std::exit(-1);
        }

        // Print out the parsed IPVS
        std::cout << "[" << this->NAME << "] Demand IPV:";
        for(const uint32_t v: demand_vector)
                std::cout << " " << v;
        std::cout << " Prefetch IPV:";
        for(const uint32_t v: prefetch_vector)
                std::cout << " " << v;
        std::cout << std::endl;

        // Allocate the policies, one per cache set
        for(size_t idx = 0; idx < NUM_SET; idx++)
                PACIPV_Policy::policies[this].emplace_back(NUM_WAY, demand_vector, prefetch_vector);
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
        assert(set < PACIPV_Policy::policies[this].size());

        return PACIPV_Policy::policies[this][set].find_victim();
}

void CACHE::update_replacement_state(
                [[maybe_unused]] uint32_t triggering_cpu,
                uint32_t set,
                uint32_t way,
                [[maybe_unused]] uint64_t full_addr,
                [[maybe_unused]] uint64_t ip,
                [[maybe_unused]] uint64_t victim_addr,
                uint32_t type,
                uint8_t hit
                )
{
        // Run sanity checks
        assert(way < NUM_WAY);
        assert(set < PACIPV_Policy::policies[this].size());

        // Figure out if access was a prefetch access
        int was_prefetch = access_type{type} == access_type::PREFETCH;

        if(was_prefetch)  // Handle prefetch access
        {
                if(hit)
                        PACIPV_Policy::policies[this].at(set).prefetch_promote(way);
                else
                        PACIPV_Policy::policies[this].at(set).prefetch_insert(way);
        }
        else    // Handle demand access
        {
                if(hit)
                        PACIPV_Policy::policies[this].at(set).demand_promote(way);
                else
                        PACIPV_Policy::policies[this].at(set).demand_insert(way);
        }
}

void CACHE::replacement_final_stats()
{
        // Do nothing
        return;
}
