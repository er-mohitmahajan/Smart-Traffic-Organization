#include "Dijkstra.h"
#include "Graph.h"
#include "MaxHeap.h"
#include "MinHeap.h"
#include "Queue.h"

#include <cassert>
#include <iostream>
#include <vector>

int main() {
    traffic::Queue<int> queue;
    int value = 0;
    assert(queue.empty());
    assert(!queue.dequeue(value));
    queue.enqueue(2);
    queue.enqueueFront(1);
    queue.enqueue(3);
    assert(queue.size() == 3);
    assert(queue.dequeue(value) && value == 1);
    assert(queue.dequeue(value) && value == 2);
    assert(queue.dequeue(value) && value == 3);
    assert(queue.empty());

    traffic::MaxHeap<int> maxHeap;
    maxHeap.build({3, 9, 1, 6, 2});
    assert(maxHeap.top() != nullptr && *maxHeap.top() == 9);
    assert(maxHeap.pop(value) && value == 9);
    maxHeap.push(12);
    assert(maxHeap.pop(value) && value == 12);

    traffic::MinHeap<int> minHeap;
    minHeap.build({3, 9, 1, 6, 2});
    assert(minHeap.top() != nullptr && *minHeap.top() == 1);
    assert(minHeap.pop(value) && value == 1);
    minHeap.push(0);
    assert(minHeap.pop(value) && value == 0);

    traffic::Graph graph;
    const int a = graph.addNode("A", {0.0f, 0.0f});
    const int b = graph.addNode("B", {1.0f, 0.0f});
    const int c = graph.addNode("C", {2.0f, 0.0f});
    const int d = graph.addNode("D", {1.0f, 1.0f});
    assert(graph.addEdge(a, b, 1.0f));
    assert(graph.addEdge(b, c, 1.0f));
    assert(graph.addEdge(a, d, 1.5f));
    assert(graph.addEdge(d, c, 1.5f));
    assert(!graph.addEdge(a, b, 1.0f));
    assert(graph.findEdge(a, b) != nullptr);

    std::vector<int> path = traffic::findShortestPath(graph, a, c);
    assert((path == std::vector<int>{a, b, c}));
    assert(graph.updateCongestion(a, b, 3.0f));
    path = traffic::findShortestPath(graph, a, c);
    assert((path == std::vector<int>{a, d, c}));
    assert(traffic::findShortestPath(graph, c, c) == std::vector<int>{c});
    assert(traffic::findShortestPath(graph, -1, c).empty());

    std::cout << "All custom queue, heap, graph, and Dijkstra tests passed.\n";
    return 0;
}
