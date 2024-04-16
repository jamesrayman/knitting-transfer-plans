#include "cbraid.h"
#include <vector>

std::size_t combine_hash(std::size_t, std::size_t);

namespace knitting {
    class KnittingState;
}

template <>
struct std::hash<knitting::KnittingState> {
    std::size_t operator()(const knitting::KnittingState&) const;
};


namespace knitting {

namespace cb = CBraid;

class InvalidKnittingMachineException { };
class InvalidRackingException { };

class NeedleLabel {
public:
    const bool front;
    const int i;

    NeedleLabel(bool, int);
    NeedleLabel(const NeedleLabel&);

    int id() const;
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

class KnittingState {
public:
    using Bed = std::vector<int>;

    class Transition;
private:
    KnittingMachine machine;
    Bed back_needles;
    Bed front_needles;
    cb::ArtinBraid braid;
    const KnittingState* target;

public:

    KnittingState();
    KnittingState(
        const KnittingMachine,
        const Bed&,
        const Bed&,
        const cb::ArtinBraid&,
        const KnittingState* target = nullptr
    );
    KnittingState(const KnittingState&);

    int loop_count(const NeedleLabel&);

    bool transfer(int, bool);
    bool rack(int);

    bool operator==(const KnittingState&) const;
    bool operator!=(const KnittingState&) const;

    KnittingState& operator=(const KnittingState&);

    std::vector<Transition> adjacent() const;

    int no_heuristic() const;
    int target_heuristic() const;
    int braid_heuristic() const;

    friend std::ostream& operator<<(std::ostream&, const KnittingState&);
    friend std::size_t std::hash<KnittingState>::operator()(const KnittingState&) const;
};

class KnittingState::Transition {
public:
    KnittingState prev;
    int weight;
    KnittingState next;
    std::string command;

    Transition();
    Transition(const KnittingState&, int, const KnittingState&, const std::string&);
    Transition(const KnittingState::Transition&);
    KnittingState::Transition& operator=(const KnittingState::Transition&);
};


}
