#include <iostream>
#include <cassert>

// #define DEBUG

#include "cache.h"

#define NODE_TYPE_INTERNAL -1

namespace PseudoLRU
{
	class LRUTree
	{
		private:
			// This structure represents the nodes of the LRUTree
			struct Node
			{
				int32_t way;		// The way this node is associated with. This is set to NODE_TYPE_INTERNAL for internal nodes
				bool dir;		// The direction of this node. False means left and True means right

				Node()
				{
					this->way = NODE_TYPE_INTERNAL;
					this->dir = false;
				}
			};

			Node* all_nodes;	// Array of all the nodes of the LRUTree
			uint32_t num_nodes;	// The total number of nodes in the tree
			uint32_t num_leaves;	// Number of leaves in the LRUTree

		public:
			LRUTree()
			{
				this->all_nodes = nullptr;
				this->num_nodes = 0;
				this->num_leaves = 0;
			}

			// Build the LRU tree with num_ways leaves
			void set_ways(uint32_t num_ways)
			{
				this->num_leaves = num_ways;
				this->num_nodes = (this->num_leaves * 2) - 1;

				this->all_nodes = new Node[this->num_nodes]();	// Allocate the nodes

				// Set the way for the leaf nodes
				uint32_t way = 0;
				for(uint32_t idx = this->num_nodes - this->num_leaves; idx < this->num_nodes; idx++)
					this->all_nodes[idx].way = way++;
			}

			// Move the way to the MRU position
			void move_to_MRU(uint32_t way)
			{
				// Run sanity check
				assert(way < this->num_leaves);

				// Traverse the tree from the node corresponding to the way to the root while flipping the direction bits
				uint32_t idx = this->num_nodes - this->num_leaves + way;	// Start from the node corresponding to way

				// Flip bits until we reach the root node
				while(idx != 0)
				{
					uint32_t parent = (idx - 1) / 2;	// Get the parent index

					// Sanity check
					assert(parent < this->num_nodes);

					// The right child is always at an even index
					if(this->all_nodes[parent].dir ^ (idx % 2))
						this->all_nodes[parent].dir = !this->all_nodes[parent].dir;	// Flip the direction bit if it points towards the child
					idx = parent;
				}
			}

			// Get the LRU way
			int32_t get_LRU()
			{
				uint32_t idx = 0;	// Start from the root node which is at the 0th index

				// Traverse the tree until we reach a leaf node
				while(this->all_nodes[idx].way == NODE_TYPE_INTERNAL)
					idx = this->all_nodes[idx].dir ? 2 * (idx + 1) : 2 * (idx + 1) - 1;

				// Run sanity checks
				assert(this->all_nodes[idx].way >= 0);
				assert((uint32_t)this->all_nodes[idx].way < this->num_leaves);

				// Return the LRU way
				return this->all_nodes[idx].way;
			}

			#ifdef DEBUG
				void print_directions()
				{
					std::cout << "\t---->";
					for(uint32_t idx = 0; idx < this->num_nodes; idx++)
						std::cout << idx << ".dir:" << this->all_nodes[idx].dir << ", idx.way:" << this->all_nodes[idx].way << "\t";
					std::cout << std::endl;
				}
			#endif

			~LRUTree()
			{
				if(this->all_nodes != nullptr)
					delete[] this->all_nodes;
			}
	};
}

PseudoLRU::LRUTree* trees;

void CACHE::initialize_replacement()
{
	#ifdef DEBUG
		std::cout << "[DEBUG] Creating LRU trees" << std::endl;
	#endif

	trees = new PseudoLRU::LRUTree[NUM_SET]();	// Create the LRU Trees for all the cache sets

	// Set the number of ways for all sets
	for(uint32_t i = 0; i < NUM_SET; i++)
		trees[i].set_ways(NUM_WAY);
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

	#ifdef DEBUG
		int32_t victim = trees[set].get_LRU();
		std::cout << "[DEBUG] Set:" << set << " Found victim way " << victim << std::endl;
		trees[set].print_directions();
		return (uint32_t)victim;
	#endif

	// Victim is at the LRU position
	return (uint32_t)trees[set].get_LRU();
}

void CACHE::update_replacement_state(
		[[maybe_unused]] uint32_t triggering_cpu,
		uint32_t set,
		uint32_t way,
		[[maybe_unused]] uint64_t full_addr,
		[[maybe_unused]] uint64_t ip,
		[[maybe_unused]] uint64_t victim_addr,
		[[maybe_unused]] uint32_t type,
		uint8_t hit
		)
{
	// Run sanity checks
	assert(way < NUM_WAY);
	assert(set < NUM_SET);

	// Move the way to the MRU position
	if(!hit)	// Only update the state on cache fills
		trees[set].move_to_MRU(way);

	#ifdef DEBUG
		std::cout << "[DEBUG] Set:" << set << " way " << way << " moved to MRU" << std::endl;
		trees[set].print_directions();
	#endif
}

void CACHE::replacement_final_stats()
{
	#ifdef DEBUG
		std::cout << "[DEBUG] Deleting LRU trees" << std::endl;
	#endif

	delete[] trees;
}
