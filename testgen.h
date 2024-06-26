#include <random>
#include "search.h"
#include "knitting.h"
#include "cbraid.h"
#include "util.h"

#ifndef TESTGEN_H
#define TESTGEN_H

namespace knitting {

class TestCase {

    KnittingMachine machine;
    const std::vector<char> source_back_needles;
    const std::vector<char> source_front_needles;
    const std::vector<char> target_back_needles;
    const std::vector<char> target_front_needles;
    const cb::ArtinBraid source_braid;
    const std::vector<SlackConstraint> slack_constraints;
    char target_racking;
    int target_needle_count;

public:
    TestCase(
        const KnittingMachine,
        const std::vector<char>&,
        const std::vector<char>&,
        const std::vector<char>&,
        const std::vector<char>&,
        const cb::ArtinBraid&,
        const std::vector<SlackConstraint>&,
        char = 0
    );
    TestCase(const TestCase&);


    search::SearchResult<KnittingState> test(bool, unsigned int (KnittingState::*h)() const);
    search::SearchResult<KnittingState> test_id(bool, unsigned int (KnittingState::*h)() const);
    search::SearchResult<KnittingStateLM21> test(
        bool, unsigned int (KnittingStateLM21::*h)() const
    );

    friend std::ostream& operator<<(std::ostream&, const TestCase&);
};

TestCase flat_lace (KnittingMachine, int, int, std::mt19937&);
TestCase simple_tube (KnittingMachine, int, int, std::mt19937&);

class ResultAggregate {
public:
    std::size_t search_tree_size;
    double seconds_taken;

    ResultAggregate();

    template<typename State>
    void add_result(search::SearchResult<State> result) {
        search_tree_size += result.search_tree_size;
        seconds_taken += result.seconds_taken;
    }
};

}

#endif
