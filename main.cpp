#include "knitting.h"
#include "search.h"
#include <iostream>

namespace cb = CBraid;
namespace kn = knitting;

int main () {
    kn::KnittingMachine m (5, -4, 4);

    cb::ArtinBraid b(5);
    b.LeftMultiply(cb::ArtinFactor(5, 1));

    std::vector<kn::LoopSlackConstraint> loop_slack_constraints = {
        kn::LoopSlackConstraint(0, 1, 2),
        kn::LoopSlackConstraint(1, 2, 2),
        kn::LoopSlackConstraint(2, 3, 2),
        kn::LoopSlackConstraint(3, 4, 2)
    };
    std::vector<kn::SlackConstraint> slack_constraints = {
        kn::SlackConstraint(kn::NeedleLabel(true, 0), kn::NeedleLabel(true, 1), 2),
        kn::SlackConstraint(kn::NeedleLabel(true, 1), kn::NeedleLabel(true, 2), 2),
        kn::SlackConstraint(kn::NeedleLabel(true, 2), kn::NeedleLabel(true, 3), 2),
        kn::SlackConstraint(kn::NeedleLabel(true, 3), kn::NeedleLabel(true, 4), 2)
    };

    using State = kn::KnittingState;

    State target(m,
        { 0, 0, 0, 0, 0 },
        { 1, 1, 1, 1, 1 },
        cb::ArtinBraid(5), slack_constraints
    );

    State source(m,
        { 0, 0, 0, 0, 0 },
        { 1, 1, 1, 1, 1 },
        b, slack_constraints, &target
    );

    auto result = search::a_star(
        source.all_rackings(), target,
        &State::canonical_adjacent, &State::log_heuristic
    );

    std::cout << "Tree size: " << result.search_tree_size << std::endl;
    std::cout << "Path length: " << result.path_length << std::endl;
    std::cout << "Seconds taken: " << result.seconds_taken << std::endl;
    std::cout << "Nodes per second: " << result.search_tree_size / result.seconds_taken << std::endl;
    if (result.path.size() > 0) {
        std::cout << "  rack " << result.path.back().prev.racking() << std::endl;
    }
    for (const auto& t : result.path) {
        std::cout << "  " << t.command << std::endl;
    }

    return 0;
}
