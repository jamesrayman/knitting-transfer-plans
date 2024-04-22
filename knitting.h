#include "cbraid.h"
#include <vector>
#include <unordered_set>

#ifndef KNITTING_H
#define KNITTING_H

namespace knitting {
    class KnittingState;
}

template <>
struct std::hash<knitting::KnittingState> {
    std::size_t operator()(const knitting::KnittingState&) const;
};


namespace knitting {

namespace cb = CBraid;

class NotImplemented { };
class InvalidKnittingMachineException { };
class InvalidRackingException { };
class InvalidTargetStateException { };
class InvalidBraidRankException { };

class NeedleLabel {
public:
    bool front;
    int i;

    NeedleLabel(bool = false, int = -1);
    NeedleLabel(const NeedleLabel&);

    bool operator==(const NeedleLabel&) const;
    bool operator!=(const NeedleLabel&) const;

    NeedleLabel& operator=(const NeedleLabel&);

    int id() const;
    int location(int) const;
    int offset(NeedleLabel) const;

    friend std::ostream& operator<<(std::ostream&, const NeedleLabel&);
};

class Needle {
public:
    NeedleLabel destination;
    int count;

    Needle(int, NeedleLabel = NeedleLabel());
    Needle(const Needle&);
};

class KnittingMachine {
public:
    int width;
    int min_racking;
    int max_racking;
    int racking;

    KnittingMachine(int = 1, int = 0, int = 0, int = 0);
    KnittingMachine(const KnittingMachine&);

    NeedleLabel operator[](int) const;
};

class SlackConstraint {
public:
    NeedleLabel needle_1;
    NeedleLabel needle_2;
    int limit;

    SlackConstraint(NeedleLabel, NeedleLabel, int);
    bool respected(int) const;
    void replace(NeedleLabel, NeedleLabel);

    SlackConstraint& operator=(const SlackConstraint&);
};

class KnittingState {
public:
    using Bed = std::vector<Needle>;

    class TransitionIterator;
    class CanonicalTransitionIterator;
    class Backpointer;
private:
    KnittingMachine machine;
    Bed back_needles;
    Bed front_needles;
    cb::ArtinBraid braid;
    std::vector<SlackConstraint> slack_constraints;
    KnittingState* target;

    void calculate_destinations();
public:
    KnittingState();
    KnittingState(
        const KnittingMachine,
        const std::vector<int>&,
        const std::vector<int>&,
        const cb::ArtinBraid&,
        const std::vector<SlackConstraint>&,
        KnittingState* target = nullptr
    );
    KnittingState(const KnittingState&);

    int racking() const;

    void set_target(KnittingState*);

    int& loop_count(const NeedleLabel&);
    int loop_count(const NeedleLabel&) const;

    NeedleLabel& destination(const NeedleLabel&);
    NeedleLabel destination(const NeedleLabel&) const;

    NeedleLabel needle_with_braid_rank(int) const;

    bool transfer(int, bool);
    bool rack(int);

    bool operator==(const KnittingState&) const;
    bool operator!=(const KnittingState&) const;

    KnittingState& operator=(const KnittingState&);

    std::vector<KnittingState> all_rackings();
    std::vector<KnittingState> all_canonical_rackings();

    TransitionIterator adjacent() const;
    CanonicalTransitionIterator canonical_adjacent() const;

    void canonicalize();

    std::unordered_set<int> offsets() const;

    int no_heuristic() const;
    int target_heuristic() const;
    int braid_heuristic() const;
    int log_heuristic() const;
    int prebuilt_heuristic() const;
    int braid_log_heuristic() const;
    int braid_prebuilt_heuristic() const;

    friend std::ostream& operator<<(std::ostream&, const KnittingState&);
    friend std::size_t std::hash<KnittingState>::operator()(const KnittingState&) const;
};

class KnittingState::TransitionIterator {
    int racking;
    int xfer_i;
    bool to_front;
    bool good;

    bool try_next();
public:
    const KnittingState& prev;
    int weight;
    KnittingState next;
    std::string command;

    TransitionIterator(const KnittingState&);

    bool has_next();
};

class KnittingState::Backpointer {
public:
    KnittingState prev;
    std::string command;

    Backpointer();
    Backpointer(const KnittingState&, const std::string&);
    Backpointer(const KnittingState::Backpointer&);
    KnittingState::Backpointer& operator=(const KnittingState::Backpointer&);
};

class KnittingState::CanonicalTransitionIterator {

};

}

#endif
