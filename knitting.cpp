#include "knitting.h"
#include "cbraid.h"
#include "braiding.h"

namespace knitting {

NeedleLabel::NeedleLabel(bool front, int i) :
     front(front),
     i(i)
{ }

NeedleLabel::NeedleLabel(const NeedleLabel& other) :
     front(other.front),
     i(other.i)
{ }

int NeedleLabel::id() const {
    return front ? 2*i + 1 : 2*i;
}

KnittingMachine::KnittingMachine(int width, int min_racking, int max_racking, int racking) :
    width(width),
    min_racking(min_racking),
    max_racking(max_racking),
    racking(racking)
{
    if (max_racking >= width || min_racking <= -width) {
        throw InvalidKnittingMachineException();
    }
    if (max_racking < racking || min_racking > racking) {
        throw InvalidRackingException();
    }
}

KnittingMachine::KnittingMachine(const KnittingMachine& other) :
    width(other.width),
    min_racking(other.min_racking),
    max_racking(other.max_racking),
    racking(other.racking)
{ }

NeedleLabel KnittingMachine::operator[](int i) const {
    if (i < abs(racking)) {
        return NeedleLabel(racking > 0, i);
    }
    if (i >= 2*width - abs(racking)) {
        return NeedleLabel(racking < 0, i - width);
    }
    i -= abs(racking);
    if (i % 2 == 0) {
        if (racking > 0) {
            return NeedleLabel(false, i/2);
        }
        else {
            return NeedleLabel(false, i/2 - racking);
        }
    }
    else {
        if (racking > 0) {
            return NeedleLabel(true, i/2 + racking);
        }
        else {
            return NeedleLabel(true, i/2);
        }
    }
}

KnittingState::KnittingState() :
    braid(1)
{}

KnittingState::KnittingState(
    const KnittingMachine machine,
    const Bed& back_needles,
    const Bed& front_needles,
    const cb::ArtinBraid& braid,
    const KnittingState* target
) :
    machine(machine),
    back_needles(back_needles),
    front_needles(front_needles),
    braid(braid),
    target(target)
{ }

KnittingState::KnittingState(const KnittingState& other) :
    machine(other.machine),
    back_needles(other.back_needles),
    front_needles(other.front_needles),
    braid(other.braid),
    target(other.target)
{ }

int KnittingState::loop_count(const NeedleLabel& n) {
    return n.front ? front_needles[n.i] : back_needles[n.i];
}

bool KnittingState::transfer(int loc, bool to_back) {
    auto& back_needle = back_needles[loc - machine.racking];
    auto& front_needle = front_needles[loc];

    // find which needle this is
    int j = 0;
    for (int i = 0; i < 2*machine.width; i++) {
        NeedleLabel needle = machine[i];
        if (!needle.front && needle.i == loc - machine.racking) {
            break;
        }
        if (loop_count(needle) > 0) {
            j++;
        }
    }

    if (front_needle > 0 && back_needle > 0) {
        if (!braid.CanMerge(j+1)) {
            return false;
        }
        braid = braid.Merge(j+1);
    }

    if (to_back) {
        back_needle += front_needle;
        front_needle = 0;
    }
    else {
        front_needle += back_needle;
        back_needle = 0;
    }

    return true;
}

bool KnittingState::rack(int new_racking) {
    if (new_racking > machine.max_racking || new_racking < machine.min_racking) {
        return false;
    }
    if (new_racking == machine.racking) {
        return true;
    }

    cb::ArtinFactor f(braid.Index(), cb::ArtinFactor::Uninitialize, new_racking < machine.racking);
    std::vector<int> needle_positions (2*machine.width);

    for (int i = 0, j = 0; i < 2*machine.width; i++) {
        NeedleLabel needle = machine[i];
        if (loop_count(needle) > 0) {
            needle_positions[needle.id()] = j;
            j++;
        }
    }
    machine.racking = new_racking;
    for (int i = 0, j = 0; i < 2*machine.width; i++) {
        NeedleLabel needle = machine[i];
        if (loop_count(needle) > 0) {
            f[j + 1] = needle_positions[needle.id()] + 1;
            j++;
        }
    }

    braid.LeftMultiply(f);
    braid.MakeMCF();

    return true;
}

bool KnittingState::operator==(const KnittingState& other) const {
    return machine.racking == other.machine.racking &&
           front_needles == other.front_needles &&
           back_needles == other.back_needles &&
           braid == other.braid;
}
bool KnittingState::operator!=(const KnittingState& other) const {
    return !(*this == other);
}

KnittingState& KnittingState::operator=(const KnittingState& other) {
    machine.racking = other.machine.racking;
    front_needles = other.front_needles;
    back_needles = other.back_needles;
    braid = other.braid;

    return *this;
}

KnittingState::Transition::Transition() {}

KnittingState::Transition::Transition(
    const KnittingState& prev, int weight, const KnittingState& next, const std::string& command
) :
    prev(prev),
    weight(weight),
    next(next),
    command(command)
{ }

KnittingState::Transition::Transition(
    const KnittingState::Transition& other
) :
    prev(other.prev),
    weight(other.weight),
    next(other.next),
    command(other.command)
{ }

KnittingState::Transition& KnittingState::Transition::operator=(const KnittingState::Transition& other) {
    prev = other.prev;
    weight = other.weight;
    next = other.next;
    command = other.command;
    return *this;
}

std::vector<KnittingState::Transition> KnittingState::adjacent() const {
    std::vector<KnittingState::Transition> adj;

    for (int r = machine.min_racking; r <= machine.max_racking; r++) {
        KnittingState next = *this;

        if (next.rack(r)) {
            adj.emplace_back(*this, 1, next, "rack " + std::to_string(r));
        }
    }

    for (int i = std::max(0, machine.racking); i < machine.width + std::min(0, machine.racking); i++) {
        KnittingState next_xfer_to_back = *this;
        KnittingState next_xfer_to_front = *this;

        if (next_xfer_to_back.transfer(i, true)) {
            adj.emplace_back(
                *this, 0, next_xfer_to_back,
                "xfer f." + std::to_string(i+1) + " b." + std::to_string(i-machine.racking+1)
            );
        }
        if (next_xfer_to_front.transfer(i, false)) {
            adj.emplace_back(
                *this, 0, next_xfer_to_front,
                "xfer b." + std::to_string(i-machine.racking+1) + " f." + std::to_string(i+1)
            );
        }
    }

    return adj;
}

int KnittingState::no_heuristic() const {
    return 0;
}

int KnittingState::target_heuristic() const {
    if (target == nullptr) {
        return 0;
    }
    return *this == *target ? 0 : 1;
}

int KnittingState::braid_heuristic() const {
    return std::max(target_heuristic(), int(braid.FactorList.size()));
}

std::ostream& operator<<(std::ostream& o, const knitting::KnittingState& state) {
    o << state.machine.racking << " [";
    for (int i = 0; i < state.machine.width; i++) {
        if (i > 0) {
            o << " ";
        }
        o << state.back_needles[i];
    }
    o << "] [";
    for (int i = 0; i < state.machine.width; i++) {
        if (i > 0) {
            o << " ";
        }
        o << state.front_needles[i];
    }
    o << "] ";
    o << state.braid;

    return o;
}


}

std::size_t combine_hash(std::size_t x, std::size_t y) {
    return std::hash<std::size_t>()(127*x ^ y);
}

std::size_t std::hash<knitting::KnittingState>::operator()(
    const knitting::KnittingState& state
) const {
    std::size_t h = 255 + state.machine.racking;

    for (auto x : state.back_needles) {
        h = combine_hash(h, x);
    }
    for (auto x : state.front_needles) {
        h = combine_hash(h, x);
    }

    h = combine_hash(h, state.braid.FactorList.size());

    return h;
}
