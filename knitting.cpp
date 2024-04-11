#include "knitting.h"
#include "cbraid.h"
#include "braiding.h"

namespace knitting {

KnittingMachine::KnittingMachine(int width, int min_racking, int max_racking) :
    width(width),
    min_racking(min_racking),
    max_racking(max_racking)
{
    if (max_racking >= width || min_racking <= -width) {
        throw InvalidKnittingMachineException();
    }
}

KnittingMachine::KnittingMachine(const KnittingMachine& other) :
    width(other.width),
    min_racking(other.min_racking),
    max_racking(other.max_racking)
{ }

KnittingState::KnittingState(
    const KnittingMachine machine,
    const std::vector<int>& back_loop_counts,
    const std::vector<int>& front_loop_counts,
    int racking
) :
    machine(machine),
    racking(racking),
    braid(1)
{
    int current_loop = 0;
    for (int i = 0; i < machine.width; i++) {
        back_needles.emplace_back();
        for (int j = 0; j < back_loop_counts[i]; j++) {
            back_needles.back().push_back(current_loop);
            current_loop++;
        }
        front_needles.emplace_back();
        for (int j = 0; j < front_loop_counts[i]; j++) {
            front_needles.back().push_back(current_loop);
            current_loop++;
        }
    }
    loop_count = current_loop;
    braid = cb::ArtinBraid(loop_count);
}

KnittingState::KnittingState(const KnittingState& other) :
    machine(other.machine),
    racking(other.racking),
    back_needles(other.back_needles),
    front_needles(other.front_needles),
    braid(other.braid),
    loop_count(other.loop_count)
{ }

KnittingState::BedIterator::BedIterator(
    const KnittingState& state, int racking, int location, bool front
) :
    state(state),
    racking(racking),
    needle_location(location),
    needle_end(state.machine.width + std::max(0, racking)),
    front(front),
    loop_iterator(current_needle().begin()),
    loop_end(current_needle().end())
{
    roll_over();
}

KnittingState::BedIterator::BedIterator(const KnittingState& state, int racking) :
    BedIterator(state, racking, std::min(0, racking), 0 < racking)
{ }

void KnittingState::BedIterator::roll_over() {
    while (needle_location < needle_end && (!current_needle_exists() || loop_iterator == loop_end)) {
        if (front) {
            needle_location++;
        }
        front = !front;

        if (needle_location < needle_end && current_needle_exists()) {
            loop_iterator = current_needle().begin();
            loop_end = current_needle().end();
        }
    }
}

bool KnittingState::BedIterator::current_needle_exists() const {
    if (front) {
        return 0 <= needle_location && needle_location < state.machine.width;
    }
    else {
        return 0 <= needle_location-racking && needle_location-racking < state.machine.width;
    }
}

const std::vector<int>& KnittingState::BedIterator::current_needle() {
    return front ? state.front_needles[needle_location] : state.back_needles[needle_location-racking];
}

KnittingState::BedIterator::reference KnittingState::BedIterator::operator*() const {
    return *loop_iterator;
}
KnittingState::BedIterator::pointer KnittingState::BedIterator::operator->() const {
    return &*loop_iterator;
}

KnittingState::BedIterator& KnittingState::BedIterator::operator++() {
    loop_iterator++;
    roll_over();
    return *this;
}
KnittingState::BedIterator KnittingState::BedIterator::operator++(int) {
    BedIterator old = *this;
    ++(*this);
    return old;
}

bool operator==(const KnittingState::BedIterator& lhs, const KnittingState::BedIterator& rhs) {
    return lhs.racking == rhs.racking &&
           lhs.needle_location == rhs.needle_location &&
           lhs.front == rhs.front &&
           (
             lhs.current_needle_exists() == rhs.current_needle_exists() ||
             lhs.loop_iterator == rhs.loop_iterator
           );
}
bool operator!=(const KnittingState::BedIterator& lhs, const KnittingState::BedIterator& rhs) {
    return !(lhs == rhs);
}

KnittingState::BedIterator KnittingState::begin(int r) const {
    return BedIterator(*this, r);
}
KnittingState::BedIterator KnittingState::end(int r) const {
    return BedIterator(*this, r, machine.width + std::max(0, r), false);
}
KnittingState::BedIterator KnittingState::begin() const {
    return begin(racking);
}
KnittingState::BedIterator KnittingState::end() const {
    return end(racking);
}

int KnittingState::get_loop_count() const {
    return loop_count;
}

void KnittingState::transfer(int loc, bool to_back) {
    auto& back_needle = back_needles[loc+racking];
    auto& front_needle = front_needles[loc];

    if (to_back) {
        back_needle.insert(back_needle.end(), front_needle.begin(), front_needle.end());
    }
    else {
        front_needle.insert(front_needle.end(), back_needle.begin(), back_needle.end());
    }
}

void KnittingState::rack(int new_racking) {
    if (new_racking > machine.max_racking || new_racking < machine.min_racking) {
        throw InvalidRackingException();
    }
    if (new_racking == racking) {
        return;
    }

    cb::ArtinFactor f(loop_count, cb::ArtinFactor::Uninitialize, new_racking < racking);
    std::vector<int> loop_positions(loop_count);

    int i = 0;
    for (auto it = begin(racking); it != end(racking); it++, i++) {
        loop_positions[*it] = i;
    }

    i = 0;
    for (auto it = begin(new_racking); it != end(new_racking); it++, i++) {
        f[loop_positions[*it] + 1] = i + 1;
    }

    braid.LeftMultiply(f);
    racking = new_racking;
}

}
