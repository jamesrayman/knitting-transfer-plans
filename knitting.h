#include "cbraid.h"
#include <vector>

namespace knitting {

namespace cb = CBraid;

class InvalidKnittingMachineException { };
class InvalidRackingException { };

class KnittingMachine {
public:
    const int width;
    const int min_racking;
    const int max_racking;

    KnittingMachine(int, int, int);
    KnittingMachine(const KnittingMachine&);
};

class KnittingState {
public:
    using Bed = std::vector<std::vector<int>>;

    const KnittingMachine machine;

private:
    int racking;
    Bed back_needles;
    Bed front_needles;
    cb::ArtinBraid braid;
    int loop_count;

public:
    class BedIterator {
    public:
        using reference = const int&;
        using pointer = const int*;
        using LoopIterator = std::vector<int>::const_iterator;

    private:
        const KnittingState& state;
        const int racking;
        int needle_location;
        const int needle_end;
        bool front;
        LoopIterator loop_iterator;
        LoopIterator loop_end;

        const std::vector<int>& current_needle();
        bool current_needle_exists() const;
        void roll_over();
    public:
        BedIterator(const KnittingState&, int, int, bool);
        BedIterator(const KnittingState&, int);

        reference operator*() const;
        pointer operator->() const;

        BedIterator& operator++();
        BedIterator operator++(int);

        friend bool operator==(const BedIterator& lhs, const BedIterator& rhs);
        friend bool operator!=(const BedIterator& lhs, const BedIterator& rhs);
    };

    KnittingState(const KnittingMachine, const std::vector<int>&, const std::vector<int>&, int = 0);
    KnittingState(const KnittingState&);

    BedIterator begin(int) const;
    BedIterator end(int) const;
    BedIterator begin() const;
    BedIterator end() const;

    int get_loop_count() const;

    void transfer(int, bool);
    void rack(int);
};

}
