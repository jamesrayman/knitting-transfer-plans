#include "cbraid.h"
#include <vector>

#ifndef KNITTING_H
#define KNITTING_H

namespace knitting {
    class KnittingState;
    class KnittingStateLM21;
}

template <>
struct std::hash<knitting::KnittingState> {
    std::size_t operator()(const knitting::KnittingState&) const;
};

template <>
struct std::hash<knitting::KnittingStateLM21> {
    std::size_t operator()(const knitting::KnittingStateLM21&) const;
};

namespace knitting {

namespace cb = CBraid;

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
        KnittingState* = nullptr
    );
    KnittingState(const KnittingState&);

    int racking() const;

    void set_target(KnittingState*);

    int& loop_count(const NeedleLabel&);
    int loop_count(const NeedleLabel&) const;

    NeedleLabel& destination(const NeedleLabel&);
    NeedleLabel destination(const NeedleLabel&) const;

    NeedleLabel needle_with_braid_rank(int) const;
    bool can_transfer(int) const;

    bool transfer(int, bool);
    bool rack(int);

    bool operator==(const KnittingState&) const;
    bool operator!=(const KnittingState&) const;

    KnittingState& operator=(const KnittingState&);

    std::vector<KnittingState> all_rackings();
    std::vector<KnittingState> all_canonical_rackings();

    TransitionIterator adjacent() const;
    TransitionIterator canonical_adjacent() const;

    bool canonicalize();

    unsigned long long offsets() const;

    unsigned int no_heuristic() const;
    unsigned int target_heuristic() const;
    unsigned int braid_heuristic() const;
    unsigned int log_heuristic() const;
    unsigned int prebuilt_heuristic() const;
    unsigned int braid_log_heuristic() const;
    unsigned int braid_prebuilt_heuristic() const;

    friend std::ostream& operator<<(std::ostream&, const KnittingState&);
    friend std::size_t std::hash<KnittingState>::operator()(const KnittingState&) const;
};

class KnittingState::TransitionIterator {
    int racking;
    std::vector<int> xfer_is;
    std::vector<bool> xfer_types; // false => 2 choices; true => 3 choices
    std::vector<int> xfers; // xfer actions: 0 => nothing; 1 => xfer_to_back; 2 => xfer_to_front
    std::string xfer_command;
    bool canonicalize;
    bool good;
    bool done;
    KnittingState next_uncanonical;

    void increment_xfers();
    bool try_next();
public:
    const KnittingState& prev;
    int weight = 1;
    KnittingState next;
    std::string command;

    TransitionIterator(const KnittingState&, bool);

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


class LoopSlackConstraint {
public:
    int loop_1;
    int loop_2;
    int limit;

    LoopSlackConstraint(int, int, int);
    bool respected(NeedleLabel, NeedleLabel, int) const;
};

class KnittingStateLM21 {
public:
    class TransitionIterator;
    class Backpointer;

private:
    KnittingMachine machine;
    cb::ArtinBraid braid;
    std::vector<NeedleLabel> loop_locations;
    const std::vector<LoopSlackConstraint>* slack_constraints;
    KnittingStateLM21* target;

    void calculate_destinations();

public:
    KnittingStateLM21();
    KnittingStateLM21(
        const KnittingMachine,
        const std::vector<int>&,
        const std::vector<int>&,
        const cb::ArtinBraid&,
        const std::vector<LoopSlackConstraint>&,
        KnittingStateLM21* = nullptr
    );
    KnittingStateLM21(const KnittingStateLM21&);

    int racking() const;
    bool needle_empty(NeedleLabel) const;
    int loop_count(NeedleLabel) const;

    void set_target(KnittingStateLM21*);
    bool can_transfer(int) const;

    bool rack(int);
    bool transfer(int, bool);

    bool operator==(const KnittingStateLM21&) const;
    bool operator!=(const KnittingStateLM21&) const;

    KnittingStateLM21& operator=(const KnittingStateLM21&);

    std::vector<KnittingStateLM21> all_rackings();
    std::vector<KnittingStateLM21> all_canonical_rackings();

    TransitionIterator adjacent() const;
    TransitionIterator canonical_adjacent() const;

    bool canonicalize();

    unsigned long long offsets() const;

    unsigned int no_heuristic() const;
    unsigned int target_heuristic() const;
    unsigned int braid_heuristic() const;
    unsigned int log_heuristic() const;
    unsigned int prebuilt_heuristic() const;
    unsigned int braid_log_heuristic() const;
    unsigned int braid_prebuilt_heuristic() const;

    friend std::ostream& operator<<(std::ostream&, const KnittingStateLM21&);
    friend std::size_t std::hash<KnittingStateLM21>::operator()(const KnittingStateLM21&) const;
};

class KnittingStateLM21::TransitionIterator {
    int racking;
    std::vector<int> xfer_is;
    std::vector<bool> xfer_types;
    std::vector<int> xfers;
    std::string xfer_command;
    bool canonicalize;
    bool good;
    bool done;
    KnittingStateLM21 next_uncanonical;

    void increment_xfers();
    bool try_next();
public:
    const KnittingStateLM21& prev;
    int weight;
    KnittingStateLM21 next;
    std::string command;

    TransitionIterator(const KnittingStateLM21&, bool);

    bool has_next();
};

class KnittingStateLM21::Backpointer {
public:
    KnittingStateLM21 prev;
    std::string command;

    Backpointer();
    Backpointer(const KnittingStateLM21&, const std::string&);
    Backpointer(const KnittingStateLM21::Backpointer&);
    KnittingStateLM21::Backpointer& operator=(const KnittingStateLM21::Backpointer&);
};

}

#endif
