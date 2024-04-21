#include "knitting.h"
#include "search.h"
#include <iostream>

namespace cb = CBraid;
namespace kn = knitting;

int main () {
    kn::KnittingMachine m (5, -4, 4);

    cb::ArtinBraid b(5);
    b.LeftMultiply(cb::ArtinFactor(5, 1));

    kn::KnittingState target(m,
        { 0, 0, 0, 0, 0 },
        { 1, 1, 1, 1, 1 },
        cb::ArtinBraid(5), {}
    );
    kn::KnittingState source(m,
        { 0, 0, 0, 0, 0 },
        { 1, 1, 1, 1, 1 },
        b, {
            kn::SlackConstraint(kn::NeedleLabel(true, 0), kn::NeedleLabel(true, 1), 1),
            kn::SlackConstraint(kn::NeedleLabel(true, 1), kn::NeedleLabel(true, 2), 1),
            kn::SlackConstraint(kn::NeedleLabel(true, 2), kn::NeedleLabel(true, 3), 1),
            kn::SlackConstraint(kn::NeedleLabel(true, 3), kn::NeedleLabel(true, 4), 1)
        }, &target
    );

    auto result = search::a_star(
        source.all_rackings(), target, &kn::KnittingState::adjacent, &kn::KnittingState::braid_heuristic
    );

    std::cout << "Tree size: " << result.search_tree_size << std::endl;
    std::cout << "Path length: " << result.path_length << std::endl;
    for (const auto& t : result.path) {
        std::cout << t.command << std::endl;
    }

    return 0;
}
