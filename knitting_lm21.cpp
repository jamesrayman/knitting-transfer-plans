#include <bit>
#include "knitting.h"
#include "prebuilt.h"
#include "util.h"


namespace knitting {

LoopSlackConstraint::LoopSlackConstraint(char loop_1, char loop_2, char limit) :
    loop_1(loop_1),
    loop_2(loop_2),
    limit(limit)
{ }

bool LoopSlackConstraint::respected(NeedleLabel needle_1, NeedleLabel needle_2, char racking) const {
    return abs(needle_1.location(racking) - needle_2.location(racking)) <= limit;
}


KnittingStateLM21::KnittingStateLM21() :
    braid(1)
{ }
KnittingStateLM21::KnittingStateLM21(
    const KnittingMachine machine,
    const std::vector<char>& back_loop_counts,
    const std::vector<char>& front_loop_counts,
    const cb::ArtinBraid& braid,
    const std::vector<SlackConstraint>& slack_constraints,
    KnittingStateLM21* target,
    bool only_contractions
) :
    machine(machine),
    braid(braid),
    loop_locations(braid.Index()),
    only_contractions(only_contractions)
{
    auto permutation = braid.GetPerm().Inverse();

    for (char i = 0, j = 0; i < 2*machine.width; i++) {
        NeedleLabel needle = machine[i];
        const std::vector<char>& loop_counts = needle.front ? front_loop_counts : back_loop_counts;
        int loop_count = loop_counts[needle.i];

        for (int k = 0; k < loop_count; k++, j++) {
            loop_locations[permutation[j+1]-1] = needle;
        }
    }

    for (SlackConstraint constraint : slack_constraints) {
        this->slack_constraints.emplace_back(
            std::find(loop_locations.begin(), loop_locations.end(), constraint.needle_1)
                - loop_locations.begin(),
            std::find(loop_locations.begin(), loop_locations.end(), constraint.needle_2)
                - loop_locations.begin(),
            constraint.limit
        );
    }

    set_target(target);
}
KnittingStateLM21::KnittingStateLM21(const KnittingStateLM21& other) :
    machine(other.machine),
    braid(other.braid),
    loop_locations(other.loop_locations),
    slack_constraints(other.slack_constraints),
    target(other.target),
    only_contractions(other.only_contractions)
{ }

char KnittingStateLM21::racking() const {
    return machine.racking;
}

bool KnittingStateLM21::needle_empty(NeedleLabel needle) const {
    for (const auto& n : loop_locations) {
        if (n == needle) {
            return false;
        }
    }
    return true;
}

char KnittingStateLM21::loop_count(NeedleLabel needle) const {
    char count = 0;
    for (const auto& n : loop_locations) {
        if (n == needle) {
            count++;
        }
    }
    return count;
}

void KnittingStateLM21::set_target(KnittingStateLM21* t) {
    target = t;
    if (target != nullptr) {
        if (!target->braid.CompareWithIdentity()) {
            throw InvalidTargetStateException();
        }
    }
}
bool KnittingStateLM21::can_transfer(char loc) const {
    NeedleLabel back_needle = NeedleLabel(false, loc - machine.racking);
    NeedleLabel front_needle = NeedleLabel(true, loc);

    if (needle_empty(front_needle) && needle_empty(back_needle)) {
        return false;
    }
    if (only_contractions && !needle_empty(front_needle) && !needle_empty(back_needle)) {
        for (auto& constraint : slack_constraints) {
            if (
                (loop_locations[constraint.loop_1] == front_needle &&
                 loop_locations[constraint.loop_2] == back_needle) ||
                (loop_locations[constraint.loop_1] == back_needle &&
                 loop_locations[constraint.loop_2] == front_needle)
            ) {
                return true;
            }
        }

        return false;
    }
    return true;
}

std::vector<char> KnittingStateLM21::back_bed() const {
    std::vector<char> bed (machine.width);

    for (auto needle : loop_locations) {
        if (!needle.front) {
            bed[needle.i]++;
        }
    }

    return bed;
}
std::vector<char> KnittingStateLM21::front_bed() const {
    std::vector<char> bed (machine.width);

    for (auto needle : loop_locations) {
        if (needle.front) {
            bed[needle.i]++;
        }
    }

    return bed;

}

bool KnittingStateLM21::rack(char new_racking) {
    if (new_racking > machine.max_racking || new_racking < machine.min_racking) {
        return false;
    }
    if (new_racking == machine.racking) {
        return true;
    }

    for (const LoopSlackConstraint& constraint : slack_constraints) {
        if (!constraint.respected(
            loop_locations[constraint.loop_1], loop_locations[constraint.loop_2], new_racking
        )) {
            return false;
        }
    }

    cb::ArtinFactor f(braid.Index(), cb::ArtinFactor::Uninitialize, new_racking < machine.racking);
    std::vector<int> needle_positions (2*machine.width);

    for (char i = 0, j = 0; i < 2*machine.width; i++) {
        NeedleLabel needle = machine[i];
        needle_positions[needle.id()] = j;
        j += loop_count(needle);
    }
    machine.racking = new_racking;

    for (char i = 0, j = 0; i < 2*machine.width; i++) {
        NeedleLabel needle = machine[i];
        char count = loop_count(needle);

        for (int k = 0; k < count; k++, j++) {
            f[j + 1] = needle_positions[needle.id()] + k + 1;
        }
    }

    braid.LeftMultiply(f);
    braid.MakeMCF();

    return true;
}
bool KnittingStateLM21::transfer(char loc, bool to_front) {
    NeedleLabel back_needle = NeedleLabel(false, loc - machine.racking);
    NeedleLabel front_needle = NeedleLabel(true, loc);

    NeedleLabel to_needle = to_front ? front_needle : back_needle;
    NeedleLabel from_needle = to_front ? back_needle : front_needle;

    for (auto& needle : loop_locations) {
        if (needle == from_needle) {
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
    machine = other.machine;
    loop_locations = other.loop_locations;
    braid = other.braid;
    slack_constraints = other.slack_constraints;
    target = other.target;
    only_contractions = other.only_contractions;

    return *this;
}

std::vector<KnittingStateLM21> KnittingStateLM21::all_rackings() {
    std::vector<KnittingStateLM21> v;
    char old_racking = machine.racking;

    for (char r = machine.min_racking; r <= machine.max_racking; r++) {
        if (rack(r)) {
            v.push_back(*this);
        }
    }
    rack(old_racking);

    return v;
}
std::vector<KnittingStateLM21> KnittingStateLM21::all_canonical_rackings() {
    return all_rackings();
}

KnittingStateLM21::TransitionIterator KnittingStateLM21::adjacent() const {
    return KnittingStateLM21::TransitionIterator(*this, false);
}
KnittingStateLM21::TransitionIterator KnittingStateLM21::canonical_adjacent() const {
    return KnittingStateLM21::TransitionIterator(*this, true);
}

bool KnittingStateLM21::canonicalize() {
    if (target != nullptr && *this == *target) {
        return false;
    }

    for (
        char i = std::max('\0', machine.racking);
        i < machine.width + std::min('\0', machine.racking);
        i++
    ) {
        NeedleLabel front_needle = NeedleLabel(true, i);

        if (needle_empty(front_needle)) {
            transfer(i, true);
        }
    }
    return true;
}

unsigned long long KnittingStateLM21::offsets() const {
    unsigned long long offs = 0;

    for (unsigned char i = 0; i < loop_locations.size(); i++) {
        int off = loop_locations[i].offset(target->loop_locations[i]);
        if (off != 0 && off < 32 && off >= -32) {
            offs |= 1ULL << (off+32);
        }
    }

    return offs;
}

unsigned int KnittingStateLM21::no_heuristic() const {
    return 0;
}
unsigned int KnittingStateLM21::target_heuristic() const {
    if (target == nullptr) {
        return 0;
    }
    return *this == *target ? 0 : 1;
}
unsigned int KnittingStateLM21::braid_heuristic() const {
    if (braid.FactorList.size() > 0) {
        return (unsigned int)braid.FactorList.size();
    }
    return target_heuristic();
}
unsigned int KnittingStateLM21::log_heuristic() const {
    auto x = log_offsets(offsets());

    return x == 0 ? target_heuristic() : x;
}
unsigned int KnittingStateLM21::prebuilt_heuristic() const {
    return prebuilt::query(offsets(), machine.racking);
}
unsigned int KnittingStateLM21::braid_log_heuristic() const {
    return std::max((unsigned int)braid.FactorList.size(), log_heuristic());
}
unsigned int KnittingStateLM21::braid_prebuilt_heuristic() const {
    return std::max((unsigned int)braid.FactorList.size(), prebuilt_heuristic());
}


KnittingStateLM21::TransitionIterator::TransitionIterator(
    const KnittingStateLM21& prev,
    bool canonicalize
) :
    canonicalize(canonicalize),
    next_uncanonical(prev),
    prev(prev),
    next(prev)
{
    racking = prev.machine.min_racking;
    good = false;

    for (
        char i = std::max('\0', prev.machine.racking);
        i < prev.machine.width + std::min('\0', prev.machine.racking);
        i++
    ) {
        if (prev.can_transfer(i)) {
            xfer_is.push_back(i);
            xfers.push_back(0);
            xfer_types.push_back(
                !prev.needle_empty(NeedleLabel(true, i)) &&
                !prev.needle_empty(NeedleLabel(false, i - prev.machine.racking))
            );
        }
    }

    done = xfers.empty();
    xfer_command = "xfer none";
}

void KnittingStateLM21::TransitionIterator::increment_xfers() {
    next_uncanonical = prev;

    for (unsigned int i = 0; i < xfers.size(); i++) {

        if (xfers[i] == (xfer_types[i] ? 2 : 1)) {
            xfers[i] = 0;
            if (i + 1 == xfer_is.size()) {
                done = true;
                return;
            }
        }
        else {
            xfers[i]++;
            break;
        }
    }
    xfer_command = "xfer";

    for (unsigned int i = 0; i < xfers.size(); i++) {
        if (xfers[i] == 0) {
            continue;
        }
        bool to_front = xfers[i] == 2;

        if (!to_front && prev.loop_count(NeedleLabel(true, xfer_is[i])) == 0) {
            // front needle has no loops, so transfer to front instead
            to_front = true;
        }

        next_uncanonical.transfer(xfer_is[i], to_front);
        xfer_command += (to_front ? " f" : " b") + std::to_string(xfer_is[i]);
    }
}

bool KnittingStateLM21::TransitionIterator::try_next() {
    if (racking > prev.machine.max_racking) {
        increment_xfers();
        racking = prev.machine.min_racking;
        if (done) {
            return false;
        }
    }

    command = xfer_command + "; rack " + std::to_string(racking);

    next = next_uncanonical;
    good = next.rack(racking);
    if (canonicalize && next.canonicalize() && next == *prev.target) {
        weight = 2;
    }
    else {
        weight = 1;
    }
    racking++;

    return good;
}
bool KnittingStateLM21::TransitionIterator::has_next() {
    while (!done) {
        if (try_next()) {
            return true;
        }
    }
    return false;
}

KnittingStateLM21 KnittingStateLM21::TransitionIterator::random(std::mt19937& rng) {
    KnittingStateLM21 state;
    int valid_so_far = 0;

    while (has_next()) {
        std::uniform_int_distribution<int> dist(0, valid_so_far);
        if (dist(rng) == 0) {
            state = next;
        }
        valid_so_far++;
    }

    return state;
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

std::ostream& operator<<(std::ostream& o, const knitting::KnittingStateLM21& state) {
    o << (int)state.machine.racking << " [";
    for (unsigned int i = 0; i < state.loop_locations.size(); i++) {
        if (i > 0) {
            o << " ";
        }
        o << state.loop_locations[i];
    }
    o << "] ";
    o << state.braid;

    return o;

}

}

std::size_t std::hash<knitting::KnittingStateLM21>::operator()(
    const knitting::KnittingStateLM21& state
) const {
    size_t h = 0xf0e35c6e3c319f8;
    h = hash_combine(h, state.machine.racking);

    for (const auto& needle : state.loop_locations) {
        h = hash_combine(h, needle.front);
        h = hash_combine(h, needle.i);
    }

    h = hash_combine(h, state.braid.Hash());

    return h;
}
