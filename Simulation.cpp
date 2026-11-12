#include "Simulation.h"

#include "TrafficConfig.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace traffic {
namespace {

Point lanePosition(Point start, Point end, float progress) {
    const float dx = end.x - start.x;
    const float dy = end.y - start.y;
    const float length = std::sqrt(dx * dx + dy * dy);
    if (length <= 0.0f) {
        return start;
    }
    const float offsetX = -dy / length * config::vehicleLaneOffset;
    const float offsetY = dx / length * config::vehicleLaneOffset;
    const float ratio = progress / length;
    return {start.x + dx * ratio + offsetX,
            start.y + dy * ratio + offsetY};
}

}  // namespace

Simulation::Simulation() : Simulation(std::random_device{}()) {}

Simulation::Simulation(unsigned int randomSeed) : randomEngine_(randomSeed) {
    reset();
}

float Simulation::comparisonSecondsRemaining() const {
    return comparisonActive_
               ? std::max(0.0f,
                          config::comparisonDurationSeconds - comparisonElapsed_)
               : 0.0f;
}

void Simulation::createRoadNetwork() {
    constexpr float xPositions[] = {120.0f, 420.0f, 720.0f};
    constexpr float yPositions[] = {130.0f, 430.0f, 730.0f};

    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            const std::string name =
                "J" + std::to_string(row * 3 + column + 1);
            graph_.addNode(name, {xPositions[column], yPositions[row]});
        }
    }

    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            const int node = row * 3 + column;
            if (column < 2) {
                graph_.addEdge(node, node + 1, 2.8f);
            }
            if (row < 2) {
                graph_.addEdge(node, node + 3, 2.8f);
            }
        }
    }

    signals_.resize(graph_.size());
    for (std::size_t junction = 0; junction < signals_.size(); ++junction) {
        signals_[junction].initialize(graph_, static_cast<int>(junction));
    }
}

void Simulation::reset() {
    graph_ = Graph{};
    signals_.clear();
    vehicles_.clear();
    smartStatistics_ = {};
    fixedStatistics_ = {};
    mode_ = TrafficMode::Smart;
    selectedJunction_ = 4;
    emergencyDestination_ = 8;
    activeEmergencyVehicle_ = -1;
    spawnRate_ = config::defaultSpawnRate;
    vehicleSpeedMultiplier_ = config::defaultVehicleSpeedMultiplier;
    spawnElapsed_ = 0.0f;
    congestionElapsed_ = 0.0f;
    elapsedSeconds_ = 0.0f;
    comparisonElapsed_ = 0.0f;
    paused_ = false;
    comparisonActive_ = false;
    createRoadNetwork();
}

const TrafficSignal& Simulation::signal(int junction) const {
    return signals_[static_cast<std::size_t>(junction)];
}

const ModeStatistics& Simulation::statistics(TrafficMode mode) const {
    return mode == TrafficMode::Smart ? smartStatistics_ : fixedStatistics_;
}

int Simulation::activeVehicleCount() const {
    return static_cast<int>(std::count_if(
        vehicles_.begin(), vehicles_.end(),
        [](const Vehicle& vehicle) { return vehicle.active; }));
}

int Simulation::liveVehicleCount(int junction) const {
    if (junction < 0 || static_cast<std::size_t>(junction) >= signals_.size()) {
        return 0;
    }
    return signals_[static_cast<std::size_t>(junction)].liveVehicleCount();
}

void Simulation::updateCongestion() {
    // SIGNAL LOGIC UPDATE: refresh edge costs and detector counts from live routes.
    for (std::size_t from = 0; from < graph_.size(); ++from) {
        for (const Edge& edge : graph_.nodes()[from].edges) {
            int vehiclesOnRoad = 0;
            for (const Vehicle& vehicle : vehicles_) {
                if (!vehicle.active) {
                    continue;
                }
                if (!vehicle.waiting &&
                    ((vehicle.currentNode == static_cast<int>(from) &&
                      vehicle.nextNode == edge.to) ||
                     (vehicle.currentNode == edge.to &&
                      vehicle.nextNode == static_cast<int>(from)))) {
                    ++vehiclesOnRoad;
                } else if (vehicle.waiting && vehicle.routeIndex > 0 &&
                           vehicle.route[vehicle.routeIndex - 1] ==
                               static_cast<int>(from) &&
                           vehicle.currentNode == edge.to) {
                    ++vehiclesOnRoad;
                }
            }
            if (static_cast<int>(from) < edge.to) {
                graph_.updateCongestion(
                    static_cast<int>(from), edge.to,
                    static_cast<float>(vehiclesOnRoad) * 0.8f);
            }
        }
    }

    for (std::size_t junction = 0; junction < signals_.size(); ++junction) {
        for (const LaneState& lane : signals_[junction].lanes()) {
            int approaching = 0;
            for (const Vehicle& vehicle : vehicles_) {
                if (!vehicle.active || vehicle.waiting ||
                    vehicle.nextNode != static_cast<int>(junction) ||
                    vehicle.currentNode != lane.fromNeighbor ||
                    vehicle.routeIndex + 1 >= vehicle.route.size() ||
                    vehicle.route[vehicle.routeIndex + 1] !=
                        static_cast<int>(junction)) {
                    continue;
                }
                const Edge* edge =
                    graph_.findEdge(vehicle.currentNode, vehicle.nextNode);
                if (edge == nullptr) {
                    continue;
                }
                const Point start = graph_.nodes()[static_cast<std::size_t>(
                    vehicle.currentNode)].position;
                const Point end = graph_.nodes()[static_cast<std::size_t>(
                    vehicle.nextNode)].position;
                const float dx = end.x - start.x;
                const float dy = end.y - start.y;
                const float length = std::sqrt(dx * dx + dy * dy);
                if (length - vehicle.edgeProgress <= config::detectionRange) {
                    ++approaching;
                }
            }
            signals_[junction].setApproachingCount(lane.fromNeighbor,
                                                    approaching);
        }
    }
    replanEmergencyRoute();
}

int Simulation::randomBoundaryNode() {
    std::uniform_int_distribution<int> distribution(0, 7);
    const int index = distribution(randomEngine_);
    return index < 4 ? index : index + 1;
}

int Simulation::randomNodeExcept(int excluded) {
    std::uniform_int_distribution<int> distribution(
        0, static_cast<int>(graph_.size()) - 2);
    int node = distribution(randomEngine_);
    if (node >= excluded) {
        ++node;
    }
    return node;
}

int Simulation::createVehicle(const std::vector<int>& path, bool emergency) {
    if (path.size() < 2 ||
        activeVehicleCount() >= config::maximumActiveVehicles) {
        return -1;
    }

    int id = -1;
    for (std::size_t index = 0; index < vehicles_.size(); ++index) {
        if (!vehicles_[index].active) {
            id = static_cast<int>(index);
            break;
        }
    }
    if (id < 0) {
        id = static_cast<int>(vehicles_.size());
        vehicles_.emplace_back();
    }

    Vehicle vehicle;
    vehicle.id = id;
    vehicle.route = path;
    vehicle.routeIndex = 0;
    vehicle.currentNode = path.front();
    vehicle.nextNode = path[1];
    vehicle.position = graph_.nodes()[static_cast<std::size_t>(path.front())].position;
    vehicle.speed = emergency ? config::emergencyVehicleSpeed
                              : config::normalVehicleSpeed;
    vehicle.currentSpeed = 0.0f;
    vehicle.active = true;
    vehicle.emergency = emergency;
    vehicle.spawnMode = mode_;
    if (!emergency) {
        std::uniform_int_distribution<int> colorChannel(65, 225);
        vehicle.bodyRed = static_cast<std::uint8_t>(colorChannel(randomEngine_));
        vehicle.bodyGreen = static_cast<std::uint8_t>(colorChannel(randomEngine_));
        vehicle.bodyBlue = static_cast<std::uint8_t>(colorChannel(randomEngine_));
    }
    const Point start = graph_.nodes()[static_cast<std::size_t>(path.front())].position;
    const Point next = graph_.nodes()[static_cast<std::size_t>(path[1])].position;
    vehicle.position = lanePosition(start, next, 0.0f);
    vehicles_[static_cast<std::size_t>(id)] = std::move(vehicle);
    if (emergency) {
        activeEmergencyVehicle_ = id;
    }
    return id;
}

void Simulation::finishVehicle(Vehicle& vehicle) {
    if (vehicle.collectStatistics) {
        ModeStatistics& stats = vehicle.spawnMode == TrafficMode::Smart
                                    ? smartStatistics_
                                    : fixedStatistics_;
        ++stats.vehiclesCleared;
        stats.totalWaitSeconds += vehicle.waitSeconds;
    }
    if (vehicle.id == activeEmergencyVehicle_) {
        if (vehicle.routeIndex > 0 &&
            static_cast<std::size_t>(vehicle.currentNode) < signals_.size()) {
            signals_[static_cast<std::size_t>(vehicle.currentNode)]
                .clearEmergencyOverride(
                    vehicle.route[vehicle.routeIndex - 1]);
        }
        activeEmergencyVehicle_ = -1;
    }
    vehicle.active = false;
    vehicle.waiting = false;
}

void Simulation::moveVehicleToNextNode(Vehicle& vehicle) {
    if (vehicle.routeIndex + 1 >= vehicle.route.size()) {
        finishVehicle(vehicle);
        return;
    }
    vehicle.currentNode = vehicle.route[vehicle.routeIndex];
    vehicle.nextNode = vehicle.route[vehicle.routeIndex + 1];
    vehicle.edgeProgress = 0.0f;
    vehicle.currentSpeed = 0.0f;
    vehicle.waiting = false;
    vehicle.position = lanePosition(
        graph_.nodes()[static_cast<std::size_t>(vehicle.currentNode)].position,
        graph_.nodes()[static_cast<std::size_t>(vehicle.nextNode)].position,
        0.0f);
    if (vehicle.emergency && vehicle.currentNode >= 0 &&
        static_cast<std::size_t>(vehicle.currentNode) < signals_.size()) {
        signals_[static_cast<std::size_t>(vehicle.currentNode)]
            .clearEmergencyOverride(vehicle.route[vehicle.routeIndex - 1]);
    }
}

void Simulation::updateVehicles(float deltaTime) {
    for (Vehicle& vehicle : vehicles_) {
        if (!vehicle.active) {
            continue;
        }
        if (vehicle.waiting) {
            vehicle.waitSeconds += deltaTime;
            vehicle.currentSpeed = 0.0f;
            continue;
        }

        const Edge* edge = graph_.findEdge(vehicle.currentNode, vehicle.nextNode);
        if (edge == nullptr) {
            finishVehicle(vehicle);
            continue;
        }
        const Point start =
            graph_.nodes()[static_cast<std::size_t>(vehicle.currentNode)].position;
        const Point end =
            graph_.nodes()[static_cast<std::size_t>(vehicle.nextNode)].position;
        const float dx = end.x - start.x;
        const float dy = end.y - start.y;
        const float length = std::sqrt(dx * dx + dy * dy);

        float targetSpeed = vehicle.speed * vehicleSpeedMultiplier_;
        float nearestLeaderProgress = length;
        float leaderSpeed = 0.0f;
        bool leaderFound = false;
        for (const Vehicle& other : vehicles_) {
            if (!other.active || other.id == vehicle.id) {
                continue;
            }
            float otherProgress = -1.0f;
            float otherSpeed = 0.0f;
            if (!other.waiting && other.currentNode == vehicle.currentNode &&
                other.nextNode == vehicle.nextNode) {
                otherProgress = other.edgeProgress;
                otherSpeed = other.currentSpeed;
            } else if (other.waiting && other.currentNode == vehicle.nextNode &&
                       other.routeIndex > 0 &&
                       other.route[other.routeIndex - 1] == vehicle.currentNode) {
                otherProgress = length;
            }
            if (otherProgress > vehicle.edgeProgress &&
                (!leaderFound || otherProgress < nearestLeaderProgress)) {
                nearestLeaderProgress = otherProgress;
                leaderSpeed = otherSpeed;
                leaderFound = true;
            }
        }

        if (leaderFound) {
            const float availableGap =
                std::max(0.0f, nearestLeaderProgress - vehicle.edgeProgress -
                                   config::vehicleSafeGap);
            const float safeFollowingSpeed = std::sqrt(
                leaderSpeed * leaderSpeed +
                2.0f * config::vehicleBraking * availableGap);
            targetSpeed = std::min(targetSpeed, safeFollowingSpeed);
        }

        const float acceleration =
            targetSpeed < vehicle.currentSpeed ? config::vehicleBraking
                                               : config::vehicleAcceleration;
        const float speedDelta = acceleration * deltaTime;
        if (vehicle.currentSpeed < targetSpeed) {
            vehicle.currentSpeed =
                std::min(targetSpeed, vehicle.currentSpeed + speedDelta);
        } else {
            vehicle.currentSpeed =
                std::max(targetSpeed, vehicle.currentSpeed - speedDelta);
        }
        vehicle.edgeProgress =
            std::min(length, vehicle.edgeProgress + vehicle.currentSpeed * deltaTime);
        vehicle.position = lanePosition(start, end, vehicle.edgeProgress);

        if (vehicle.edgeProgress < length) {
            continue;
        }

        const int fromNeighbor = vehicle.currentNode;
        vehicle.currentNode = vehicle.nextNode;
        ++vehicle.routeIndex;
        vehicle.position = lanePosition(end, end, 0.0f);
        if (vehicle.routeIndex + 1 >= vehicle.route.size()) {
            finishVehicle(vehicle);
            continue;
        }

        vehicle.waiting = true;
        vehicle.currentSpeed = 0.0f;
        const bool queued =
            signals_[static_cast<std::size_t>(vehicle.currentNode)]
                .enqueue(fromNeighbor, vehicle.id,
                         vehicle.emergency && mode_ == TrafficMode::Smart);
        if (!queued) {
            finishVehicle(vehicle);
        }
    }
}

void Simulation::updateEmergencyApproaches() {
    // EMERGENCY PREEMPTION: clear passed junctions and reserve the next route approaches.
    if (mode_ != TrafficMode::Smart || activeEmergencyVehicle_ < 0 ||
        static_cast<std::size_t>(activeEmergencyVehicle_) >= vehicles_.size()) {
        for (TrafficSignal& signalController : signals_) {
            const int overrideNeighbor = signalController.overrideNeighbor();
            if (overrideNeighbor >= 0) {
                signalController.clearEmergencyOverride(overrideNeighbor);
            }
        }
        return;
    }
    const Vehicle& ambulance =
        vehicles_[static_cast<std::size_t>(activeEmergencyVehicle_)];
    if (!ambulance.active) {
        return;
    }

    std::vector<int> approachFrom(signals_.size(), -1);
    std::size_t nextPathIndex = ambulance.routeIndex + 1;
    float distanceToNext = 0.0f;
    if (ambulance.waiting && ambulance.routeIndex > 0) {
        const int junction = ambulance.route[ambulance.routeIndex];
        approachFrom[static_cast<std::size_t>(junction)] =
            ambulance.route[ambulance.routeIndex - 1];
        nextPathIndex = ambulance.routeIndex + 1;
    } else if (nextPathIndex < ambulance.route.size()) {
        const Edge* edge =
            graph_.findEdge(ambulance.currentNode, ambulance.nextNode);
        if (edge != nullptr) {
            const Point start = graph_.nodes()[static_cast<std::size_t>(
                ambulance.currentNode)].position;
            const Point end = graph_.nodes()[static_cast<std::size_t>(
                ambulance.nextNode)].position;
            const float dx = end.x - start.x;
            const float dy = end.y - start.y;
            distanceToNext =
                std::max(0.0f, std::sqrt(dx * dx + dy * dy) -
                                   ambulance.edgeProgress);
        }
    }

    float distanceAlongPath = distanceToNext;
    for (std::size_t pathIndex = nextPathIndex;
         pathIndex < ambulance.route.size() &&
         distanceAlongPath <= config::emergencyLookAheadRange;
         ++pathIndex) {
        const int junction = ambulance.route[pathIndex];
        approachFrom[static_cast<std::size_t>(junction)] =
            ambulance.route[pathIndex - 1];
        if (pathIndex + 1 < ambulance.route.size()) {
            const Edge* edge = graph_.findEdge(
                ambulance.route[pathIndex], ambulance.route[pathIndex + 1]);
            if (edge != nullptr) {
                const Point start = graph_.nodes()[static_cast<std::size_t>(
                    ambulance.route[pathIndex])].position;
                const Point end = graph_.nodes()[static_cast<std::size_t>(
                    ambulance.route[pathIndex + 1])].position;
                const float dx = end.x - start.x;
                const float dy = end.y - start.y;
                distanceAlongPath += std::sqrt(dx * dx + dy * dy);
            }
        }
    }

    for (std::size_t junction = 0; junction < signals_.size(); ++junction) {
        const int target = approachFrom[junction];
        const int activeOverride = signals_[junction].overrideNeighbor();
        if (target >= 0) {
            signals_[junction].setEmergencyOverride(target);
        } else if (activeOverride >= 0) {
            signals_[junction].clearEmergencyOverride(activeOverride);
        }
    }
}

void Simulation::replanEmergencyRoute() {
    if (mode_ != TrafficMode::Smart || activeEmergencyVehicle_ < 0 ||
        static_cast<std::size_t>(activeEmergencyVehicle_) >= vehicles_.size()) {
        return;
    }
    Vehicle& ambulance =
        vehicles_[static_cast<std::size_t>(activeEmergencyVehicle_)];
    if (!ambulance.active || !ambulance.waiting ||
        ambulance.routeIndex + 1 >= ambulance.route.size()) {
        return;
    }

    const std::vector<int> tail =
        findShortestPath(graph_, ambulance.currentNode, emergencyDestination_);
    if (tail.empty()) {
        return;
    }
    std::vector<int> rerouted;
    rerouted.insert(rerouted.end(), ambulance.route.begin(),
                    ambulance.route.begin() +
                        static_cast<std::ptrdiff_t>(ambulance.routeIndex + 1));
    if (tail.size() > 1) {
        rerouted.insert(rerouted.end(), tail.begin() + 1, tail.end());
    }
    if (rerouted != ambulance.route) {
        ambulance.route = std::move(rerouted);
    }
}

void Simulation::update(float deltaTime) {
    if (paused_ || deltaTime <= 0.0f) {
        return;
    }
    deltaTime = std::min(deltaTime, config::simulationMaximumDeltaTime);
    elapsedSeconds_ += deltaTime;
    if (comparisonActive_) {
        // C comparison uses the same simulation: 30 seconds per signal strategy.
        const float previousComparisonElapsed = comparisonElapsed_;
        comparisonElapsed_ += deltaTime;
        if (previousComparisonElapsed < config::comparisonHalfSeconds &&
            comparisonElapsed_ >= config::comparisonHalfSeconds -
                                      config::comparisonBoundaryTolerance) {
            mode_ = TrafficMode::Fixed;
        }
        if (comparisonElapsed_ >= config::comparisonDurationSeconds -
                                      config::comparisonBoundaryTolerance) {
            comparisonElapsed_ = config::comparisonDurationSeconds;
            comparisonActive_ = false;
        }
    }
    updateVehicles(deltaTime);
    updateEmergencyApproaches();

    for (TrafficSignal& signalController : signals_) {
        signalController.update(deltaTime, mode_ == TrafficMode::Smart);
    }

    for (std::size_t junction = 0; junction < signals_.size(); ++junction) {
        std::pair<int, int> servedVehicle;
        if (!signals_[junction].popServedVehicle(servedVehicle)) {
            continue;
        }
        const int vehicleId = servedVehicle.second;
        if (vehicleId < 0 ||
            static_cast<std::size_t>(vehicleId) >= vehicles_.size()) {
            continue;
        }
        Vehicle& vehicle = vehicles_[static_cast<std::size_t>(vehicleId)];
        if (vehicle.active && vehicle.waiting) {
            moveVehicleToNextNode(vehicle);
        }
    }

    spawnElapsed_ += deltaTime;
    const float spawnInterval =
        std::max(config::minimumSpawnIntervalSeconds, 1.0f / spawnRate_);
    if (spawnElapsed_ >= spawnInterval) {
        spawnElapsed_ = std::fmod(spawnElapsed_, spawnInterval);
        if (activeVehicleCount() < config::maximumActiveVehicles) {
            const int start = randomBoundaryNode();
            const int destination = randomNodeExcept(start);
            createVehicle(findShortestPath(graph_, start, destination), false);
        }
    }

    congestionElapsed_ += deltaTime;
    if (congestionElapsed_ >= 0.5f) {
        congestionElapsed_ = 0.0f;
        updateCongestion();
    }
}

void Simulation::toggleMode() {
    comparisonActive_ = false;
    mode_ = mode_ == TrafficMode::Smart ? TrafficMode::Fixed : TrafficMode::Smart;
}

void Simulation::startComparison() {
    smartStatistics_ = {};
    fixedStatistics_ = {};
    for (Vehicle& vehicle : vehicles_) {
        if (vehicle.active) {
            vehicle.collectStatistics = false;
        }
    }
    comparisonElapsed_ = 0.0f;
    comparisonActive_ = true;
    mode_ = TrafficMode::Smart;
}

void Simulation::changeSpawnRate(float adjustment) {
    spawnRate_ = std::clamp(spawnRate_ + adjustment,
                            config::minimumSpawnRate,
                            config::maximumSpawnRate);
}

void Simulation::changeVehicleSpeed(float adjustment) {
    const float updatedSpeed =
        vehicleSpeedMultiplier_ + adjustment;
    vehicleSpeedMultiplier_ = std::clamp(
        std::round(updatedSpeed * 10.0f) / 10.0f,
        config::minimumVehicleSpeedMultiplier,
        config::maximumVehicleSpeedMultiplier);
}

void Simulation::spawnEmergency() {
    if (emergencyActive()) {
        return;
    }
    const int start = randomBoundaryNode();
    int destination = emergencyDestination_;
    if (destination == start) {
        destination = randomNodeExcept(start);
        emergencyDestination_ = destination;
    }
    const std::vector<int> path = findShortestPath(graph_, start, destination);
    createVehicle(path, true);
}

void Simulation::selectJunction(Point mousePosition) {
    constexpr float selectionRadius = 30.0f;
    for (std::size_t junction = 0; junction < graph_.size(); ++junction) {
        const Point position = graph_.nodes()[junction].position;
        const float dx = mousePosition.x - position.x;
        const float dy = mousePosition.y - position.y;
        if (dx * dx + dy * dy <= selectionRadius * selectionRadius) {
            selectedJunction_ = static_cast<int>(junction);
            emergencyDestination_ = static_cast<int>(junction);
            return;
        }
    }
}

const std::vector<int>& Simulation::emergencyPath() const {
    static const std::vector<int> emptyPath;
    if (!emergencyActive() ||
        static_cast<std::size_t>(activeEmergencyVehicle_) >= vehicles_.size()) {
        return emptyPath;
    }
    return vehicles_[static_cast<std::size_t>(activeEmergencyVehicle_)].route;
}

std::string Simulation::emergencyStartName() const {
    const std::vector<int>& path = emergencyPath();
    return path.empty() ? "?" : graph_.nodes()[static_cast<std::size_t>(path.front())].name;
}

std::string Simulation::emergencyDestinationName() const {
    return graph_.nodes()[static_cast<std::size_t>(emergencyDestination_)].name;
}

bool Simulation::pathContainsEdge(const std::vector<int>& path, int from, int to) {
    for (std::size_t index = 1; index < path.size(); ++index) {
        if ((path[index - 1] == from && path[index] == to) ||
            (path[index - 1] == to && path[index] == from)) {
            return true;
        }
    }
    return false;
}

}  // namespace traffic
