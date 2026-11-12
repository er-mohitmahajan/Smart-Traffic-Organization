#include "Simulation.h"

#include "TrafficConfig.h"
#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace {

constexpr int screenWidth = 1360;
constexpr int screenHeight = 850;
constexpr int mapWidth = 920;
constexpr float radiansToDegrees = 57.2957795f;
const Rectangle vehicleSpeedDownButton{mapWidth + 284.0f, 130.0f, 30.0f, 24.0f};
const Rectangle vehicleSpeedUpButton{mapWidth + 320.0f, 130.0f, 30.0f, 24.0f};

Color withAlpha(Color color, unsigned char alpha) {
    color.a = alpha;
    return color;
}

Vector2 toRaylib(traffic::Point point) {
    return {point.x, point.y};
}

Vector2 add(Vector2 left, Vector2 right) {
    return {left.x + right.x, left.y + right.y};
}

Vector2 multiply(Vector2 value, float scale) {
    return {value.x * scale, value.y * scale};
}

Vector2 directionBetween(Vector2 start, Vector2 end) {
    const float dx = end.x - start.x;
    const float dy = end.y - start.y;
    const float length = std::sqrt(dx * dx + dy * dy);
    return length > 0.0f ? Vector2{dx / length, dy / length} : Vector2{1.0f, 0.0f};
}

void drawSpeedButton(Rectangle bounds, const char* label, bool hovered) {
    const Color fill = hovered ? Color{82, 105, 88, 255}
                               : Color{67, 82, 70, 255};
    DrawRectangleRounded(bounds, 0.25f, 4, fill);
    DrawRectangleRoundedLines(bounds, 0.25f, 4, Color{143, 166, 145, 255});
    DrawText(label, static_cast<int>(bounds.x) + 10,
             static_cast<int>(bounds.y) + 4, 16, RAYWHITE);
}

void drawCityGround() {
    // REFERENCE UI: flat concrete-gray ground with no decorative city blocks.
    ClearBackground(Color{118, 120, 120, 255});
    for (int x = 0; x < mapWidth; x += 32) {
        DrawLine(x, 0, x, screenHeight - 24, withAlpha(Color{150, 152, 151, 255}, 24));
    }
    for (int y = 0; y < screenHeight - 24; y += 32) {
        DrawLine(0, y, mapWidth, y, withAlpha(Color{150, 152, 151, 255}, 24));
    }
}

void drawDashedCenterLine(Vector2 start, Vector2 end, Vector2 perpendicular,
                          float offset) {
    const Vector2 direction = directionBetween(start, end);
    const float dx = end.x - start.x;
    const float dy = end.y - start.y;
    const float length = std::sqrt(dx * dx + dy * dy);
    constexpr float dashLength = 13.0f;
    constexpr float dashGap = 12.0f;
    for (float distance = 0.0f; distance < length;
         distance += dashLength + dashGap) {
        const float endDistance = std::min(distance + dashLength, length);
        const Vector2 from = add(add(start, multiply(direction, distance)),
                                 multiply(perpendicular, offset));
        const Vector2 to = add(add(start, multiply(direction, endDistance)),
                               multiply(perpendicular, offset));
        DrawLineEx(from, to, 1.4f, Color{220, 218, 198, 255});
    }
}

void drawRoads(const traffic::Simulation& simulation) {
    const traffic::Graph& graph = simulation.graph();
    const std::vector<traffic::Node>& nodes = graph.nodes();

    for (std::size_t nodeId = 0; nodeId < nodes.size(); ++nodeId) {
        for (const traffic::Edge& edge : nodes[nodeId].edges) {
            if (static_cast<int>(nodeId) >= edge.to) {
                continue;
            }
            const Vector2 start = toRaylib(nodes[nodeId].position);
            const Vector2 end =
                toRaylib(nodes[static_cast<std::size_t>(edge.to)].position);
            const Vector2 direction = directionBetween(start, end);
            const Vector2 perpendicular{-direction.y, direction.x};

            // REFERENCE UI: narrow charcoal roads with pale edges and one dashed center line.
            DrawLineEx(start, end, traffic::config::roadWidth + 3.0f,
                       Color{166, 168, 165, 255});
            DrawLineEx(start, end, traffic::config::roadWidth,
                       Color{45, 47, 49, 255});
            for (float side : {-1.0f, 1.0f}) {
                DrawLineEx(add(start, multiply(
                                     perpendicular,
                                     side * traffic::config::roadEdgeInset)),
                           add(end, multiply(
                                   perpendicular,
                                   side * traffic::config::roadEdgeInset)),
                           1.0f, Color{187, 188, 183, 255});
            }
            drawDashedCenterLine(start, end, perpendicular, 0.0f);
        }
    }

    if (!simulation.emergencyActive()) {
        return;
    }
    const std::vector<int>& path = simulation.emergencyPath();
    for (std::size_t index = 1; index < path.size(); ++index) {
        const Vector2 start =
            toRaylib(nodes[static_cast<std::size_t>(path[index - 1])].position);
        const Vector2 end =
            toRaylib(nodes[static_cast<std::size_t>(path[index])].position);
        // VISUAL OVERHAUL: layered lines create a glowing ambulance route.
        DrawLineEx(start, end, 17.0f, withAlpha(Color{255, 54, 91, 255}, 78));
        DrawLineEx(start, end, 7.0f, Color{222, 54, 78, 255});
        DrawLineEx(start, end, 2.0f, Color{255, 207, 160, 255});
    }
}

void drawTrafficLight(Vector2 position, traffic::TrafficLightPhase phase) {
    const Rectangle box{position.x - 4.0f, position.y - 10.0f, 8.0f, 20.0f};
    DrawRectangleRounded(box, 0.25f, 3, Color{28, 29, 30, 255});
    const Color dim{76, 76, 73, 255};
    DrawCircleV({position.x, position.y - 6.0f}, 2.0f,
                phase == traffic::TrafficLightPhase::Red
                    ? Color{239, 68, 68, 255}
                    : dim);
    DrawCircleV(position, 2.0f,
                phase == traffic::TrafficLightPhase::Yellow
                    ? Color{250, 204, 21, 255}
                    : dim);
    DrawCircleV({position.x, position.y + 6.0f}, 2.0f,
                phase == traffic::TrafficLightPhase::Green
                    ? Color{34, 197, 94, 255}
                    : dim);
}

void drawJunctions(const traffic::Simulation& simulation) {
    const traffic::Graph& graph = simulation.graph();
    const std::vector<traffic::Node>& nodes = graph.nodes();

    for (std::size_t nodeId = 0; nodeId < nodes.size(); ++nodeId) {
        const Vector2 position = toRaylib(nodes[nodeId].position);
        const int junction = static_cast<int>(nodeId);
        const traffic::TrafficSignal& signal = simulation.signal(junction);
        const bool selected = junction == simulation.selectedJunction();

        if (selected) {
            // REFERENCE UI: restrained selection ring around the selected junction.
            DrawCircleLines(static_cast<int>(position.x),
                            static_cast<int>(position.y),
                            traffic::config::junctionHubRadius + 7.0f,
                            withAlpha(Color{255, 224, 96, 255}, 130));
        }

        DrawCircleV(position, traffic::config::junctionHubRadius,
                    Color{166, 168, 167, 255});
        DrawCircleLines(static_cast<int>(position.x),
                        static_cast<int>(position.y),
                        traffic::config::junctionHubRadius,
                        Color{68, 70, 71, 255});

        for (const traffic::LaneState& lane : signal.lanes()) {
            const Vector2 neighbor =
                toRaylib(nodes[static_cast<std::size_t>(lane.fromNeighbor)].position);
            const Vector2 towardNeighbor = directionBetween(position, neighbor);
            const Vector2 perpendicular{-towardNeighbor.y, towardNeighbor.x};
            const Vector2 lightPosition = add(
                add(position, multiply(towardNeighbor, 25.0f)),
                multiply(perpendicular, 17.0f));
            drawTrafficLight(lightPosition,
                             signal.phaseForNeighbor(lane.fromNeighbor));
        }

        DrawCircleV(position, 4.0f,
                    selected ? Color{255, 231, 138, 255}
                             : Color{83, 87, 87, 255});
    }
}

void drawVehicle(const traffic::Vehicle& vehicle, Vector2 center,
                 float rotationDegrees, float elapsedSeconds) {
    const float angle = rotationDegrees;
    const float radians = angle / radiansToDegrees;
    const Vector2 forward{std::cos(radians), std::sin(radians)};
    const Color body = vehicle.emergency
                           ? Color{245, 245, 235, 255}
                           : Color{vehicle.bodyRed, vehicle.bodyGreen,
                                   vehicle.bodyBlue, 255};

    // VISUAL OVERHAUL: overlapping rotated shapes form a rounded 18x10 car.
    DrawCircleV(add(center, multiply(forward, -4.0f)),
                traffic::config::vehicleWidth * 0.5f, body);
    DrawRectanglePro(
        {center.x - 5.0f, center.y - 5.0f, 10.0f, 10.0f},
        {5.0f, 5.0f}, angle, body);
    DrawCircleV(add(center, multiply(forward, 4.0f)),
                traffic::config::vehicleWidth * 0.5f, body);

    if (vehicle.emergency) {
        DrawRectanglePro({center.x - 3.0f, center.y - 4.5f, 6.0f, 9.0f},
                         {3.0f, 4.5f}, angle, Color{220, 38, 55, 255});
    }

    const Vector2 windshield = add(center, multiply(forward, 3.0f));
    DrawRectanglePro({windshield.x - 2.0f, windshield.y - 3.2f,
                      4.0f, 6.4f},
                     {2.0f, 3.2f}, angle, Color{37, 57, 66, 255});

    if (vehicle.emergency) {
        const bool redFlash =
            std::fmod(elapsedSeconds,
                      traffic::config::emergencyFlashSeconds * 2.0f) <
            traffic::config::emergencyFlashSeconds;
        const Vector2 bar = add(center, multiply(forward, -1.0f));
        const Vector2 side{-forward.y, forward.x};
        DrawCircleV(add(bar, multiply(side, -2.4f)),
                    1.8f, redFlash ? Color{255, 30, 52, 255}
                                   : Color{35, 91, 255, 255});
        DrawCircleV(add(bar, multiply(side, 2.4f)),
                    1.8f, redFlash ? Color{35, 91, 255, 255}
                                   : Color{255, 30, 52, 255});
    }
}

void drawVehicles(const traffic::Simulation& simulation) {
    const traffic::Graph& graph = simulation.graph();
    const std::vector<traffic::Vehicle>& vehicles = simulation.vehicles();

    for (const traffic::Vehicle& vehicle : vehicles) {
        if (!vehicle.active || vehicle.waiting) {
            continue;
        }
        const Vector2 start =
            toRaylib(graph.nodes()[static_cast<std::size_t>(vehicle.currentNode)]
                         .position);
        const Vector2 end =
            toRaylib(graph.nodes()[static_cast<std::size_t>(vehicle.nextNode)]
                         .position);
        const float angle =
            std::atan2(end.y - start.y, end.x - start.x) * radiansToDegrees;
        drawVehicle(vehicle, toRaylib(vehicle.position), angle,
                    simulation.elapsedSeconds());
    }

    for (std::size_t junction = 0; junction < graph.size(); ++junction) {
        const traffic::TrafficSignal& signal =
            simulation.signal(static_cast<int>(junction));
        const Vector2 node =
            toRaylib(graph.nodes()[junction].position);
        for (const traffic::LaneState& lane : signal.lanes()) {
            const Vector2 from = toRaylib(
                graph.nodes()[static_cast<std::size_t>(lane.fromNeighbor)].position);
            const Vector2 direction = directionBetween(from, node);
            const Vector2 perpendicular{-direction.y, direction.x};
            lane.waitingVehicles.forEach(
                [&](int vehicleId, std::size_t index) {
                    if (vehicleId < 0 ||
                        static_cast<std::size_t>(vehicleId) >= vehicles.size()) {
                        return;
                    }
                    const traffic::Vehicle& vehicle =
                        vehicles[static_cast<std::size_t>(vehicleId)];
                    if (!vehicle.active) {
                        return;
                    }
                    const float distance =
                        traffic::config::roadWidth * 0.5f +
                        traffic::config::vehicleLength * 0.5f + 5.0f +
                        static_cast<float>(index) *
                            traffic::config::vehicleSafeGap;
                    const Vector2 position = add(
                        add(node, multiply(direction, -distance)),
                        multiply(perpendicular,
                                 traffic::config::vehicleLaneOffset));
                    const float angle =
                        std::atan2(direction.y, direction.x) * radiansToDegrees;
                    drawVehicle(vehicle, position, angle,
                                simulation.elapsedSeconds());
                });
        }
    }
}

void drawJunctionLabels(const traffic::Simulation& simulation) {
    const traffic::Graph& graph = simulation.graph();
    for (std::size_t index = 0; index < graph.size(); ++index) {
        const traffic::Node& node = graph.nodes()[index];
        const traffic::TrafficSignal& signal =
            simulation.signal(static_cast<int>(index));
        const std::string number = std::to_string(index + 1);
        const std::string count =
            std::to_string(simulation.liveVehicleCount(static_cast<int>(index)));
        const std::string firstLine =
            "J" + number + ": " + count + " Cars";
        const std::string stateLine = signal.emergencyOverrideActive()
                                          ? "EMERGENCY"
                                          : signal.greenAxisName();
        const Vector2 position = toRaylib(node.position);
        const Rectangle box{position.x - 53.0f, position.y - 72.0f,
                            106.0f, 31.0f};
        const Color labelColor =
            signal.emergencyOverrideActive()
                ? Color{255, 100, 103, 255}
                : signal.greenAxisName().find("YELLOW") != std::string::npos
                      ? Color{250, 204, 21, 255}
                      : signal.greenAxisName().find("N-S") != std::string::npos
                            ? Color{134, 239, 172, 255}
                              : Color{255, 112, 112, 255};
        DrawRectangleRounded(box, 0.04f, 2, Color{22, 23, 24, 245});
        DrawRectangleLinesEx(box, 1.0f, Color{142, 144, 143, 255});
        DrawText(firstLine.c_str(), static_cast<int>(box.x) + 5,
                 static_cast<int>(box.y) + 3, 10, Color{229, 231, 229, 255});
        DrawText(stateLine.c_str(), static_cast<int>(box.x) + 5,
                 static_cast<int>(box.y) + 17, 9, labelColor);
    }
}

void drawEmergencyBanner(const traffic::Simulation& simulation) {
    if (!simulation.emergencyActive()) {
        return;
    }
    const Rectangle banner{288.0f, 45.0f, 344.0f, 27.0f};
    DrawRectangleRounded(banner, 0.04f, 2, Color{35, 29, 30, 245});
    DrawRectangleLinesEx(banner, 1.0f, Color{196, 79, 84, 255});
    DrawText(TextFormat("EMERGENCY OVERRIDE  |  %s -> %s",
                        simulation.emergencyStartName().c_str(),
                        simulation.emergencyDestinationName().c_str()),
             297, 53, 12, WHITE);
}

void drawLegend(int x, int y) {
    DrawText("LEGEND", x, y, 12, Color{207, 216, 202, 255});
    DrawCircle(x + 75, y + 7, 5.0f, Color{34, 197, 94, 255});
    DrawText("Green", x + 84, y + 1, 10, Color{226, 232, 220, 255});
    DrawCircle(x + 147, y + 7, 5.0f, Color{250, 204, 21, 255});
    DrawText("Yellow", x + 156, y + 1, 10, Color{226, 232, 220, 255});
    DrawCircle(x + 229, y + 7, 5.0f, Color{239, 68, 68, 255});
    DrawText("Red", x + 238, y + 1, 10, Color{226, 232, 220, 255});
    DrawCircle(x + 289, y + 7, 5.0f, Color{56, 189, 248, 255});
    DrawText("Car", x + 298, y + 1, 10, Color{226, 232, 220, 255});
}

void drawDashboard(const traffic::Simulation& simulation, int fps) {
    constexpr int panelX = mapWidth;
    constexpr int panelWidth = screenWidth - panelX;
    const Color muted{174, 176, 174, 255};
    const Color text{234, 235, 233, 255};
    const Color panel{55, 56, 56, 255};
    const Color card{36, 37, 37, 255};
    const int selected = simulation.selectedJunction();
    const traffic::TrafficSignal& signal = simulation.signal(selected);
    const traffic::Node& node =
        simulation.graph().nodes()[static_cast<std::size_t>(selected)];

    // VISUAL OVERHAUL: retain the right-side dashboard with a city-green theme.
    DrawRectangle(panelX, 0, panelWidth, screenHeight, panel);
    DrawLine(panelX, 0, panelX, screenHeight, Color{151, 153, 151, 255});
    DrawText("SMART TRAFFIC", panelX + 24, 20, 22, text);
    DrawText("CITY CONTROL / DATA STRUCTURES", panelX + 24, 48, 12, muted);
    DrawLine(panelX + 22, 68, screenWidth - 22, 68,
             Color{123, 125, 123, 255});

    const std::string mode =
        std::string("Mode: ") +
        (simulation.mode() == traffic::TrafficMode::Smart
             ? "SMART DENSITY"
             : "FIXED TIMER");
    DrawText(mode.c_str(), panelX + 24, 82, 16,
             simulation.mode() == traffic::TrafficMode::Smart
                 ? Color{134, 239, 172, 255}
                 : Color{250, 204, 21, 255});
    DrawText(simulation.paused() ? "PAUSED" : "RUNNING",
             panelX + 316, 84, 12,
             simulation.paused() ? Color{248, 113, 113, 255}
                                 : Color{134, 239, 172, 255});
    DrawText(TextFormat("FPS: %d    Active vehicles: %d", fps,
                        simulation.activeVehicleCount()),
             panelX + 24, 110, 14, text);
    DrawText(TextFormat("Spawn rate: %.1f vehicles/sec",
                        simulation.spawnRate()),
             panelX + 24, 134, 13, muted);
    DrawText(TextFormat("Speed: %.1fx", simulation.vehicleSpeedMultiplier()),
             panelX + 190, 134, 13, text);
    const Vector2 mousePosition = GetMousePosition();
    drawSpeedButton(vehicleSpeedDownButton, "-",
                    CheckCollisionPointRec(mousePosition,
                                           vehicleSpeedDownButton));
    drawSpeedButton(vehicleSpeedUpButton, "+",
                    CheckCollisionPointRec(mousePosition,
                                           vehicleSpeedUpButton));

    DrawRectangleRounded({panelX + 18.0f, 160.0f, 400.0f, 104.0f},
                         0.08f, 5, card);
    DrawText("MODE COMPARISON", panelX + 34, 174, 13, muted);
    const traffic::ModeStatistics& smart =
        simulation.statistics(traffic::TrafficMode::Smart);
    const traffic::ModeStatistics& fixed =
        simulation.statistics(traffic::TrafficMode::Fixed);
    DrawText(TextFormat("Smart  cleared: %d   avg wait: %.1fs",
                        smart.vehiclesCleared, smart.averageWait()),
             panelX + 34, 198, 13, Color{134, 239, 172, 255});
    DrawText(TextFormat("Fixed   cleared: %d   avg wait: %.1fs",
                        fixed.vehiclesCleared, fixed.averageWait()),
             panelX + 34, 222, 13, Color{250, 204, 21, 255});
    if (simulation.comparisonActive()) {
        DrawText(TextFormat("60s comparison running: %.0fs left",
                            simulation.comparisonSecondsRemaining()),
                 panelX + 34, 244, 10, Color{147, 197, 253, 255});
    }

    DrawText("EMERGENCY ROUTING", panelX + 24, 284, 14, text);
    if (simulation.emergencyActive()) {
        DrawText(TextFormat("Ambulance: %s -> %s",
                            simulation.emergencyStartName().c_str(),
                            simulation.emergencyDestinationName().c_str()),
                 panelX + 24, 308, 12, Color{255, 135, 132, 255});
        DrawText("Dijkstra route highlighted; signals preempt ahead.",
                 panelX + 24, 329, 11, muted);
    } else {
        DrawText(TextFormat("Ready. Destination: %s (click a junction)",
                            simulation.graph()
                                .nodes()[static_cast<std::size_t>(
                                    simulation.emergencyDestination())]
                                .name.c_str()),
                 panelX + 24, 308, 11, muted);
    }

    DrawText(TextFormat("JUNCTION %s  |  GREEN: %s",
                        node.name.c_str(), signal.greenNeighborName().c_str()),
             panelX + 24, 364, 13, text);
    DrawText("Incoming lanes (cars / priority)", panelX + 24, 389, 11, muted);

    int lineY = 410;
    for (const traffic::LaneState& lane : signal.lanes()) {
        const std::string laneText =
            lane.name + "  " + std::to_string(lane.liveVehicleCount()) +
            " cars  |  " +
            std::to_string(static_cast<int>(lane.priorityScore()));
        const traffic::TrafficLightPhase phase =
            signal.phaseForNeighbor(lane.fromNeighbor);
        DrawText(laneText.c_str(), panelX + 30, lineY, 11,
                 phase == traffic::TrafficLightPhase::Green
                     ? Color{134, 239, 172, 255}
                     : phase == traffic::TrafficLightPhase::Yellow
                           ? Color{250, 204, 21, 255}
                           : text);
        lineY += 19;
    }

    DrawText("MAX-HEAP ORDER (highest first)", panelX + 24, 510, 11, muted);
    lineY = 531;
    const std::vector<traffic::LanePriority> heap = signal.heapOrder();
    for (std::size_t i = 0; i < heap.size() && i < 4; ++i) {
        const std::string item =
            std::to_string(i + 1) + ". " + heap[i].laneName + "  " +
            std::to_string(static_cast<int>(heap[i].score));
        DrawText(item.c_str(), panelX + 30, lineY, 11, text);
        lineY += 18;
    }

    DrawText("CONTROLS", panelX + 24, 620, 13, text);
    DrawText("SPACE  Pause / resume", panelX + 30, 644, 11, text);
    DrawText("T  Toggle smart / fixed mode", panelX + 30, 662, 11, text);
    DrawText("E  Send ambulance to selected junction",
             panelX + 30, 680, 11, text);
    DrawText("+ / -  Change vehicle spawn rate",
             panelX + 30, 698, 11, text);
    DrawText("R  Reset simulation", panelX + 30, 716, 11, text);
    DrawText("C  Run both modes for 60 seconds",
             panelX + 30, 734, 11, text);
    DrawText("Click a junction to inspect / set destination",
             panelX + 30, 752, 10, muted);
    drawLegend(panelX + 24, 782);
    DrawText("Queue O(1) | Heap O(log n) | Dijkstra O((V+E)log V)",
             panelX + 24, 821, 9, muted);
}

}  // namespace

int main() {
    InitWindow(screenWidth, screenHeight, "Smart Traffic Management System");
    SetTargetFPS(60);

    traffic::Simulation simulation;

    while (!WindowShouldClose()) {
        const float deltaTime = GetFrameTime();

        if (IsKeyPressed(KEY_SPACE)) {
            simulation.togglePaused();
        }
        if (IsKeyPressed(KEY_T)) {
            simulation.toggleMode();
        }
        if (IsKeyPressed(KEY_E)) {
            simulation.spawnEmergency();
        }
        if (IsKeyPressed(KEY_R)) {
            simulation.reset();
        }
        if (IsKeyPressed(KEY_C)) {
            simulation.startComparison();
        }
        if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) {
            simulation.changeSpawnRate(traffic::config::spawnRateStep);
        }
        if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) {
            simulation.changeSpawnRate(-traffic::config::spawnRateStep);
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            const Vector2 mouse = GetMousePosition();
            if (CheckCollisionPointRec(mouse, vehicleSpeedDownButton)) {
                simulation.changeVehicleSpeed(
                    -traffic::config::vehicleSpeedMultiplierStep);
            } else if (CheckCollisionPointRec(mouse, vehicleSpeedUpButton)) {
                simulation.changeVehicleSpeed(
                    traffic::config::vehicleSpeedMultiplierStep);
            } else {
                simulation.selectJunction({mouse.x, mouse.y});
            }
        }

        simulation.update(deltaTime);

        BeginDrawing();
        drawCityGround();
        DrawText("3 x 3 CITY GRID  |  click a junction to inspect",
                 28, 20, 14, Color{54, 73, 62, 255});
        drawRoads(simulation);
        drawJunctions(simulation);
        drawVehicles(simulation);
        drawJunctionLabels(simulation);
        drawEmergencyBanner(simulation);
        DrawRectangle(0, screenHeight - 24, mapWidth, 24,
                      Color{34, 35, 36, 245});
        DrawText("SMART CITY TRAFFIC MANAGEMENT  |  RAYLIB DESKTOP SIMULATION",
                 15, screenHeight - 18, 11, Color{224, 225, 222, 255});
        drawDashboard(simulation, GetFPS());
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
