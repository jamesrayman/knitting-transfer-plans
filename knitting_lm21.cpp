#include <bit>
#include "knitting.h"
#include "prebuilt.h"
#include "util.h"


namespace knitting {

LoopSlackConstraint::LoopSlackConstraint(int loop_1, int loop_2, int limit) :
    loop_1(loop_1),
    loop_2(loop_2),
    limit(limit)
{ }

bool LoopSlackConstraint::respected(NeedleLabel needle_1, NeedleLabel needle_2, int racking) const {
    return abs(needle_1.location(racking) - needle_2.location(racking)) <= limit;
}


KnittingStateLM21::KnittingStateLM21() :
    braid(1)
{ }
KnittingStateLM21::KnittingStateLM21(
    const KnittingMachine machine,
    const std::vector<int>& back_loop_counts,
    const std::vector<int>& front_loop_counts,
    const cb::ArtinBraid& braid,
    const std::vector<LoopSlackConstraint>& slack_constraints,
    KnittingStateLM21* target
) :
    machine(machine),
    braid(braid),
    slack_constraints(&slack_constraints)
{
    throw NotImplemented();
    set_target(target);
}
KnittingStateLM21::KnittingStateLM21(const KnittingStateLM21& other) :
    machine(other.machine),
    braid(other.braid),
    loop_locations(other.loop_locations),
    slack_constraints(other.slack_constraints),
    target(other.target)
{ }

int KnittingStateLM21::racking() const {
    return machine.racking;
}

bool KnittingStateLM21::needle_empty(NeedleLabel needle) const {
    throw NotImplemented();
}

void KnittingStateLM21::set_target(KnittingStateLM21* t) {
    target = t;
    if (target != nullptr) {
        if (!target->braid.CompareWithIdentity()) {
            throw InvalidTargetStateException();
        }
    }
}
bool KnittingStateLM21::can_transfer(int loc) const {
    NeedleLabel back_needle = NeedleLabel(false, loc - machine.racking);
    NeedleLabel front_needle = NeedleLabel(true, loc);

    return !needle_empty(front_needle) || !needle_empty(back_needle);
}

bool KnittingStateLM21::rack(int new_racking) {
    if (new_racking > machine.max_racking || new_racking < machine.min_racking) {
        return false;
    }
    if (new_racking == machine.racking) {
        return true;
    }

    for (const LoopSlackConstraint& constraint : *slack_constraints) {
        if (!constraint.respected(
            loop_locations[constraint.loop_1], loop_locations[constraint.loop_2], new_racking
        )) {
            return false;
        }
    }

    throw NotImplemented();
}
bool KnittingStateLM21::transfer(int loc, bool to_front) {
    NeedleLabel back_needle = NeedleLabel(false, loc - machine.racking);
    NeedleLabel front_needle = NeedleLabel(true, loc);

    NeedleLabel to_needle = to_front ? front_needle : back_needle;
    NeedleLabel from_needle = to_front ? back_needle : front_needle;

    for (auto& needle : loop_locations) {
        if (needle == front_needle) {
            needle = to_needle;
        }
    }

    return true;
}

bool KnittingStateLM21::operator==(const KnittingStateLM21& other) const {
    return machine.racking == other.machine.racking &&
           loop_locations == other.loop_locations &&
           braid == other.braid;
}
bool KnittingStateLM21::operator!=(const KnittingStateLM21& other) const {
    return !(*this == other);
}

KnittingStateLM21& KnittingStateLM21::operator=(const KnittingStateLM21& other) {
    machine.racking = other.machine.racking;
    loop_locations = other.loop_locations;
    braid = other.braid;
    slack_constraints = other.slack_constraints;
    target = other.target;

    return *this;
}

std::vector<KnittingStateLM21> KnittingStateLM21::all_rackings() {
    std::vector<KnittingStateLM21> v;
    int old_racking = machine.racking;

    for (int r = machine.min_racking; r <= machine.max_racking; r++) {
        if (rack(r)) {
            v.push_back(*this);
        }
    }
    rack(old_racking);

    return v;
}
std::vector<KnittingStateLM21> KnittingStateLM21::all_canonical_rackings() {
    auto v = all_rackings();
    for (auto& state : v) {
        state.canonicalize();
    }
    return v;
}

KnittingStateLM21::TransitionIterator KnittingStateLM21::adjacent() const {
    return KnittingStateLM21::TransitionIterator(*this);
}
KnittingStateLM21::CanonicalTransitionIterator KnittingStateLM21::canonical_adjacent() const {
    return KnittingStateLM21::CanonicalTransitionIterator(*this);
}

bool KnittingStateLM21::canonicalize() {
    if (target != nullptr && *this == *target) {
        return false;
    }

    for (
        int i = std::max(0, machine.racking); i < machine.width + std::min(0, machine.racking); i++
    ) {
        NeedleLabel front_needle = NeedleLabel(true, i);

        if (needle_empty(front_needle)) {
            transfer(i, true);
        }
    }
    return true;
}

unsigned long long KnittingStateLM21::offsets() const {
    throw NotImplemented();
}

int KnittingStateLM21::no_heuristic() const {
    return 0;
}
int KnittingStateLM21::target_heuristic() const {
    if (target == nullptr) {
        return 0;
    }
    return *this == *target ? 0 : 1;
}
int KnittingStateLM21::braid_heuristic() const {
    if (braid.FactorList.size() > 0) {
        return braid.FactorList.size();
    }
    return target_heuristic();
}
int KnittingStateLM21::log_heuristic() const {
    int n = std::popcount(offsets());

    if (n == 0) {
        return target_heuristic();
    }

    // calculate x = ceil(log_2(n+1))
    int x = 1;
    while (n > 1) {
        x++;
        n >>= 1;
    }
    return x;
}
int KnittingStateLM21::prebuilt_heuristic() const {
    return prebuilt::query(offsets());
}
int KnittingStateLM21::braid_log_heuristic() const {
    return std::max((int)braid.FactorList.size(), log_heuristic());
}
int KnittingStateLM21::braid_prebuilt_heuristic() const {
    return std::max((int)braid.FactorList.size(), prebuilt_heuristic());
}


KnittingStateLM21::TransitionIterator::TransitionIterator(const KnittingStateLM21& prev) :
    prev(prev),
    next(prev)
{
    throw NotImplemented();
}

bool KnittingStateLM21::TransitionIterator::try_next() {
    throw NotImplemented();
}

bool KnittingStateLM21::TransitionIterator::has_next() {
    throw NotImplemented();
}

KnittingStateLM21::CanonicalTransitionIterator::CanonicalTransitionIterator(
    const KnittingStateLM21& prev
) :
    next_uncanonical(prev),
    prev(prev),
    next(prev)
{
    throw NotImplemented();
}

void KnittingStateLM21::CanonicalTransitionIterator::increment_xfers() {
    throw NotImplemented();
}
bool KnittingStateLM21::CanonicalTransitionIterator::try_next() {
    throw NotImplemented();
}
bool KnittingStateLM21::CanonicalTransitionIterator::has_next() {
    throw NotImplemented();
}


KnittingStateLM21::Backpointer::Backpointer() { }
KnittingStateLM21::Backpointer::Backpointer(
    const KnittingStateLM21& prev, const std::string& command
) :
    prev(prev),
    command(command)
{ }

KnittingStateLM21::Backpointer::Backpointer(
    const KnittingStateLM21::Backpointer& other
) :
    prev(other.prev),
    command(other.command)
{ }

KnittingStateLM21::Backpointer& KnittingStateLM21::Backpointer::operator=(
    const KnittingStateLM21::Backpointer& other
) {
    prev = other.prev;
    command = other.command;
    return *this;
}

}

std::ostream& operator<<(std::ostream& o, const knitting::KnittingStateLM21& state) {
    throw NotImplemented();
}

std::size_t std::hash<knitting::KnittingStateLM21>::operator()(
    const knitting::KnittingStateLM21& state
) const {
    throw NotImplemented();
}
