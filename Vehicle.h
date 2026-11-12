#ifndef TRAFFIC_VEHICLE_H
#define TRAFFIC_VEHICLE_H

#include "Graph.h"

#include <cstdint>
#include <vector>

namespace traffic {

enum class TrafficMode { Smart, Fixed };

struct ModeStatistics {
    int vehiclesCleared = 0;
    double totalWaitSeconds = 0.0;

    double averageWait() const {
        return vehiclesCleared == 0
                   ? 0.0
                   : totalWaitSeconds / static_cast<double>(vehiclesCleared);
    }
};

struct Vehicle {
    int id = -1;
    std::vector<int> route;
    std::size_t routeIndex = 0;
    int currentNode = -1;
    int nextNode = -1;
    Point position;
    float edgeProgress = 0.0f;
    float speed = 105.0f;
    float currentSpeed = 0.0f;
    float waitSeconds = 0.0f;
    // VISUAL OVERHAUL: per-vehicle paint is stored without coupling the model to raylib.
    std::uint8_t bodyRed = 56;
    std::uint8_t bodyGreen = 189;
    std::uint8_t bodyBlue = 248;
    bool waiting = false;
    bool active = false;
    bool emergency = false;
    bool collectStatistics = true;
    TrafficMode spawnMode = TrafficMode::Smart;
};

}  // namespace traffic

#endif
