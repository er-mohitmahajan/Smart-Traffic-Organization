#ifndef TRAFFIC_GRAPH_H
#define TRAFFIC_GRAPH_H

#include <string>
#include <vector>

namespace traffic {

struct Point {
    float x = 0.0f;
    float y = 0.0f;
};

struct Edge {
    int to = -1;
    float baseTravelTime = 1.0f;
    float congestion = 0.0f;

    float weight() const { return baseTravelTime + congestion; }
};

struct Node {
    std::string name;
    Point position;
    std::vector<Edge> edges;
};

// Dynamic-array adjacency list: addNode O(1) amortized, addEdge O(degree),
// neighbor traversal O(degree), and a full traversal O(V + E).
class Graph {
private:
    std::vector<Node> nodes_;

public:
    int addNode(const std::string& name, Point position);
    bool addEdge(int from, int to, float baseTravelTime);
    bool updateCongestion(int from, int to, float congestion);
    const Edge* findEdge(int from, int to) const;
    const std::vector<Node>& nodes() const { return nodes_; }
    std::size_t size() const { return nodes_.size(); }
};

}  // namespace traffic

#endif
