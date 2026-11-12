#include "Dijkstra.h"

#include "MinHeap.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace traffic {
namespace {

struct HeapEntry {
    float distance;
    int node;

    bool operator<(const HeapEntry& other) const {
        if (distance != other.distance) {
            return distance < other.distance;
        }
        return node < other.node;
    }
};

}  // namespace

std::vector<int> findShortestPath(const Graph& graph, int start, int destination) {
    const std::size_t count = graph.size();
    if (start < 0 || destination < 0 ||
        static_cast<std::size_t>(start) >= count ||
        static_cast<std::size_t>(destination) >= count) {
        return {};
    }
    if (start == destination) {
        return {start};
    }

    const float infinity = std::numeric_limits<float>::infinity();
    std::vector<float> distances(count, infinity);
    std::vector<int> previous(count, -1);
    MinHeap<HeapEntry> frontier;
    distances[static_cast<std::size_t>(start)] = 0.0f;
    frontier.push({0.0f, start});

    while (!frontier.empty()) {
        HeapEntry current{};
        frontier.pop(current);
        if (current.distance > distances[static_cast<std::size_t>(current.node)]) {
            continue;
        }
        if (current.node == destination) {
            break;
        }

        for (const Edge& edge :
             graph.nodes()[static_cast<std::size_t>(current.node)].edges) {
            const float nextDistance = current.distance + edge.weight();
            if (nextDistance < distances[static_cast<std::size_t>(edge.to)]) {
                distances[static_cast<std::size_t>(edge.to)] = nextDistance;
                previous[static_cast<std::size_t>(edge.to)] = current.node;
                frontier.push({nextDistance, edge.to});
            }
        }
    }

    if (!std::isfinite(distances[static_cast<std::size_t>(destination)])) {
        return {};
    }

    std::vector<int> path;
    for (int node = destination; node != -1;
         node = previous[static_cast<std::size_t>(node)]) {
        path.push_back(node);
        if (node == start) {
            break;
        }
    }
    if (path.back() != start) {
        return {};
    }
    std::reverse(path.begin(), path.end());
    return path;
}

}  // namespace traffic
