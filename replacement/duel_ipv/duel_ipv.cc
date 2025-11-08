/*
 * This is the new Set Dueling IPV replacement policy.
 *
 * HOW TO USE:
 * 1. Add this file to your build system (Makefile/CMakeLists.txt).
 * 2. REMOVE 'replacement/pacipv/pacipv.cc' from your build system.
 * (You must compile only one replacement policy).
 * 3. When running, set TWO environment variables for the LLC:
 * export LLC_IPV_INSTR="0,1,1,0,3#0,1,0,0,3"
 * export LLC_IPV_DATA="0,0,0,1,3#0,0,1,1,3"
 */

#include <iostream>
#include <cstdlib>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cassert>
#include <map>
#include <unordered_set>

#include "cache.h"
#include "champsim.h" // Needed for instruction counter

uint64_t current_instr_count[NUM_CPUS] = {0};


// Create a new namespace for our dueling policy
namespace DUEL_IPV_Policy
{
    enum class cache_type
    {
        UNDEFINED,
        L1I,
        L1D,
        L2C,
        LLC
    };
    
    // --- STEP 1: Define the original PACIPV logic ---
    // This is the PACIPV class from pacipv.cc, renamed to PACIPV_internal.
    // It will be used *inside* our new DUEL_IPV class.
    class PACIPV_internal
    {
    private:
        uint32_t num_ways;
        const std::vector<uint32_t> demand_vector;
        const std::vector<uint32_t> prefetch_vector;
        std::vector<uint32_t> rrpvs;

    public:
        PACIPV_internal(uint32_t ways, decltype(demand_vector)& dv, decltype(prefetch_vector)& pv): num_ways(ways), demand_vector(dv), prefetch_vector(pv)
        {
            uint32_t max_valid_rrpv = demand_vector.size() - 1;
            rrpvs.resize(num_ways, max_valid_rrpv);
        }

        void demand_insert(uint32_t way)
        {
            assert(way < num_ways);
            rrpvs.at(way) = demand_vector.at(demand_vector.size() - 1);
        }

        void demand_promote(uint32_t way)
        {
            assert(way < num_ways);
            uint32_t old_rrpv = rrpvs.at(way);
            uint32_t new_rrpv = demand_vector.at(old_rrpv - 1);
            rrpvs.at(way) = new_rrpv;
        }

        void prefetch_insert(uint32_t way)
        {
            assert(way < num_ways);
            rrpvs.at(way) = prefetch_vector.at(prefetch_vector.size() - 1);
        }

        void prefetch_promote(uint32_t way)
        {
            assert(way < num_ways);
            uint32_t old_rrpv = rrpvs.at(way);
            uint32_t new_rrpv = prefetch_vector.at(old_rrpv - 1);
            rrpvs.at(way) = new_rrpv;
        }

        uint32_t find_victim()
        {
            uint32_t max_valid_rrpv = demand_vector.size() - 1;
            uint32_t max_rrpv = *std::max_element(rrpvs.cbegin(), rrpvs.cend());
            while (max_valid_rrpv != max_rrpv) {
                std::transform(rrpvs.cbegin(), rrpvs.cend(), rrpvs.begin(), [](uint32_t rrpv) { return rrpv + 1; });
                max_rrpv = *std::max_element(rrpvs.cbegin(), rrpvs.cend());
            }

            std::unordered_set<std::size_t> victims;
            for (decltype(rrpvs)::const_iterator it = rrpvs.cbegin(); it != rrpvs.cend(); it++) {
                if (*it == max_rrpv)
                    victims.insert(std::distance(rrpvs.cbegin(), it));
            }

            assert(victims.size() > 0 && victims.size() <= num_ways);
            std::size_t idx = rand() % victims.size();
            decltype(victims)::const_iterator it = victims.cbegin();
            std::advance(it, idx);
            return static_cast<uint32_t>(*it);
        }
    }; // --- End of PACIPV_internal class ---

    // --- Dueling Parameters ---
    constexpr uint32_t DUEL_NUM_SETS = 32;     // 16 for instr, 16 for data
    constexpr uint32_t DUEL_EPOCH_LENGTH = 256000; // Check winner every 256k instrs

    // --- Global Dueling State (per cache) ---
    // This state is shared by all sets in a cache
    struct DuelingState
    {
        long long policy_instr_misses = 0;
        long long policy_data_misses = 0;
        unsigned long long last_epoch_instrs = 0;
        int current_winner = 0; // 0 = instr, 1 = data

        // The vectors to duel between
        std::vector<uint32_t> demand_vector_instr;
        std::vector<uint32_t> prefetch_vector_instr;
        std::vector<uint32_t> demand_vector_data;
        std::vector<uint32_t> prefetch_vector_data;
    };
    std::map<CACHE*, DuelingState> cache_duel_state;

    // --- STEP 2: Create the DUEL_IPV "meta" class ---
    // One of these will be created *per set*
    class DUEL_IPV
    {
    private:
        PACIPV_internal policy_instr; // Internal policy for instruction-heavy
        PACIPV_internal policy_data;  // Internal policy for data-heavy
        DuelingState* global_state; // Pointer to the cache-wide shared state
        uint32_t set_index;         // This set's index

    public:
        // Constructor: Initialize both internal policies
        DUEL_IPV(uint32_t ways, uint32_t set_idx, DuelingState* state)
            : policy_instr(ways, state->demand_vector_instr, state->prefetch_vector_instr),
              policy_data(ways, state->demand_vector_data, state->prefetch_vector_data),
              global_state(state),
              set_index(set_idx)
        {
        }

        // Helper to get the correct policy based on set index and winner
        PACIPV_internal* get_policy()
        {
            if (set_index < (DUEL_NUM_SETS / 2)) {
                return &policy_instr; // This set *always* uses instr policy (Monitor A)
            } else if (set_index < DUEL_NUM_SETS) {
                return &policy_data; // This set *always* uses data policy (Monitor B)
            } else {
                // This is a "winner" set, use the global winner
                if (global_state->current_winner == 0)
                    return &policy_instr;
                else
                    return &policy_data;
            }
        }

        // --- Public Interface (to be called by CACHE::) ---

        uint32_t find_victim()
        {
            return get_policy()->find_victim();
        }

        // The insert functions now track misses for monitor sets
        void demand_insert(uint32_t way)
        {
            if (set_index < (DUEL_NUM_SETS / 2)) {
                global_state->policy_instr_misses++;
            } else if (set_index < DUEL_NUM_SETS) {
                global_state->policy_data_misses++;
            }
            get_policy()->demand_insert(way);
        }

        void demand_promote(uint32_t way)
        {
            get_policy()->demand_promote(way);
        }

        void prefetch_insert(uint32_t way)
        {
            if (set_index < (DUEL_NUM_SETS / 2)) {
                global_state->policy_instr_misses++;
            } else if (set_index < DUEL_NUM_SETS) {
                global_state->policy_data_misses++;
            }
            get_policy()->prefetch_insert(way);
        }

        void prefetch_promote(uint32_t way)
        {
            get_policy()->prefetch_promote(way);
        }
    }; // --- End of DUEL_IPV class ---

    // --- STEP 3: The global map that holds our policies ---
    std::map<CACHE*, std::vector<DUEL_IPV>> policies;

    // --- Helper function to parse IPV string (from pacipv.cc) ---
    bool parse_ipv_string(const std::string& ipv_vals, std::vector<uint32_t>& demand_vector, std::vector<uint32_t>& prefetch_vector, const std::string& cache_name)
    {
        std::size_t split_point = ipv_vals.find(std::string("#"));
        if (split_point == std::string::npos) {
            std::cerr << "[ERROR (" << cache_name << ")] Illegal IPV specified. Please provide both demand and prefetch IPVs separated by #" << std::endl;
            return false;
        }
        const std::string demand = ipv_vals.substr(0, split_point++);
        const std::string prefetch = ipv_vals.substr(split_point);

        std::istringstream demand_stream(demand);
        std::istringstream prefetch_stream(prefetch);
        uint32_t val;
        while (demand_stream >> val) {
            demand_vector.push_back(val);
            demand_stream.ignore();
        }
        while (prefetch_stream >> val) {
            prefetch_vector.push_back(val);
            prefetch_stream.ignore();
        }

        if (demand_vector.size() != prefetch_vector.size()) {
            std::cerr << "[ERROR (" << cache_name << ")] Illegal IPV specified. The sizes of demand and prefetch IPVs are not same." << std::endl;
            return false;
        }

        const uint32_t min_demand_rrpv = *std::min_element(demand_vector.cbegin(), demand_vector.cend());
        const uint32_t max_demand_rrpv = *std::max_element(demand_vector.cbegin(), demand_vector.cend());
        const uint32_t min_prefetch_rrpv = *std::min_element(prefetch_vector.cbegin(), prefetch_vector.cend());
        const uint32_t max_prefetch_rrpv = *std::max_element(prefetch_vector.cbegin(), prefetch_vector.cend());
        if (max_demand_rrpv >= demand_vector.size() || min_demand_rrpv < 1 || max_prefetch_rrpv >= prefetch_vector.size() || min_prefetch_rrpv < 1) {
            std::cerr << "[ERROR (" << cache_name << ")] Illegal IPV specified. Illegal RRPV value(s) found in IPVs." << std::endl;
            return false;
        }
        return true;
    }    
} // --- End of namespace DUEL_IPV_Policy ---

// --- STEP 4: Define the CACHE:: functions ---
// These functions are called by ChampSim.

void CACHE::initialize_replacement()
{
    // Get the cache type (copied from pacipv.cc)
    DUEL_IPV_Policy::cache_type cache;
    if (this->NAME.find("L1I") != std::string::npos)
        cache = DUEL_IPV_Policy::cache_type::L1I;
    else if (this->NAME.find("L1D") != std::string::npos)
        cache = DUEL_IPV_Policy::cache_type::L1D;
    else if (this->NAME.find("L2C") != std::string::npos)
        cache = DUEL_IPV_Policy::cache_type::L2C;
    else if (this->NAME.find("LLC") != std::string::npos)
        cache = DUEL_IPV_Policy::cache_type::LLC;
    else
        cache = DUEL_IPV_Policy::cache_type::UNDEFINED;

    if (cache == DUEL_IPV_Policy::cache_type::UNDEFINED) {
        std::cerr << "[ERROR (" << this->NAME << ")] Could not infer cache type from name." << std::endl;
        std::exit(-1);
    }

    // --- MODIFIED LOGIC: Read TWO IPV specs for LLC ---
    const char* ipv_string_instr = nullptr;
    const char* ipv_string_data = nullptr;

    switch (cache) {
    case DUEL_IPV_Policy::cache_type::L1I:
        ipv_string_instr = std::getenv("L1I_IPV");
        ipv_string_data = ipv_string_instr; // L1I doesn't duel
        break;
    case DUEL_IPV_Policy::cache_type::L1D:
        ipv_string_instr = std::getenv("L1D_IPV");
        ipv_string_data = ipv_string_instr; // L1D doesn't duel
        break;
    case DUEL_IPV_Policy::cache_type::L2C:
        ipv_string_instr = std::getenv("L2C_IPV");
        ipv_string_data = ipv_string_instr; // L2C doesn't duel
        break;
    case DUEL_IPV_Policy::cache_type::LLC:
        // LLC is the only one that duels. It needs TWO env vars.
        ipv_string_instr = std::getenv("LLC_IPV_INSTR");
        ipv_string_data = std::getenv("LLC_IPV_DATA");
        break;
    default:
        std::cerr << "[ERROR (" << this->NAME << ")] Unknown cache type" << std::endl;
        std::exit(-1);
    }

    if (ipv_string_instr == nullptr) {
        std::cerr << "[ERROR (" << this->NAME << ")] Main IPV (or LLC_IPV_INSTR) not specified" << std::endl;
        std::exit(-1);
    }

    if (cache == DUEL_IPV_Policy::cache_type::LLC && ipv_string_data == nullptr) {
        std::cerr << "[ERROR (" << this->NAME << ")] LLC_IPV_DATA not specified. Dueling requires two policies." << std::endl;
        std::exit(-1);
    } else if (cache == DUEL_IPV_Policy::cache_type::LLC && ipv_string_data == nullptr) {
        ipv_string_data = ipv_string_instr; // Fallback if not LLC
    }

    // --- NEW: Initialize global duel state for this cache ---
    DUEL_IPV_Policy::cache_duel_state[this] = DUEL_IPV_Policy::DuelingState();
    DUEL_IPV_Policy::DuelingState* global_state = &DUEL_IPV_Policy::cache_duel_state[this];

    // --- NEW: Parse BOTH strings into the global state ---
    if (!DUEL_IPV_Policy::parse_ipv_string(std::string(ipv_string_instr), global_state->demand_vector_instr, global_state->prefetch_vector_instr, this->NAME))
        std::exit(-1);
    
    if (!DUEL_IPV_Policy::parse_ipv_string(std::string(ipv_string_data), global_state->demand_vector_data, global_state->prefetch_vector_data, this->NAME))
        std::exit(-1);

    // Print out the parsed IPVS
    std::cout << "[" << this->NAME << "] INSTR Demand IPV:";
    for (const uint32_t v : global_state->demand_vector_instr) std::cout << " " << v;
    std::cout << " Prefetch IPV:";
    for (const uint32_t v : global_state->prefetch_vector_instr) std::cout << " " << v;
    std::cout << std::endl;

    if (cache == DUEL_IPV_Policy::cache_type::LLC) {
        std::cout << "[" << this->NAME << "] DATA  Demand IPV:";
        for (const uint32_t v : global_state->demand_vector_data) std::cout << " " << v;
        std::cout << " Prefetch IPV:";
        for (const uint32_t v : global_state->prefetch_vector_data) std::cout << " " << v;
        std::cout << std::endl;
    }


    // --- NEW: Allocate the DUEL_IPV policies ---
    DUEL_IPV_Policy::policies[this] = std::vector<DUEL_IPV_Policy::DUEL_IPV>();
    for (size_t idx = 0; idx < NUM_SET; idx++) {
        DUEL_IPV_Policy::policies[this].emplace_back(NUM_WAY, idx, global_state);
    }
}

uint32_t CACHE::find_victim(
    [[maybe_unused]] uint32_t triggering_cpu,
    [[maybe_unused]] uint64_t instr_id,
    uint32_t set,
    [[maybe_unused]] const BLOCK* current_set,
    [[maybe_unused]] uint64_t ip,
    [[maybe_unused]] uint64_t full_addr,
    [[maybe_unused]] uint32_t type)
{
    assert(set < DUEL_IPV_Policy::policies[this].size());
    // Delegate to our DUEL_IPV object for this set
    return DUEL_IPV_Policy::policies[this][set].find_victim();
}

void CACHE::update_replacement_state(
    uint32_t triggering_cpu,
    uint32_t set,
    uint32_t way,
    [[maybe_unused]] uint64_t full_addr,
    [[maybe_unused]] uint64_t ip,
    [[maybe_unused]] uint64_t victim_addr,
    uint32_t type,
    uint8_t hit)
{
    assert(way < NUM_WAY);
    assert(set < DUEL_IPV_Policy::policies[this].size());

    // --- NEW: Epoch Check Logic (only for LLC) ---
    if (DUEL_IPV_Policy::cache_duel_state.count(this)) // Check if this cache has duel state
    {
        DUEL_IPV_Policy::DuelingState* global_state = &DUEL_IPV_Policy::cache_duel_state[this];

        // 'current_instr_count' is a global from champsim.h
        if (current_instr_count[triggering_cpu] - global_state->last_epoch_instrs > DUEL_IPV_Policy::DUEL_EPOCH_LENGTH) {
            
            // Only duel if the policies are different
            if(global_state->demand_vector_instr != global_state->demand_vector_data) {
                if (global_state->policy_data_misses < global_state->policy_instr_misses) {
                    global_state->current_winner = 1; // data wins
                } else {
                    global_state->current_winner = 0; // instr wins (or ties)
                }
            }

            // <<< PUT DEBUG PRINT HERE >>>
    std::cout << "[DUEL] Epoch ended. INSTR misses = " << global_state->policy_instr_misses
              << ", DATA misses = " << global_state->policy_data_misses
              << ". Winner = " << (global_state->current_winner == 0 ? "INSTR" : "DATA") 
              << std::endl;

            // Reset for next epoch
            global_state->policy_instr_misses = 0;
            global_state->policy_data_misses = 0;
            global_state->last_epoch_instrs = current_instr_count[triggering_cpu];
        }
    }

    // --- (Original logic from pacipv.cc) ---
    int was_prefetch = access_type{type} == access_type::PREFETCH;

    if (was_prefetch) {
        if (hit)
            DUEL_IPV_Policy::policies[this].at(set).prefetch_promote(way);
        else
            // This call will now also check/increment miss counter
            DUEL_IPV_Policy::policies[this].at(set).prefetch_insert(way);
    } else {
        if (hit)
            DUEL_IPV_Policy::policies[this].at(set).demand_promote(way);
        else
            // This call will now also check/increment miss counter
            DUEL_IPV_Policy::policies[this].at(set).demand_insert(way);
    }
}

void CACHE::replacement_final_stats()
{
    // Do nothing
    return;
}

