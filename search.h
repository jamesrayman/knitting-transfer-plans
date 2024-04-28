#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <deque>
#include <iostream>
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
    const int path_length;
    const std::size_t search_tree_size;
    const double seconds_taken;

    SearchResult(
        const std::vector<typename State::Backpointer>& path,
        unsigned int path_length,
        std::size_t search_tree_size,
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
        return d.count(state) ? d[state] : 1'000'000'000;
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

template <typename State, typename TransitionIterator>
SearchResult<State> ida_star_search(
    const State& source, const State& target,
    TransitionIterator (State::*adj)() const, unsigned int (State::*h)() const,
    unsigned int bound
) {
    StopWatch stop_watch;
    std::size_t nodes_searched = 0;

    std::vector<TransitionIterator> path;
    path.reserve(bound+2);
    path.push_back((source.*adj)());
    std::vector<unsigned int> ds { 0 };
    std::vector<unsigned int> dhs { (source.*h)() };

    while (!path.empty()) {
        auto& it = path.back();

        if (it.has_next()) {
            nodes_searched++;

            unsigned int d = ds.back() + it.weight;
            unsigned int dh = d + (it.next.*h)();

            if (it.next == target && dh <= bound) {
                std::vector<typename State::Backpointer> back_path;

                for (auto& back_it : path) {
                    back_path.emplace_back(back_it.prev, back_it.command);
                }

                return SearchResult<State>(
                    back_path, d, nodes_searched, stop_watch.stop()
                );
            }

            if (dh <= bound) {
                auto next_it = (it.next.*adj)();
                path.push_back(next_it);
                ds.push_back(d);
                dhs.push_back(dh);
            }
        }
        else {
            path.pop_back();
            ds.pop_back();
            dhs.pop_back();
        }
    }

    return SearchResult<State>(
        std::vector<typename State::Backpointer>(), -1, nodes_searched, stop_watch.stop()
    );
}

template <typename State, typename TransitionIterator>
SearchResult<State> ida_star(
    const std::vector<State>& sources, const State& target,
    TransitionIterator (State::*adj)() const, unsigned int (State::*h)() const,
    unsigned int limit = 1e9
) {
    StopWatch stop_watch;
    std::size_t nodes_searched = 0;

    // edge case for when target is equal to one of the sources
    for (const State& source : sources) {
        nodes_searched++;
        if (source == target) {
            return SearchResult<State>(
                std::vector<typename State::Backpointer>(), 0, nodes_searched, stop_watch.stop()
            );
        }
    }

    for (unsigned int bound = 1; bound < limit; bound++) {
        for (const State& source : sources) {
            auto result = ida_star_search(source, target, adj, h, bound);
            nodes_searched += result.search_tree_size;
            if (result.path_length != -1) {
                return SearchResult<State>(
                    result.path, result.path_length, nodes_searched, stop_watch.stop()
                );
            }
        }
    }
    return SearchResult<State>(
        std::vector<typename State::Backpointer>(), -1, nodes_searched, stop_watch.stop()
    );
}

}

#endif
