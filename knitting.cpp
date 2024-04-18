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

bool NeedleLabel::operator==(const NeedleLabel& other) const {
    return front == other.front && i == other.i;
}
bool NeedleLabel::operator!=(const NeedleLabel& other) const {
    return !(*this == other);
}

NeedleLabel& NeedleLabel::operator=(const NeedleLabel& other) {
    front = other.front;
    i = other.i;

    return *this;
}

int NeedleLabel::id() const {
    return front ? 2*i + 1 : 2*i;
}
int NeedleLabel::location(int racking) const {
    return front ? i : i + racking;
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

SlackConstraint::SlackConstraint(NeedleLabel needle_1, NeedleLabel needle_2, int limit) :
    needle_1(needle_1),
    needle_2(needle_2),
    limit(limit)
{ }

bool SlackConstraint::respected(int racking) const {
    return abs(needle_1.location(racking) - needle_2.location(racking)) <= limit;
}
void SlackConstraint::replace(NeedleLabel from, NeedleLabel to) {
    if (needle_1 == from) {
        needle_1 = to;
    }
    if (needle_2 == from) {
        needle_2 = to;
    }
}

KnittingState::KnittingState() :
    braid(1)
{ }

KnittingState::KnittingState(
    const KnittingMachine machine,
    const Bed& back_needles,
    const Bed& front_needles,
    const cb::ArtinBraid& braid,
    const std::vector<SlackConstraint>& slack_constraints,
    const KnittingState* target
) :
    machine(machine),
    back_needles(back_needles),
    front_needles(front_needles),
    braid(braid),
    slack_constraints(slack_constraints),
    target(target)
{ }

KnittingState::KnittingState(const KnittingState& other) :
    machine(other.machine),
    back_needles(other.back_needles),
    front_needles(other.front_needles),
    braid(other.braid),
    slack_constraints(other.slack_constraints),
    target(other.target)
{ }

int& KnittingState::loop_count(const NeedleLabel& n) {
    return n.front ? front_needles[n.i] : back_needles[n.i];
}
int KnittingState::loop_count(const NeedleLabel& n) const {
    return n.front ? front_needles[n.i] : back_needles[n.i];
}

bool KnittingState::transfer(int loc, bool to_back) {
    NeedleLabel back_needle = NeedleLabel(false, loc - machine.racking);
    NeedleLabel front_needle = NeedleLabel(true, loc);

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

    if (loop_count(front_needle) > 0 && loop_count(back_needle) > 0) {
        if (!braid.CanMerge(j+1)) {
            return false;
        }
        braid = braid.Merge(j+1);
    }

    if (to_back) {
        loop_count(back_needle) += loop_count(front_needle);
        loop_count(front_needle) = 0;
    }
    else {
        loop_count(front_needle) += loop_count(back_needle);
        loop_count(back_needle) = 0;
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

    for (const SlackConstraint& constraint : slack_constraints) {
        if (!constraint.respected(new_racking)) {
            return false;
        }
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

KnittingState::TransitionIterator::TransitionIterator(
    const KnittingState& prev
) :
    prev(prev),
    next(prev)
{
    racking = prev.machine.min_racking;
    xfer_i = std::max(0, prev.machine.racking);
    to_front = false;
    good = false;
}

bool KnittingState::TransitionIterator::try_next() {
    if (good) {
        next = prev;
    }

    if (racking <= prev.machine.max_racking) {
        weight = 1;
        command = "rack " + std::to_string(racking);

        good = next.rack(racking);
        racking++;
    }
    else if (to_front) {
        weight = 0;
        command = "xfer_to_front " + std::to_string(xfer_i);

        good = next.transfer(xfer_i, to_front);
        to_front = false;
        xfer_i++;
    }
    else {
        weight = 0;
        command = "xfer_to_back " + std::to_string(xfer_i);

        good = next.transfer(xfer_i, to_front);
        to_front = true;
    }
    return good;
}
bool KnittingState::TransitionIterator::has_next() {
    while (xfer_i < prev.machine.width + std::min(0, prev.machine.racking)) {
        if (try_next()) {
            return true;
        }
    }
    return false;
}

KnittingState::Backpointer::Backpointer() {}

KnittingState::Backpointer::Backpointer(
    const KnittingState& prev, const std::string& command
) :
    prev(prev),
    command(command)
{ }

KnittingState::Backpointer::Backpointer(
    const KnittingState::Backpointer& other
) :
    prev(other.prev),
    command(other.command)
{ }

KnittingState::Backpointer& KnittingState::Backpointer::operator=(const KnittingState::Backpointer& other) {
    prev = other.prev;
    command = other.command;
    return *this;
}

KnittingState::TransitionIterator KnittingState::adjacent() const {
    return KnittingState::TransitionIterator(*this);
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

std::size_t hash_combine(std::size_t x, std::size_t y) {
    return x ^ (y + 0x5e7a3ddcc8414e72 + (x << 12) + (x >> 3));
}

std::size_t std::hash<knitting::KnittingState>::operator()(
    const knitting::KnittingState& state
) const {
    size_t h = 0xf0e35c6e3c319f8;
    h = hash_combine(h, state.machine.racking);

    for (auto x : state.back_needles) {
        h = hash_combine(h, x);
    }
    for (auto x : state.front_needles) {
        h = hash_combine(h, x);
    }

    h = hash_combine(h, state.braid.Hash());

    return h;
}
