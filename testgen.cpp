#include "testgen.h"
#include "cbraid.h"


namespace knitting {

TestCase::TestCase(
    const KnittingMachine machine,
    const std::vector<char>& source_back_needles,
    const std::vector<char>& source_front_needles,
    const std::vector<char>& target_back_needles,
    const std::vector<char>& target_front_needles,
    const cb::ArtinBraid& source_braid,
    const std::vector<SlackConstraint>& slack_constraints
) :
    machine(machine),
    source_back_needles(source_back_needles),
    source_front_needles(source_front_needles),
    target_back_needles(target_back_needles),
    target_front_needles(target_front_needles),
    source_braid(source_braid),
    slack_constraints(slack_constraints)
{
    target_needle_count = 0;

    for (char x : target_back_needles) {
        if (x > 0) {
            target_needle_count++;
        }
    }

    for (char x : target_front_needles) {
        if (x > 0) {
            target_needle_count++;
        }
    }

}
TestCase::TestCase(const TestCase& other) :
    machine(other.machine),
    source_back_needles(other.source_back_needles),
    source_front_needles(other.source_front_needles),
    target_back_needles(other.target_back_needles),
    target_front_needles(other.target_front_needles),
    source_braid(other.source_braid),
    slack_constraints(other.slack_constraints),
    target_needle_count(other.target_needle_count)
{ }

search::SearchResult<KnittingState> TestCase::test(
    bool canonicalize, unsigned int (KnittingState::*h)() const
) {
    KnittingState target(
        machine, target_back_needles, target_front_needles,
        CBraid::ArtinBraid(target_needle_count), slack_constraints
    );
    KnittingState source(
        machine, source_back_needles, source_front_needles,
        source_braid, slack_constraints, &target
    );

    if (canonicalize) {
        return search::a_star(
            source.all_canonical_rackings(), target,
            &KnittingState::canonical_adjacent, h
        );
    }
    else {
        return search::a_star(
            source.all_rackings(), target,
            &KnittingState::adjacent, h
        );
    }
}

search::SearchResult<KnittingState> TestCase::test_id(
    bool canonicalize, unsigned int (KnittingState::*h)() const
) {
    KnittingState target(
        machine, target_back_needles, target_front_needles,
        CBraid::ArtinBraid(target_needle_count), slack_constraints
    );
    KnittingState source(
        machine, source_back_needles, source_front_needles,
        source_braid, slack_constraints, &target
    );

    if (canonicalize) {
        return search::ida_star(
            source.all_canonical_rackings(), target,
            &KnittingState::canonical_adjacent, h
        );
    }
    else {
        return search::ida_star(
            source.all_rackings(), target,
            &KnittingState::adjacent, h
        );
    }
}

search::SearchResult<KnittingStateLM21> TestCase::test(
    bool canonicalize, unsigned int (KnittingStateLM21::*h)() const
) {
    KnittingStateLM21 target(
        machine, target_back_needles, target_front_needles,
        CBraid::ArtinBraid(source_braid.Index()), slack_constraints
    );
    KnittingStateLM21 source(
        machine, source_back_needles, source_front_needles,
        source_braid, slack_constraints, &target
    );

    if (canonicalize) {
        return search::a_star(
            source.all_canonical_rackings(), target,
            &KnittingStateLM21::canonical_adjacent, h
        );
    }
    else {
        return search::a_star(
            source.all_rackings(), target,
            &KnittingStateLM21::adjacent, h
        );
    }
}
std::ostream& operator<<(std::ostream& o, const knitting::TestCase& state) {
    KnittingState target(
        state.machine, state.target_back_needles, state.target_front_needles,
        CBraid::ArtinBraid(state.target_needle_count), state.slack_constraints
    );
    KnittingState source(
        state.machine, state.source_back_needles, state.source_front_needles,
        state.source_braid, state.slack_constraints
    );

    o << source << "\n" << target;

    return o;
}

std::vector<char> flat_bed(
    KnittingMachine machine, int loop_count, int max_stack, std::mt19937& rng
) {
    std::vector<char> bed(machine.width, 0);
    std::uniform_int_distribution<char> loop_skip(1, 2);

    int loops_remaining = loop_count;
    char i = loop_skip(rng)-1;
    for (; i < machine.width; i += loop_skip(rng)) {
        bed[i]++;
        loops_remaining--;

        if (loops_remaining == 0) {
            break;
        }
    }

    std::uniform_int_distribution<char> loop_loc_dist(0, std::min(i, (char)(machine.width-1)));
    while (loops_remaining > 0) {
        int j = loop_loc_dist(rng);

        if (bed[j] < max_stack) {
            bed[j]++;
            loops_remaining--;
        }
    }

    return bed;
}

TestCase flat_lace (KnittingMachine machine, int loop_count, int max_stack, std::mt19937& rng) {

    std::vector<char> empty_bed(machine.width, 0);
    std::vector<char> source_bed = flat_bed(machine, loop_count, 1, rng);
    std::vector<char> target_bed = flat_bed(machine, loop_count, max_stack, rng);
    std::vector<SlackConstraint> slack_constraints;

    char prev = -1;
    for (char i = 0; i < machine.width; i++) {
        if (source_bed[i] > 0) {
            if (prev >= 0) {
                slack_constraints.emplace_back(NeedleLabel(true, prev), NeedleLabel(true, i), 2);
            }

            prev = i;
        }
    }

    return TestCase(
        machine,
        empty_bed,
        source_bed,
        empty_bed,
        target_bed,
        CBraid::ArtinBraid(loop_count),
        slack_constraints
    );
}

TestCase simple_tube (
    KnittingMachine machine,
    int back_loop_count,
    int front_loop_count,
    std::mt19937& rng
) {
    throw NotImplemented();
}

ResultAggregate::ResultAggregate() :
    search_tree_size(0),
    seconds_taken(0)
{ }

}
