#ifndef TRAFFIC_SIGNAL_H
#define TRAFFIC_SIGNAL_H

#include "Graph.h"
#include "MaxHeap.h"
#include "Queue.h"

#include <string>
#include <utility>
#include <vector>

namespace traffic {

enum class TrafficLightPhase { Red, Yellow, Green };
enum class RoadAxis { NorthSouth, EastWest };

struct LanePriority {
    int fromNeighbor = -1;
    std::string laneName;
    double score = 0.0;
};

struct LanePriorityHigher {
    bool operator()(const LanePriority& left, const LanePriority& right) const;
};

struct LaneState {
    int fromNeighbor = -1;
    std::string name;
    Queue<int> waitingVehicles;
    float waitingAge = 0.0f;
    int approachingVehicles = 0;
    RoadAxis axis = RoadAxis::NorthSouth;

    double priorityScore() const;
    int liveVehicleCount() const {
        return static_cast<int>(waitingVehicles.size()) + approachingVehicles;
    }
};

// One controller per junction; a custom max-heap ranks its incoming lanes.
class TrafficSignal {
private:
    std::vector<LaneState> lanes_;
    MaxHeap<LanePriority, LanePriorityHigher> priorityHeap_;
    int greenNeighbor_ = -1;
    int emergencyNeighbor_ = -1;
    int pendingNeighbor_ = -1;
    TrafficLightPhase phase_ = TrafficLightPhase::Green;
    float greenElapsed_ = 0.0f;
    float yellowElapsed_ = 0.0f;
    float priorityRefreshElapsed_ = 0.0f;
    float dischargeElapsed_ = 0.0f;
    std::size_t fixedLaneIndex_ = 0;
    std::size_t lastServedLaneIndex_ = 0;

    LaneState* findLane(int fromNeighbor);
    const LaneState* findLane(int fromNeighbor) const;
    void refreshHeap();
    int bestLaneForAxis(RoadAxis axis) const;
    int bestOtherAxisLane() const;
    void beginTransition(int nextNeighbor);
    bool sameAxis(int firstNeighbor, int secondNeighbor) const;
    bool isGreenNeighbor(int fromNeighbor) const;

public:
    void initialize(const Graph& graph, int junction);
    void update(float deltaTime, bool smartMode);
    bool enqueue(int fromNeighbor, int vehicleId, bool emergency);
    bool popServedVehicle(std::pair<int, int>& servedVehicle);
    void setApproachingCount(int fromNeighbor, int count);
    void setEmergencyOverride(int fromNeighbor);
    void clearEmergencyOverride(int fromNeighbor);
    TrafficLightPhase phaseForNeighbor(int fromNeighbor) const;
    int greenNeighbor() const { return greenNeighbor_; }
    int overrideNeighbor() const { return emergencyNeighbor_; }
    bool emergencyOverrideActive() const { return emergencyNeighbor_ >= 0; }
    const std::vector<LaneState>& lanes() const { return lanes_; }
    std::vector<LanePriority> heapOrder() const;
    std::string greenNeighborName() const;
    std::string greenAxisName() const;
    int liveVehicleCount() const;
    bool hasLane(int fromNeighbor) const;
};

}  // namespace traffic

#endif
