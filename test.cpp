#include "knitting.h"
#include "search.h"
#include <iostream>

namespace cb = CBraid;
using namespace knitting;

int main () {
    int test_needle_order_count = 0;
    auto test_needle_order =
    [&test_needle_order_count](const KnittingMachine& machine, const std::string& order) {
        test_needle_order_count++;
        std::string actual;
        for (int i = 0; i < 2*machine.width; i++) {
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

    return 0;
}
