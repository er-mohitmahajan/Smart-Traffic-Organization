#include "Simulation.h"
#include "TrafficConfig.h"

#include <cassert>
#include <iostream>

void testPriorityAndPhases() {
    traffic::LaneState lane;
    lane.waitingVehicles.enqueue(1);
    lane.waitingVehicles.enqueue(2);
    lane.approachingVehicles = 4;
    lane.waitingAge = 10.0f;
    assert(lane.priorityScore() ==
           2.0 + 4.0 * traffic::config::approachTrafficWeight +
               10.0 * traffic::config::waitingTimeWeight);

    traffic::Graph graph;
    const int center = graph.addNode("E", {100.0f, 100.0f});
    const int east = graph.addNode("F", {400.0f, 100.0f});
    const int west = graph.addNode("D", {-200.0f, 100.0f});
    const int north = graph.addNode("B", {100.0f, -200.0f});
    const int south = graph.addNode("H", {100.0f, 400.0f});
    assert(graph.addEdge(center, east, 1.0f));
    assert(graph.addEdge(center, west, 1.0f));
    assert(graph.addEdge(center, north, 1.0f));
    assert(graph.addEdge(center, south, 1.0f));

    traffic::TrafficSignal smart;
    smart.initialize(graph, center);
    smart.enqueue(north, 10, false);
    smart.enqueue(south, 11, false);
    smart.setApproachingCount(north, 20);
    for (int step = 0; step < 52; ++step) {
        smart.update(0.1f, true);
    }
    assert(smart.phaseForNeighbor(east) == traffic::TrafficLightPhase::Yellow);
    assert(smart.phaseForNeighbor(north) == traffic::TrafficLightPhase::Red);
    for (int step = 0; step < 16; ++step) {
        smart.update(0.1f, true);
    }
    assert(smart.phaseForNeighbor(north) == traffic::TrafficLightPhase::Green);
    assert(smart.phaseForNeighbor(south) == traffic::TrafficLightPhase::Green);

    std::pair<int, int> served;
    smart.update(0.6f, true);
    assert(smart.popServedVehicle(served) && served.second == 10);
    smart.update(0.6f, true);
    assert(smart.popServedVehicle(served) && served.second == 11);

    traffic::TrafficSignal emergency;
    emergency.initialize(graph, center);
    emergency.setEmergencyOverride(north);
    assert(emergency.phaseForNeighbor(east) ==
           traffic::TrafficLightPhase::Yellow);
    assert(emergency.phaseForNeighbor(north) ==
           traffic::TrafficLightPhase::Red);
    emergency.update(traffic::config::yellowSeconds, true);
    assert(emergency.phaseForNeighbor(north) ==
           traffic::TrafficLightPhase::Green);
    assert(emergency.phaseForNeighbor(south) ==
           traffic::TrafficLightPhase::Red);

    traffic::TrafficSignal fixed;
    fixed.initialize(graph, center);
    fixed.setApproachingCount(north, 100);
    for (int step = 0; step < 101; ++step) {
        fixed.update(0.1f, false);
    }
    assert(fixed.phaseForNeighbor(east) ==
           traffic::TrafficLightPhase::Yellow);
    for (int step = 0; step < 16; ++step) {
        fixed.update(0.1f, false);
    }
    assert(fixed.phaseForNeighbor(north) ==
           traffic::TrafficLightPhase::Green);
    assert(fixed.phaseForNeighbor(south) ==
           traffic::TrafficLightPhase::Green);
}

int main() {
    testPriorityAndPhases();

    traffic::Simulation speedControls(3);
    assert(speedControls.vehicleSpeedMultiplier() ==
           traffic::config::defaultVehicleSpeedMultiplier);
    for (int step = 0; step < 20; ++step) {
        speedControls.changeVehicleSpeed(
            traffic::config::vehicleSpeedMultiplierStep);
    }
    assert(speedControls.vehicleSpeedMultiplier() ==
           traffic::config::maximumVehicleSpeedMultiplier);
    for (int step = 0; step < 30; ++step) {
        speedControls.changeVehicleSpeed(
            -traffic::config::vehicleSpeedMultiplierStep);
    }
    assert(speedControls.vehicleSpeedMultiplier() ==
           traffic::config::minimumVehicleSpeedMultiplier);
    speedControls.reset();
    assert(speedControls.vehicleSpeedMultiplier() ==
           traffic::config::defaultVehicleSpeedMultiplier);

    traffic::Simulation comparison(7);
    comparison.startComparison();
    assert(comparison.comparisonActive());
    for (int step = 0; step < 300; ++step) {
        comparison.update(0.1f);
    }
    assert(comparison.mode() == traffic::TrafficMode::Fixed);
    for (int step = 0; step < 300; ++step) {
        comparison.update(0.1f);
    }
    assert(!comparison.comparisonActive());

    for (unsigned int seed = 0; seed < 20; ++seed) {
        traffic::Simulation simulation(seed);
        assert(simulation.graph().size() == 9);
        assert(simulation.selectedJunction() == 4);

        simulation.selectJunction({120.0f, 130.0f});
        assert(simulation.selectedJunction() == 0);
        assert(simulation.emergencyDestination() == 0);

        simulation.spawnEmergency();
        assert(simulation.emergencyActive());
        const std::vector<int> route = simulation.emergencyPath();
        assert(route.size() >= 2);
        assert(route.back() == simulation.emergencyDestination());
        for (std::size_t index = 1; index < route.size(); ++index) {
            assert(simulation.graph().findEdge(route[index - 1], route[index]) != nullptr);
        }

        simulation.toggleMode();
        assert(simulation.mode() == traffic::TrafficMode::Fixed);
        simulation.toggleMode();
        assert(simulation.mode() == traffic::TrafficMode::Smart);

        for (int step = 0; step < 1500 && simulation.emergencyActive(); ++step) {
            simulation.update(0.1f);
        }
        assert(!simulation.emergencyActive());
        assert(simulation.statistics(traffic::TrafficMode::Smart)
                   .vehiclesCleared >= 1);
        for (std::size_t junction = 0; junction < simulation.graph().size();
             ++junction) {
            assert(!simulation.signal(static_cast<int>(junction))
                        .emergencyOverrideActive());
        }
    }

    std::cout << "Simulation network, emergency route, and mode tests passed.\n";
    return 0;
}
