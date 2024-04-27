#include "knitting.h"
#include "cbraid.h"
#include "prebuilt.h"
#include "util.h"

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
int NeedleLabel::offset(NeedleLabel destination) const {
    return destination.front ? i - destination.i : destination.i - i;
}

std::ostream& operator<<(std::ostream& o, const NeedleLabel& needle_label) {
    return o << (needle_label.front ? 'f' : 'b') << needle_label.i;
}

Needle::Needle(int count, NeedleLabel destination) :
    destination(destination),
    count(count)
{ }
Needle::Needle(const Needle& other) :
    destination(other.destination),
    count(other.count)
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
    slack_constraints(slack_constraints)
{
    for (int i = 0; i < machine.width; i++) {
        back_needles.emplace_back(back_loop_counts[i]);
        front_needles.emplace_back(front_loop_counts[i]);
    }
    set_target(target);
}

KnittingState::KnittingState(const KnittingState& other) :
    machine(other.machine),
    back_needles(other.back_needles),
    front_needles(other.front_needles),
    braid(other.braid),
    slack_constraints(other.slack_constraints),
    target(other.target)
{ }

void KnittingState::calculate_destinations() {
    auto permutation = braid.GetPerm().Inverse();

    NeedleLabel dest;
    int left = 0;
    int j = -1;
    for (int i = 0; i < 2*machine.width; i++) {
        NeedleLabel needle = machine[i];

        while (loop_count(needle) > left) {
            if (left != 0) {
                throw InvalidTargetStateException();
            }

            j++;
            dest = target->needle_with_braid_rank(permutation[j+1]-1);
            left = target->loop_count(dest);
        }
        destination(needle) = dest;
        left -= loop_count(needle);
    }
}

int KnittingState::racking() const {
    return machine.racking;
}

void KnittingState::set_target(KnittingState* t) {
    target = t;
    if (target != nullptr) {
        if (!target->braid.CompareWithIdentity()) {
            throw InvalidTargetStateException();
        }

        calculate_destinations();
    }
}

int& KnittingState::loop_count(const NeedleLabel& n) {
    return n.front ? front_needles[n.i].count : back_needles[n.i].count;
}
int KnittingState::loop_count(const NeedleLabel& n) const {
    return n.front ? front_needles[n.i].count : back_needles[n.i].count;
}
NeedleLabel& KnittingState::destination(const NeedleLabel& n) {
    return n.front ? front_needles[n.i].destination : back_needles[n.i].destination;
}
NeedleLabel KnittingState::destination(const NeedleLabel& n) const {
    return n.front ? front_needles[n.i].destination : back_needles[n.i].destination;
}
NeedleLabel KnittingState::needle_with_braid_rank(int rank) const {
    int j = -1;
    for (int i = 0; i < 2*machine.width; i++) {
        NeedleLabel needle = machine[i];
        if (loop_count(needle) > 0) {
            j++;
            if (j == rank) {
                return needle;
            }
        }
    }
    throw InvalidBraidRankException();
}

bool KnittingState::can_transfer(int loc) const {
    NeedleLabel back_needle = NeedleLabel(false, loc - machine.racking);
    NeedleLabel front_needle = NeedleLabel(true, loc);

    if (loop_count(front_needle) == 0 && loop_count(back_needle) == 0) {
        return false;
    }
    if (loop_count(front_needle) == 0 || loop_count(back_needle) == 0) {
        return true;
    }
    if (destination(front_needle) != destination(back_needle)) {
        return false;
    }

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
    j = braid.GetPerm().Inverse()[j+1]-1;

    return braid.CanMerge(j+1);
}

bool KnittingState::transfer(int loc, bool to_front) {
    NeedleLabel back_needle = NeedleLabel(false, loc - machine.racking);
    NeedleLabel front_needle = NeedleLabel(true, loc);

    if (loop_count(front_needle) > 0 && loop_count(back_needle) > 0) {
        if (destination(front_needle) != destination(back_needle)) {
            return false;
        }

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
        j = braid.GetPerm().Inverse()[j+1]-1;

        if (!braid.CanMerge(j+1)) {
            return false;
        }
        braid = braid.Merge(j+1);
    }

    if (to_front) {
        loop_count(front_needle) += loop_count(back_needle);
        destination(front_needle) = destination(back_needle);
        loop_count(back_needle) = 0;
        for (auto& constraint : slack_constraints) {
            constraint.replace(back_needle, front_needle);
        }
    }
    else {
        loop_count(back_needle) += loop_count(front_needle);
        destination(back_needle) = destination(front_needle);
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
    machine = other.machine;
    front_needles = other.front_needles;
    back_needles = other.back_needles;
    braid = other.braid;
    slack_constraints = other.slack_constraints;
    target = other.target;

    return *this;
}

KnittingState::TransitionIterator::TransitionIterator(
    const KnittingState& prev,
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
        int i = std::max(0, prev.machine.racking);
        i < prev.machine.width + std::min(0, prev.machine.racking);
        i++
    ) {
        if (prev.can_transfer(i)) {
            xfer_is.push_back(i);
            xfers.push_back(0);
            xfer_types.push_back(
                prev.loop_count(NeedleLabel(true, i)) > 0 &&
                prev.loop_count(NeedleLabel(false, i - prev.machine.racking)) > 0
            );
        }
    }

    done = xfers.empty();
    xfer_command = "xfer none";
}

void KnittingState::TransitionIterator::increment_xfers() {
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

bool KnittingState::TransitionIterator::try_next() {
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
        // if canonicalize makes us hit our target, then we need to
        // count it as an extra transfer pass
        weight = 2;
    }
    else {
        weight = 1;
    }
    racking++;

    return good;
}
bool KnittingState::TransitionIterator::has_next() {
    while (!done) {
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
std::vector<KnittingState> KnittingState::all_canonical_rackings() {
    auto v = all_rackings();
    for (auto& state : v) {
        state.canonicalize();
    }
    return v;
}

KnittingState::TransitionIterator KnittingState::adjacent() const {
    return KnittingState::TransitionIterator(*this, false);
}

KnittingState::TransitionIterator KnittingState::canonical_adjacent() const {
    return KnittingState::TransitionIterator(*this, true);
}

bool KnittingState::canonicalize() {
    // don't canonicalize if we are at the target, and return false to
    // signal that we didn't canonicalize
    if (target != nullptr && *this == *target) {
        return false;
    }

    for (
        int i = std::max(0, machine.racking); i < machine.width + std::min(0, machine.racking); i++
    ) {
        NeedleLabel back_needle = NeedleLabel(false, i - machine.racking);
        NeedleLabel front_needle = NeedleLabel(true, i);

        if (loop_count(back_needle) > 0 && loop_count(front_needle) == 0) {
            transfer(i, true);
        }
    }
    return true;
}

unsigned int KnittingState::no_heuristic() const {
    return 0;
}

unsigned int KnittingState::target_heuristic() const {
    if (target == nullptr) {
        return 0;
    }
    return *this == *target ? 0 : 1;
}

unsigned int KnittingState::braid_heuristic() const {
    if (braid.FactorList.size() > 0) {
        return braid.FactorList.size();
    }
    return target_heuristic();
}

unsigned long long KnittingState::offsets() const {
    unsigned long long offs = 0;

    for (int i = 0; i < 2*machine.width; i++) {
        NeedleLabel needle = machine[i];
        if (loop_count(needle) == 0) {
            continue;
        }

        int off = needle.offset(destination(needle));
        if (off != 0 && off < 32 && off >= -32) {
            offs |= 1ULL << (off+32);
        }
    }

    return offs;
}

unsigned int KnittingState::log_heuristic() const {
    auto x = log_offsets(offsets());

    return x == 0 ? target_heuristic() : x;
}

unsigned int KnittingState::prebuilt_heuristic() const {
    return prebuilt::query(offsets(), machine.racking);
}

unsigned int KnittingState::braid_log_heuristic() const {
    return std::max((unsigned int)braid.FactorList.size(), log_heuristic());
}

unsigned int KnittingState::braid_prebuilt_heuristic() const {
    return std::max((unsigned int)braid.FactorList.size(), prebuilt_heuristic());
}

std::ostream& operator<<(std::ostream& o, const knitting::KnittingState& state) {
    for (int i = 0; i < state.machine.racking; i++) {
        o << "  ";
    }
    o << "[";

    for (int i = 0; i < state.machine.width; i++) {
        if (i > 0) {
            o << " ";
        }
        o << state.back_needles[i].count;
    }
    o << "]\n";

    for (int i = 0; i > state.machine.racking; i--) {
        o << "  ";
    }
    o << "[";
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
