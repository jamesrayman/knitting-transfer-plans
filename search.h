#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <deque>
#include <algorithm>

#ifndef SEARCH_H
#define SEARCH_H

namespace search {

template <typename State>
class PriorityQueue {
private:
    std::deque<std::unordered_set<State>> queue;
    int front = 0;

public:
    void insert(int key, const State& state) {
        while (key-front >= queue.size()) {
            queue.emplace_back();
        }
        queue[key-front].insert(state);
    }

    State pop() {
        State ret = *queue[0].begin();
        queue[0].erase(queue[0].begin());

        return ret;
    }

    void erase(int key, const State& state) {
        queue[key-front].erase(state);
    }

    bool empty() {
        while (!queue.empty() && queue[0].empty()) {
            queue.pop_front();
            front++;
        }

        return queue.empty();
    }
};

template <typename State>
std::vector<typename State::Backpointer> backpointer_path(
    const std::unordered_map<State, typename State::Backpointer>& from, State target
) {
    std::vector<typename State::Backpointer> v;

    while (from.count(target)) {
        auto t = from.at(target);
        v.push_back(t);
        target = t.prev;
    }

    std::reverse(v.begin(), v.end());
    return v;
}

template <typename State>
class SearchResult {
public:
    const std::vector<typename State::Backpointer> path;
    const int path_length;
    const int search_tree_size;

    SearchResult(
        const std::vector<typename State::Backpointer>& path, int path_length, int search_tree_size
    ) :
        path(path),
        path_length(path_length),
        search_tree_size(search_tree_size)
    { }
};

template <typename State>
SearchResult<State> a_star(
    const State& source, const State& target,
    typename State::TransitionIterator (State::*adj)() const, int (State::*h)() const
) {
    PriorityQueue<State> q;
    std::unordered_map<State, int> d;
    std::unordered_map<State, int> dh;
    std::unordered_map<State, typename State::Backpointer> from;

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
            return SearchResult<State>(backpointer_path(from, target), state_d, d.size());
        }

        auto it = (state.*adj)();
        while (it.has_next()) {
            const int cand_d = state_d + it.weight;

            if (cand_d < dist_at(it.next)) {
                from[it.next] = typename State::Backpointer(state, it.command);
                d[it.next] = cand_d;

                q.erase(dh[it.next], it.next);
                dh[it.next] = cand_d + (it.next.*h)();
                q.insert(dh[it.next], it.next);
            }
        }
    }

    return SearchResult<State>(std::vector<typename State::Backpointer>(), -1, d.size());
}

}

#endif
