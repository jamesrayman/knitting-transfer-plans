#include "prebuilt.h"
#include "util.h"
#include <vector>

namespace prebuilt {

std::vector<std::vector<std::vector<unsigned long long>>> table;
int table_min_racking;

void bitset_set_insert(std::vector<unsigned long long>& set, unsigned long long bitset) {
    for (unsigned int i = 0; i < set.size();) {
        if (bitset == set[i]) {
            return;
        }
        if ((set[i] | bitset) == bitset) {
            set[i] = set.back();
            set.pop_back();
        }
        else {
            i++;
        }
    }
    set.push_back(bitset);
}
unsigned long long bitset_shift(unsigned long long bitset, int shift) {
    return shift > 0 ? bitset << shift : bitset >> -shift;
}

void construct_table(int max_steps, int min_racking, int max_racking) {
    table_min_racking = min_racking;

    // base case of 0 steps;
    table.emplace_back();
    for (int racking = min_racking; racking <= max_racking; racking++) {
        table[0].emplace_back();
        if (racking == 0) {
            table[0].back().push_back(1UL << 32);
        }
    }

    for (int step = 1; step <= max_steps; step++) {
        table.emplace_back();
        for (int racking = min_racking; racking <= max_racking; racking++) {
            table[step].emplace_back();
            for (int prev_racking = min_racking; prev_racking <= max_racking; prev_racking++) {
                for (auto prev_offsets : table[step-1][prev_racking-min_racking]) {
                    bitset_set_insert(
                        table[step][racking-min_racking],
                        prev_offsets | bitset_shift(prev_offsets, prev_racking - racking)
                    );
                }
            }
        }
    }
}

unsigned int query(unsigned long long offsets, int racking) {
    for (unsigned int steps = log_offsets(offsets); steps < table.size(); steps++) {
        for (auto cand_offsets : table[steps][racking-table_min_racking]) {
            if ((offsets | cand_offsets) == cand_offsets) {
                return steps;
            }
        }
    }
    return (unsigned int)table.size();
}

}
