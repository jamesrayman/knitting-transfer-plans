#include "knitting.h"
#include "search.h"
#include <iostream>

namespace cb = CBraid;
namespace kn = knitting;

int main () {
    kn::KnittingMachine m (3, -2, 2);

    cb::ArtinBraid b(3);
    b.LeftMultiply(cb::ArtinFactor(3, 1));

    kn::KnittingState target(m, { 0, 0, 0 }, { 1, 1, 1 }, cb::ArtinBraid(3));
    kn::KnittingState source(m, { 0, 0, 0 }, { 1, 1, 1 }, b, &target);

    auto path = search::a_star(
        source, target, &kn::KnittingState::adjacent, &kn::KnittingState::braid_heuristic
    );

    for (const auto& t : path) {
        std::cout << t.command << std::endl;
    }

    return 0;
}
