#ifndef TRAFFIC_DIJKSTRA_H
#define TRAFFIC_DIJKSTRA_H

#include "Graph.h"

#include <vector>

namespace traffic {

// Custom MinHeap gives O((V + E) log V) on this adjacency-list graph.
std::vector<int> findShortestPath(const Graph& graph, int start, int destination);

}  // namespace traffic

#endif
