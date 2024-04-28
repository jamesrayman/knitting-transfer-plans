#include "cbraid.h"
#include <vector>
#include <random>

#ifndef KNITTING_H
#define KNITTING_H

namespace knitting {
    class KnittingState;
    class KnittingStateLM21;
    class TestCase;
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
    char i;

    NeedleLabel(bool = false, char = -1);
    NeedleLabel(const NeedleLabel&);

    bool operator==(const NeedleLabel&) const;
    bool operator!=(const NeedleLabel&) const;

    NeedleLabel& operator=(const NeedleLabel&);

    int id() const;
    int location(char) const;
    int offset(NeedleLabel) const;

    friend std::ostream& operator<<(std::ostream&, const NeedleLabel&);
};

class Needle {
public:
    NeedleLabel destination;
    char count;

    Needle(char, NeedleLabel = NeedleLabel());
    Needle(const Needle&);
};

class KnittingMachine {
public:
    char width;
    char min_racking;
    char max_racking;
    char racking;

    KnittingMachine(char = 1, char = 0, char = 0, char = 0);
    KnittingMachine(const KnittingMachine&);

    NeedleLabel operator[](char) const;
};

class SlackConstraint {
public:
    NeedleLabel needle_1;
    NeedleLabel needle_2;
    char limit;

    SlackConstraint(NeedleLabel, NeedleLabel, char);
    bool respected(char) const;
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
        const std::vector<char>&,
        const std::vector<char>&,
        const cb::ArtinBraid&,
        const std::vector<SlackConstraint>&,
        KnittingState* = nullptr
    );
    KnittingState(const KnittingState&);

    char racking() const;

    void set_target(KnittingState*);

    char& loop_count(const NeedleLabel&);
    char loop_count(const NeedleLabel&) const;

    NeedleLabel& destination(const NeedleLabel&);
    NeedleLabel destination(const NeedleLabel&) const;

    NeedleLabel needle_with_braid_rank(int) const;
    bool can_transfer(char) const;

    bool transfer(char, bool);
    bool rack(char);

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
    char racking;
    std::vector<char> xfer_is;
    std::vector<bool> xfer_types; // false => 2 choices; true => 3 choices
    std::vector<char> xfers; // xfer actions: 0 => nothing; 1 => xfer_to_back; 2 => xfer_to_front
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
    char loop_1;
    char loop_2;
    char limit;

    LoopSlackConstraint(char, char, char);
    bool respected(NeedleLabel, NeedleLabel, char) const;
};

class KnittingStateLM21 {
public:
    class TransitionIterator;
    class Backpointer;

    friend TestCase simple_tube (
        KnittingMachine machine, int loop_count, int pass_count, std::mt19937& rng
    );

private:
    KnittingMachine machine;
    cb::ArtinBraid braid;
    std::vector<NeedleLabel> loop_locations;
    std::vector<LoopSlackConstraint> slack_constraints;
    KnittingStateLM21* target;
    bool only_contractions;

    void calculate_destinations();

public:
    KnittingStateLM21();
    KnittingStateLM21(
        const KnittingMachine,
        const std::vector<char>&,
        const std::vector<char>&,
        const cb::ArtinBraid&,
        const std::vector<SlackConstraint>&,
        KnittingStateLM21* = nullptr,
        bool = false
    );
    KnittingStateLM21(const KnittingStateLM21&);

    char racking() const;
    bool needle_empty(NeedleLabel) const;
    char loop_count(NeedleLabel) const;

    void set_target(KnittingStateLM21*);
    bool can_transfer(char) const;
    std::vector<char> back_bed() const;
    std::vector<char> front_bed() const;

    bool rack(char);
    bool transfer(char, bool);

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
    char racking;
    std::vector<char> xfer_is;
    std::vector<bool> xfer_types;
    std::vector<char> xfers;
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
    KnittingStateLM21 random(std::mt19937&);
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
