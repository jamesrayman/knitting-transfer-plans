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

Needle::Needle(int count, NeedleLabel destination) :
    count(count),
    destination(destination)
{ }
Needle::Needle(const Needle& other) :
    count(other.count),
    destination(other.destination)
{ }

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
SlackConstraint& SlackConstraint::operator=(const SlackConstraint& other) {
    needle_1 = other.needle_1;
    needle_2 = other.needle_2;
    limit = other.limit;
    return *this;
}

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
    const std::vector<int>& back_loop_counts,
    const std::vector<int>& front_loop_counts,
    const cb::ArtinBraid& braid,
    const std::vector<SlackConstraint>& slack_constraints,
    KnittingState* target
) :
    machine(machine),
    braid(braid),
    slack_constraints(slack_constraints),
    target(target)
{
    for (int i = 0; i < machine.width; i++) {
        back_needles.emplace_back(back_loop_counts[i]);
        front_needles.emplace_back(front_loop_counts[i]);
    }
}

KnittingState::KnittingState(const KnittingState& other) :
    machine(other.machine),
    back_needles(other.back_needles),
    front_needles(other.front_needles),
    braid(other.braid),
    slack_constraints(other.slack_constraints),
    target(other.target)
{ }


void KnittingState::set_target(KnittingState* t) {
    target = t;
}

int& KnittingState::loop_count(const NeedleLabel& n) {
    return n.front ? front_needles[n.i].count : back_needles[n.i].count;
}
int KnittingState::loop_count(const NeedleLabel& n) const {
    return n.front ? front_needles[n.i].count : back_needles[n.i].count;
}

bool KnittingState::transfer(int loc, bool to_front) {
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

    if (to_front) {
        loop_count(front_needle) += loop_count(back_needle);
        loop_count(back_needle) = 0;
        for (auto& constraint : slack_constraints) {
            constraint.replace(back_needle, front_needle);
        }
    }
    else {
        loop_count(back_needle) += loop_count(front_needle);
        loop_count(front_needle) = 0;
        for (auto& constraint : slack_constraints) {
            constraint.replace(front_needle, back_needle);
        }
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
    if (machine.racking != other.machine.racking) {
        return false;
    }

    for (int i = 0; i < machine.width; i++) {
        if (
            back_needles[i].count != other.back_needles[i].count ||
            front_needles[i].count != other.front_needles[i].count
        ) {
            return false;
        }
    }

    return braid == other.braid;
}
bool KnittingState::operator!=(const KnittingState& other) const {
    return !(*this == other);
}

KnittingState& KnittingState::operator=(const KnittingState& other) {
    machine.racking = other.machine.racking;
    front_needles = other.front_needles;
    back_needles = other.back_needles;
    braid = other.braid;
    slack_constraints = other.slack_constraints;

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
    if (good) { // if `good` is set, it means the previous transition
                // worked and we need to reset `next`.
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
        if (prev.target != nullptr && next == *prev.target) {
            weight = 1;
        }
        to_front = false;
        xfer_i++;
    }
    else {
        weight = 0;
        command = "xfer_to_back " + std::to_string(xfer_i);

        good = next.transfer(xfer_i, to_front);
        if (prev.target != nullptr && next == *prev.target) {
            weight = 1;
        }
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

std::vector<KnittingState> KnittingState::all_rackings() {
    std::vector<KnittingState> v;
    int old_racking = machine.racking;

    for (int r = machine.min_racking; r <= machine.max_racking; r++) {
        if (rack(r)) {
            v.push_back(*this);
        }
    }
    rack(old_racking);

    return v;
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
        o << state.back_needles[i].count;
    }
    o << "] [";
    for (int i = 0; i < state.machine.width; i++) {
        if (i > 0) {
            o << " ";
        }
        o << state.front_needles[i].count;
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

    for (const auto& n : state.back_needles) {
        h = hash_combine(h, n.count);
    }
    for (const auto& n : state.front_needles) {
        h = hash_combine(h, n.count);
    }

    h = hash_combine(h, state.braid.Hash());

    return h;
}
