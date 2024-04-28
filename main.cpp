#include "knitting.h"
#include "search.h"
#include "testgen.h"
#include "prebuilt.h"
#include <iostream>
#include <random>

namespace cb = CBraid;
namespace kn = knitting;

int main () {
    prebuilt::construct_table(8, -5, 5);

    /* Example manual usage */
    // kn::KnittingMachine m (5, -4, 4);
    //
    // cb::ArtinBraid b(5);
    // b.LeftMultiply(cb::ArtinFactor(5, 1));
    //
    // std::vector<kn::SlackConstraint> slack_constraints = {
    //     kn::SlackConstraint(kn::NeedleLabel(true, 0), kn::NeedleLabel(true, 1), 2),
    //     kn::SlackConstraint(kn::NeedleLabel(true, 1), kn::NeedleLabel(true, 2), 2),
    //     kn::SlackConstraint(kn::NeedleLabel(true, 2), kn::NeedleLabel(true, 3), 2),
    //     kn::SlackConstraint(kn::NeedleLabel(true, 3), kn::NeedleLabel(true, 4), 2)
    // };
    //
    // using State = kn::KnittingStateLM21;
    //
    // State target(m,
    //     { 0, 0, 0, 0, 0 },
    //     { 1, 1, 1, 1, 1 },
    //     cb::ArtinBraid(5), slack_constraints
    // );
    //
    // State source(m,
    //     { 0, 0, 0, 0, 0 },
    //     { 1, 1, 1, 1, 1 },
    //     b, slack_constraints, &target
    // );
    //
    // auto result = search::a_star(
    //     source.all_rackings(), target,
    //     &State::canonical_adjacent, &State::log_heuristic
    // );
    //
    // std::cout << "Tree size: " << result.search_tree_size << std::endl;
    // std::cout << "Path length: " << result.path_length << std::endl;
    // std::cout << "Seconds taken: " << result.seconds_taken << std::endl;
    // std::cout << "Nodes/second: " << result.search_tree_size / result.seconds_taken << std::endl;
    // if (result.path.size() > 0) {
    //     std::cout << "rack " << result.path.back().prev.racking() << std::endl;
    // }
    // for (const auto& t : result.path) {
    //     std::cout << t.command << std::endl;
    // }

    // {
    //     kn::KnittingMachine machine (12, -5, 5);
    //
    //     std::mt19937 rng(0);
    //
    //     kn::ResultAggregate aggregate_1;
    //     kn::ResultAggregate aggregate_2;
    //
    //     for (int i = 0; i < 1000; i++) {
    //         kn::TestCase test_case = flat_lace(machine, 8, 3, rng);
    //
    //         auto result_1 = test_case.test(true, &kn::KnittingState::braid_prebuilt_heuristic);
    //         std::cout << result_1.path_length << " "
    //                 << result_1.search_tree_size << " "
    //                 << result_1.seconds_taken << std::endl;
    //
    //         auto result_2 = test_case.test(true, &kn::KnittingStateLM21::braid_prebuilt_heuristic);
    //         std::cout << result_2.path_length << " "
    //                 << result_2.search_tree_size << " "
    //                 << result_2.seconds_taken << std::endl;
    //
    //         std::cout << std::endl;
    //
    //         if (result_1.path_length != result_2.path_length) {
    //             std::cout << "error: i = " << i << std::endl;
    //             return 1;
    //         }
    //
    //         aggregate_1.add_result(result_1);
    //         aggregate_2.add_result(result_2);
    //     }
    //
    //     std::cout << "Totals:\n";
    //     std::cout << aggregate_1.search_tree_size << " " << aggregate_1.seconds_taken << std::endl;
    //     std::cout << aggregate_2.search_tree_size << " " << aggregate_2.seconds_taken << std::endl;
    // }

    {
        kn::KnittingMachine machine (10, -5, 5);

        std::mt19937 rng(0);

        kn::ResultAggregate aggregate_1;
        kn::ResultAggregate aggregate_2;

        for (int i = 0; i < 1000; i++) {
            kn::TestCase test_case = simple_tube(machine, 10, 2, rng);

            // std::cout << test_case << std::endl;

            auto result_1 = test_case.test(true, &kn::KnittingState::braid_prebuilt_heuristic);
            std::cout << result_1.path_length << " "
                    << result_1.search_tree_size << " "
                    << result_1.seconds_taken << std::endl;

            // for (const auto& t : result_1.path) {
            //     std::cout << t.prev << std::endl;
            //     std::cout << t.command << std::endl;
            // }

            auto result_2 = test_case.test_id(true, &kn::KnittingState::braid_prebuilt_heuristic);
            std::cout << result_2.path_length << " "
                    << result_2.search_tree_size << " "
                    << result_2.seconds_taken << std::endl;

            std::cout << std::endl;

            if (result_1.path_length != result_2.path_length) {
                std::cout << "error: i = " << i << std::endl;
                return 1;
            }

            aggregate_1.add_result(result_1);
            aggregate_2.add_result(result_2);
        }

        std::cout << "Totals:\n";
        std::cout << aggregate_1.search_tree_size << " " << aggregate_1.seconds_taken << std::endl;
        std::cout << aggregate_2.search_tree_size << " " << aggregate_2.seconds_taken << std::endl;
    }


    return 0;
}
