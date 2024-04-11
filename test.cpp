#include "knitting.h"
#include <iostream>

namespace cb = CBraid;
using namespace knitting;

int main () {
    int test_loop_order_count = 0;
    auto test_loop_order =
    [&test_loop_order_count](const KnittingState& state, const std::vector<int>& order) {
        test_loop_order_count++;
        int i = 0;
        for (auto it = state.begin(); it != state.end(); it++, i++) {
            if (*it != order[i]) {
                std::cout << "loop order test " << test_loop_order_count << " failed. "
                             "expected " << order[i] << " at " << i << ", got " << *it << ".\n";
                return;
            }
        }
    };

    {
        KnittingMachine machine (3, -2, 2);
        KnittingState state (machine, { 1, 1, 1 }, { 1, 1, 1 });

        state.rack(-1);
        test_loop_order(state, { 0, 2, 1, 4, 3, 5 });

        state.rack(1);
        test_loop_order(state, { 1, 0, 3, 2, 5, 4 });
    }

    return 0;
}
