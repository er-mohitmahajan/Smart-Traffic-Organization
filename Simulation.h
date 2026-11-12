#ifndef TRAFFIC_SIMULATION_H
#define TRAFFIC_SIMULATION_H

#include "Dijkstra.h"
#include "TrafficConfig.h"
#include "TrafficSignal.h"
#include "Vehicle.h"

#include <random>
#include <string>
#include <vector>

namespace traffic {

class Simulation {
private:
    Graph graph_;
    std::vector<TrafficSignal> signals_;
    std::vector<Vehicle> vehicles_;
    std::mt19937 randomEngine_;
    ModeStatistics smartStatistics_;
    ModeStatistics fixedStatistics_;
    TrafficMode mode_ = TrafficMode::Smart;
    int selectedJunction_ = 4;
    int emergencyDestination_ = 8;
    int activeEmergencyVehicle_ = -1;
    float spawnRate_ = config::defaultSpawnRate;
    float vehicleSpeedMultiplier_ = config::defaultVehicleSpeedMultiplier;
    float spawnElapsed_ = 0.0f;
    float congestionElapsed_ = 0.0f;
    float elapsedSeconds_ = 0.0f;
    float comparisonElapsed_ = 0.0f;
    bool paused_ = false;
    bool comparisonActive_ = false;

    void createRoadNetwork();
    void updateCongestion();
    void updateVehicles(float deltaTime);
    void moveVehicleToNextNode(Vehicle& vehicle);
    void finishVehicle(Vehicle& vehicle);
    void replanEmergencyRoute();
    int randomBoundaryNode();
    int randomNodeExcept(int excluded);
    int createVehicle(const std::vector<int>& path, bool emergency);
    void updateEmergencyApproaches();
public:
    Simulation();
    explicit Simulation(unsigned int randomSeed);
    void reset();
    void update(float deltaTime);
    void toggleMode();
    void togglePaused() { paused_ = !paused_; }
    void startComparison();
    void changeSpawnRate(float adjustment);
    void changeVehicleSpeed(float adjustment);
    void spawnEmergency();
    void selectJunction(Point mousePosition);

    const Graph& graph() const { return graph_; }
    const std::vector<Vehicle>& vehicles() const { return vehicles_; }
    const TrafficSignal& signal(int junction) const;
    TrafficMode mode() const { return mode_; }
    bool paused() const { return paused_; }
    float spawnRate() const { return spawnRate_; }
    float vehicleSpeedMultiplier() const { return vehicleSpeedMultiplier_; }
    int selectedJunction() const { return selectedJunction_; }
    int emergencyDestination() const { return emergencyDestination_; }
    bool emergencyActive() const { return activeEmergencyVehicle_ >= 0; }
    int activeVehicleCount() const;
    int liveVehicleCount(int junction) const;
    const std::vector<int>& emergencyPath() const;
    std::string emergencyStartName() const;
    std::string emergencyDestinationName() const;
    const ModeStatistics& statistics(TrafficMode mode) const;
    float elapsedSeconds() const { return elapsedSeconds_; }
    bool comparisonActive() const { return comparisonActive_; }
    float comparisonSecondsRemaining() const;
    static bool pathContainsEdge(const std::vector<int>& path, int from, int to);
};

}  // namespace traffic

#endif
