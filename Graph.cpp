#include "Graph.h"

#include <algorithm>
#include <cmath>

namespace traffic {

int Graph::addNode(const std::string& name, Point position) {
    nodes_.push_back({name, position, {}});
    return static_cast<int>(nodes_.size() - 1);
}

bool Graph::addEdge(int from, int to, float baseTravelTime) {
    if (from < 0 || to < 0 || from == to ||
        static_cast<std::size_t>(from) >= nodes_.size() ||
        static_cast<std::size_t>(to) >= nodes_.size() ||
        !std::isfinite(baseTravelTime) || baseTravelTime <= 0.0f) {
        return false;
    }

    const auto addDirectedEdge = [&](int source, int destination) {
        auto& edges = nodes_[static_cast<std::size_t>(source)].edges;
        const auto existing = std::find_if(
            edges.begin(), edges.end(),
            [destination](const Edge& edge) { return edge.to == destination; });
        if (existing != edges.end()) {
            return false;
        }
        edges.push_back({destination, baseTravelTime, 0.0f});
        return true;
    };

    const bool forward = addDirectedEdge(from, to);
    const bool reverse = addDirectedEdge(to, from);
    return forward && reverse;
}

bool Graph::updateCongestion(int from, int to, float congestion) {
    if (!std::isfinite(congestion) || congestion < 0.0f) {
        return false;
    }
    auto updateDirectedEdge = [&](int source, int destination) {
        if (source < 0 || static_cast<std::size_t>(source) >= nodes_.size()) {
            return false;
        }
        auto& edges = nodes_[static_cast<std::size_t>(source)].edges;
        const auto found = std::find_if(
            edges.begin(), edges.end(),
            [destination](const Edge& edge) { return edge.to == destination; });
        if (found == edges.end()) {
            return false;
        }
        found->congestion = congestion;
        return true;
    };

    const bool forward = updateDirectedEdge(from, to);
    const bool reverse = updateDirectedEdge(to, from);
    return forward && reverse;
}

const Edge* Graph::findEdge(int from, int to) const {
    if (from < 0 || static_cast<std::size_t>(from) >= nodes_.size()) {
        return nullptr;
    }
    const auto& edges = nodes_[static_cast<std::size_t>(from)].edges;
    const auto found = std::find_if(
        edges.begin(), edges.end(),
        [to](const Edge& edge) { return edge.to == to; });
    return found == edges.end() ? nullptr : &*found;
}

}  // namespace traffic
