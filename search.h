#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <deque>
#include <algorithm>
#include "util.h"

#ifndef SEARCH_H
#define SEARCH_H

namespace search {

template <typename State>
class PriorityQueue {
private:
    std::deque<std::unordered_set<State>> queue;
public:
    unsigned int front = 0;

    void insert(unsigned int key, const State& state) {
        while (key < front) {
            queue.emplace_front();
            front--;
        }
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

    void erase(unsigned int key, const State& state) {
        if (0 <= key-front && key-front < queue.size()) {
            queue[key-front].erase(state);
        }
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
    const unsigned int path_length;
    const unsigned int search_tree_size;
    double seconds_taken;

    SearchResult(
        const std::vector<typename State::Backpointer>& path,
        unsigned int path_length,
        unsigned int search_tree_size,
        double seconds_taken
    ) :
        path(path),
        path_length(path_length),
        search_tree_size(search_tree_size),
        seconds_taken(seconds_taken)
    { }
};

template <typename State, typename TransitionIterator>
SearchResult<State> a_star(
    const std::vector<State>& sources, const State& target,
    TransitionIterator (State::*adj)() const, unsigned int (State::*h)() const,
    unsigned int limit = 1e9
) {
    StopWatch stop_watch;
    PriorityQueue<State> q;
    std::unordered_map<State, unsigned int> d;
    std::unordered_map<State, unsigned int> dh;
    std::unordered_map<State, typename State::Backpointer> from;

    for (const State& source : sources) {
        q.insert((source.*h)(), source);
        d[source] = 0;
        dh[source] = (source.*h)();
    }

    auto dist_at = [&d](const State& state) {
        return d.count(state) ? d[state] : 1e9;
    };

    while (!q.empty() && q.front <= limit) {
        State state = q.pop();
        unsigned int state_d = dist_at(state);

        if (state == target) {
            return SearchResult<State>(
                backpointer_path(from, target), state_d, d.size(), stop_watch.stop()
            );
        }

        TransitionIterator it = (state.*adj)();
        while (it.has_next()) {
            const unsigned int cand_d = state_d + it.weight;

            if (cand_d < dist_at(it.next)) {
                from[it.next] = typename State::Backpointer(state, it.command);
                d[it.next] = cand_d;

                if (dh.count(it.next)) {
                    q.erase(dh[it.next], it.next);
                }
                dh[it.next] = cand_d + (it.next.*h)();
                q.insert(dh[it.next], it.next);
            }
        }
    }

    return SearchResult<State>(
        std::vector<typename State::Backpointer>(), -1, d.size(), stop_watch.stop()
    );
}

}

#endif
