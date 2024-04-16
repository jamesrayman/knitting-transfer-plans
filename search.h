#include <unordered_map>
#include <unordered_set>
#include <map>
#include <vector>
#include <algorithm>

namespace search {

template <typename State>
class SearchQueue {
private:
    std::map<int, std::unordered_set<State>> queue;

public:
    void insert(int key, const State& state) {
        queue[key].insert(state);
    }

    State pop() {
        State ret = *queue.begin()->second.begin();

        queue.begin()->second.erase(queue.begin()->second.begin());
        if (queue.begin()->second.empty()) {
            queue.erase(queue.begin());
        }

        return ret;
    }

    void erase(int key, const State& state) {
        queue[key].erase(state);
        if (queue[key].empty()) {
            queue.erase(key);
        }
    }

    bool empty() const {
        return queue.empty();
    }
};

template <typename State>
std::vector<typename State::Transition> backpointer_path(
    const std::unordered_map<State, typename State::Transition>& from, State target
) {
    std::vector<typename State::Transition> v;

    while (from.count(target)) {
        auto t = from.at(target);
        v.push_back(t);
        target = t.prev;
    }

    std::reverse(v.begin(), v.end());
    return v;
}

template <typename State>
std::vector<typename State::Transition> a_star(
    const State& source, const State& target,
    std::vector<typename State::Transition> (State::*adj)() const, int (State::*h)() const
) {
    SearchQueue<State> q;
    std::unordered_map<State, int> d;
    std::unordered_map<State, int> dh;
    std::unordered_map<State, typename State::Transition> from;

    q.insert((source.*h)(), source);
    d[source] = 0;
    dh[source] = (source.*h)();

    auto dist_at = [&d](const State& state) {
        return d.count(state) ? d[state] : 1e9;
    };

    while (!q.empty()) {
        State state = q.pop();
        int state_d = dist_at(state);

        if (state == target) {
            std::cout << d.size() << std::endl;
            return backpointer_path(from, target);
        }

        for (const typename State::Transition& t : (state.*adj)()) {
            const int cand_d = state_d + t.weight;

            if (cand_d < dist_at(t.next)) {
                from[t.next] = t;
                d[t.next] = cand_d;

                q.erase(dh[t.next], t.next);
                dh[t.next] = cand_d + (t.next.*h)();
                q.insert(dh[t.next], t.next);
            }
        }
    }

    return std::vector<typename State::Transition>();
}

}
