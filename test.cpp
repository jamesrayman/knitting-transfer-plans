#include "knitting.h"
#include "search.h"
#include "prebuilt.h"
#include <iostream>

namespace cb = CBraid;
using namespace knitting;

int main () {
    int test_needle_order_count = 0;
    auto test_needle_order =
    [&test_needle_order_count](const KnittingMachine& machine, const std::string& order) {
        test_needle_order_count++;
        std::string actual;
        for (char i = 0; i < 2*machine.width; i++) {
            NeedleLabel needle = machine[i];
            actual += needle.front ? 'f' : 'b';
            actual += std::to_string(needle.i);
        }
        if (actual != order) {
            std::cout << "test_needle_order #" << test_needle_order_count << " failed\n";
            std::cout << "expected: " << order << "\n";
            std::cout << "actual:   " << actual << "\n\n";
        }
    };

    {
        KnittingMachine machine (4, -2, 2, 0);

        test_needle_order(machine, "b0f0b1f1b2f2b3f3");

        machine.racking = -2;
        test_needle_order(machine, "b0b1b2f0b3f1f2f3");

        machine.racking = 2;
        test_needle_order(machine, "f0f1b0f2b1f3b2b3");
    }

    {
        KnittingMachine machine (3, -2, 2, 0);
        KnittingState state(machine,
            { 0, 0, 0 },
            { 1, 1, 1 },
            cb::ArtinBraid(3), {
                SlackConstraint(NeedleLabel(true, 0), NeedleLabel(true, 1), 1),
                SlackConstraint(NeedleLabel(true, 1), NeedleLabel(true, 2), 1),
            }
        );

        state.transfer(0, false);
        if (state.rack(-1)) {
            std::cout << "slack test failed.\n";
        }

    }

    // Counterexample to "always optimal to stack loops if it doesn't
    // result in an unsolvable state"
    {
        cb::ArtinFactor f(7, 0, false);
        f[1] = 2;
        f[2] = 1;

        KnittingMachine machine (6, -5, 5, 0);
        KnittingState source(machine,
            { 0, 0, 1, 0, 1, 1 },
            { 0, 1, 0, 1, 1, 1 },
            cb::ArtinBraid(f), {
                SlackConstraint(NeedleLabel(false, 2), NeedleLabel(false, 4), 2),
                SlackConstraint(NeedleLabel(true, 4), NeedleLabel(true, 5), 1)
            }
        );
        KnittingState target = source;

        if (!target.rack(-2)) std::cout << "error: target.rack\n";
        if (!target.transfer(1, false)) std::cout << "error: target.transfer\n";
        if (!target.rack(0)) std::cout << "error: target.rack\n";
        if (!target.transfer(4, true)) std::cout << "error: target.transfer\n";

        source.set_target(&target);

        int opt = search::a_star(
            source.all_rackings(), target,
            &KnittingState::adjacent, &KnittingState::braid_heuristic
        ).path_length;
        if (opt != 2) std::cout << "error: opt = " << opt << "\n";

        source.transfer(4, false);
        int opt_back = search::a_star(
            source.all_rackings(), target,
            &KnittingState::adjacent, &KnittingState::braid_heuristic, 2
        ).path_length;
        if (opt_back != -1) std::cout << "error: opt_back\n";

        source.transfer(4, true);
        int opt_front = search::a_star(
            source.all_rackings(), target,
            &KnittingState::adjacent, &KnittingState::braid_heuristic, 2
        ).path_length;
        if (opt_front != -1) std::cout << "error: opt_front\n";
    }

    return 0;
}
