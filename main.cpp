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
        b, {}, &target);

    auto result = search::a_star(
        source, target, &kn::KnittingState::adjacent, &kn::KnittingState::braid_heuristic
    );

    std::cout << "Tree size: " << result.search_tree_size << std::endl;
    for (const auto& t : result.path) {
        std::cout << t.command << std::endl;
    }

    return 0;
}
