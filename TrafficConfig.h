#ifndef TRAFFIC_CONFIG_H
#define TRAFFIC_CONFIG_H

namespace traffic {
namespace config {

// Visual tuning: all distances are screen pixels at the current map scale.
constexpr float roadWidth = 30.0f;
constexpr float roadEdgeInset = 14.0f;
constexpr float vehicleLaneOffset = 7.0f;
constexpr float vehicleLength = 18.0f;
constexpr float vehicleWidth = 10.0f;
constexpr float vehicleSafeGap = 25.0f;
constexpr float junctionHubRadius = 13.0f;
constexpr float simulationMaximumDeltaTime = 0.1f;
constexpr float minimumSpawnIntervalSeconds = 0.15f;
constexpr int maximumActiveVehicles = 180;
constexpr float comparisonHalfSeconds = 30.0f;
constexpr float comparisonDurationSeconds = 60.0f;
constexpr float comparisonBoundaryTolerance = 0.01f;

// Signal timing and detector tuning.
constexpr float detectionRange = 430.0f;
constexpr float emergencyLookAheadRange = 630.0f;
constexpr float smartMinimumGreenSeconds = 5.0f;
constexpr float smartMaximumGreenSeconds = 20.0f;
constexpr float fixedGreenSeconds = 10.0f;
constexpr float yellowSeconds = 1.5f;
constexpr float heapRefreshSeconds = 0.5f;
constexpr float vehicleHeadwaySeconds = 0.55f;
constexpr double approachTrafficWeight = 0.5;
constexpr double waitingTimeWeight = 0.2;

// Vehicle motion, spawn, and emergency light tuning.
constexpr float normalVehicleSpeed = 110.0f;
constexpr float emergencyVehicleSpeed = 145.0f;
constexpr float vehicleAcceleration = 180.0f;
constexpr float vehicleBraking = 320.0f;
constexpr float defaultVehicleSpeedMultiplier = 1.0f;
constexpr float minimumVehicleSpeedMultiplier = 0.5f;
constexpr float maximumVehicleSpeedMultiplier = 2.0f;
constexpr float vehicleSpeedMultiplierStep = 0.1f;
constexpr float defaultSpawnRate = 1.7f;
constexpr float spawnRateStep = 0.2f;
constexpr float minimumSpawnRate = 0.2f;
constexpr float maximumSpawnRate = 3.0f;
constexpr float emergencyFlashSeconds = 0.2f;

}  // namespace config
}  // namespace traffic

#endif
