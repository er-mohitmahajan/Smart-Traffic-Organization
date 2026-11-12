#include "TrafficSignal.h"

#include "TrafficConfig.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <utility>

namespace traffic {

bool LanePriorityHigher::operator()(const LanePriority& left,
                                    const LanePriority& right) const {
    if (left.score != right.score) {
        return left.score > right.score;
    }
    return left.fromNeighbor < right.fromNeighbor;
}

double LaneState::priorityScore() const {
    // Queue + half-weighted detected traffic + waiting-time aging.
    return static_cast<double>(waitingVehicles.size()) +
           static_cast<double>(approachingVehicles) *
               config::approachTrafficWeight +
           static_cast<double>(waitingAge) * config::waitingTimeWeight;
}

LaneState* TrafficSignal::findLane(int fromNeighbor) {
    const auto found = std::find_if(
        lanes_.begin(), lanes_.end(),
        [fromNeighbor](const LaneState& lane) {
            return lane.fromNeighbor == fromNeighbor;
        });
    return found == lanes_.end() ? nullptr : &*found;
}

const LaneState* TrafficSignal::findLane(int fromNeighbor) const {
    const auto found = std::find_if(
        lanes_.begin(), lanes_.end(),
        [fromNeighbor](const LaneState& lane) {
            return lane.fromNeighbor == fromNeighbor;
        });
    return found == lanes_.end() ? nullptr : &*found;
}

bool TrafficSignal::sameAxis(int firstNeighbor, int secondNeighbor) const {
    const LaneState* first = findLane(firstNeighbor);
    const LaneState* second = findLane(secondNeighbor);
    return first != nullptr && second != nullptr && first->axis == second->axis;
}

void TrafficSignal::initialize(const Graph& graph, int junction) {
    lanes_.clear();
    priorityHeap_.clear();
    greenNeighbor_ = -1;
    emergencyNeighbor_ = -1;
    pendingNeighbor_ = -1;
    greenElapsed_ = 0.0f;
    yellowElapsed_ = 0.0f;
    priorityRefreshElapsed_ = config::heapRefreshSeconds;
    dischargeElapsed_ = config::vehicleHeadwaySeconds;
    fixedLaneIndex_ = 0;
    lastServedLaneIndex_ = 0;

    if (junction < 0 || static_cast<std::size_t>(junction) >= graph.size()) {
        return;
    }

    const Node& node = graph.nodes()[static_cast<std::size_t>(junction)];
    lanes_.reserve(node.edges.size());
    for (const Edge& edge : node.edges) {
        const Node& neighbor = graph.nodes()[static_cast<std::size_t>(edge.to)];
        LaneState lane;
        lane.fromNeighbor = edge.to;
        lane.name = neighbor.name;
        lane.axis = std::abs(node.position.x - neighbor.position.x) <
                            std::abs(node.position.y - neighbor.position.y)
                        ? RoadAxis::NorthSouth
                        : RoadAxis::EastWest;
        lanes_.push_back(std::move(lane));
    }
    if (!lanes_.empty()) {
        greenNeighbor_ = lanes_.front().fromNeighbor;
    }
    refreshHeap();
}

void TrafficSignal::refreshHeap() {
    std::vector<LanePriority> priorities;
    priorities.reserve(lanes_.size());
    for (const LaneState& lane : lanes_) {
        priorities.push_back(
            {lane.fromNeighbor, lane.name, lane.priorityScore()});
    }
    priorityHeap_.build(priorities);
}

int TrafficSignal::bestLaneForAxis(RoadAxis axis) const {
    int bestNeighbor = -1;
    double bestScore = -1.0;
    for (const LaneState& lane : lanes_) {
        if (lane.axis == axis && lane.priorityScore() > bestScore) {
            bestNeighbor = lane.fromNeighbor;
            bestScore = lane.priorityScore();
        }
    }
    return bestNeighbor;
}

int TrafficSignal::bestOtherAxisLane() const {
    const LaneState* current = findLane(greenNeighbor_);
    if (current == nullptr) {
        return priorityHeap_.top() == nullptr
                   ? -1
                   : priorityHeap_.top()->fromNeighbor;
    }
    return bestLaneForAxis(current->axis == RoadAxis::NorthSouth
                               ? RoadAxis::EastWest
                               : RoadAxis::NorthSouth);
}

void TrafficSignal::beginTransition(int nextNeighbor) {
    if (nextNeighbor < 0 || findLane(nextNeighbor) == nullptr) {
        return;
    }
    if (sameAxis(greenNeighbor_, nextNeighbor)) {
        greenNeighbor_ = nextNeighbor;
        pendingNeighbor_ = -1;
        phase_ = TrafficLightPhase::Green;
        greenElapsed_ = 0.0f;
        return;
    }
    pendingNeighbor_ = nextNeighbor;
    phase_ = TrafficLightPhase::Yellow;
    yellowElapsed_ = 0.0f;
}

void TrafficSignal::setApproachingCount(int fromNeighbor, int count) {
    LaneState* lane = findLane(fromNeighbor);
    if (lane != nullptr) {
        lane->approachingVehicles = std::max(0, count);
    }
}

void TrafficSignal::update(float deltaTime, bool smartMode) {
    if (deltaTime <= 0.0f || lanes_.empty()) {
        return;
    }

    dischargeElapsed_ += deltaTime;
    priorityRefreshElapsed_ += deltaTime;
    for (LaneState& lane : lanes_) {
        if (lane.waitingVehicles.empty()) {
            lane.waitingAge = 0.0f;
        } else {
            lane.waitingAge += deltaTime;
        }
    }

    if (emergencyNeighbor_ >= 0) {
        if (phase_ == TrafficLightPhase::Yellow) {
            yellowElapsed_ += deltaTime;
            if (yellowElapsed_ >= config::yellowSeconds) {
                greenNeighbor_ = pendingNeighbor_;
                pendingNeighbor_ = -1;
                phase_ = TrafficLightPhase::Green;
                greenElapsed_ = 0.0f;
            }
        } else if (greenNeighbor_ != emergencyNeighbor_) {
            beginTransition(emergencyNeighbor_);
        }
        return;
    }

    if (phase_ == TrafficLightPhase::Yellow) {
        yellowElapsed_ += deltaTime;
        if (yellowElapsed_ < config::yellowSeconds) {
            return;
        }
        greenNeighbor_ = pendingNeighbor_;
        pendingNeighbor_ = -1;
        phase_ = TrafficLightPhase::Green;
        greenElapsed_ = 0.0f;
        return;
    }

    greenElapsed_ += deltaTime;
    if (priorityRefreshElapsed_ >= config::heapRefreshSeconds) {
        priorityRefreshElapsed_ =
            std::fmod(priorityRefreshElapsed_, config::heapRefreshSeconds);
        refreshHeap();
    }

    if (!smartMode) {
        if (greenElapsed_ >= config::fixedGreenSeconds) {
            const LaneState* current = findLane(greenNeighbor_);
            if (current != nullptr) {
                const RoadAxis nextAxis =
                    current->axis == RoadAxis::NorthSouth
                        ? RoadAxis::EastWest
                        : RoadAxis::NorthSouth;
                const auto currentIt = std::find_if(
                    lanes_.begin(), lanes_.end(),
                    [&](const LaneState& lane) {
                        return lane.fromNeighbor == greenNeighbor_;
                    });
                const std::size_t currentIndex =
                    static_cast<std::size_t>(
                        std::distance(lanes_.begin(), currentIt));
                for (std::size_t offset = 1; offset <= lanes_.size(); ++offset) {
                    const std::size_t index =
                        (currentIndex + offset) % lanes_.size();
                    if (lanes_[index].axis == nextAxis) {
                        fixedLaneIndex_ = index;
                        beginTransition(lanes_[index].fromNeighbor);
                        return;
                    }
                }
            }
        }
        return;
    }

    const int bestOther = bestOtherAxisLane();
    const int bestCurrent =
        findLane(greenNeighbor_) == nullptr
            ? -1
            : bestLaneForAxis(findLane(greenNeighbor_)->axis);
    const LaneState* otherLane = findLane(bestOther);
    const LaneState* currentLane = findLane(bestCurrent);
    if (bestOther < 0 || otherLane == nullptr) {
        return;
    }

    const bool maximumReached =
        greenElapsed_ >= config::smartMaximumGreenSeconds;
    const bool otherAxisHasPriority =
        currentLane == nullptr ||
        otherLane->priorityScore() > currentLane->priorityScore();
    if (greenElapsed_ >= config::smartMinimumGreenSeconds &&
        (maximumReached || otherAxisHasPriority)) {
        beginTransition(bestOther);
    }
}

bool TrafficSignal::enqueue(int fromNeighbor, int vehicleId, bool emergency) {
    LaneState* lane = findLane(fromNeighbor);
    if (lane == nullptr) {
        return false;
    }
    if (lane->waitingVehicles.empty()) {
        lane->waitingAge = 0.0f;
    }
    if (emergency) {
        lane->waitingVehicles.enqueueFront(vehicleId);
        setEmergencyOverride(fromNeighbor);
    } else {
        lane->waitingVehicles.enqueue(vehicleId);
    }
    return true;
}

bool TrafficSignal::isGreenNeighbor(int fromNeighbor) const {
    if (phase_ != TrafficLightPhase::Green) {
        return false;
    }
    if (emergencyNeighbor_ >= 0) {
        return emergencyNeighbor_ == fromNeighbor;
    }
    return sameAxis(greenNeighbor_, fromNeighbor);
}

bool TrafficSignal::popServedVehicle(std::pair<int, int>& servedVehicle) {
    if (phase_ != TrafficLightPhase::Green ||
        dischargeElapsed_ < config::vehicleHeadwaySeconds) {
        return false;
    }
    for (std::size_t offset = 1; offset <= lanes_.size(); ++offset) {
        const std::size_t index =
            (lastServedLaneIndex_ + offset) % lanes_.size();
        LaneState& lane = lanes_[index];
        if (!isGreenNeighbor(lane.fromNeighbor) ||
            lane.waitingVehicles.empty()) {
            continue;
        }
        int vehicleId = -1;
        if (!lane.waitingVehicles.dequeue(vehicleId)) {
            return false;
        }
        servedVehicle = {lane.fromNeighbor, vehicleId};
        lastServedLaneIndex_ = index;
        dischargeElapsed_ = 0.0f;
        if (lane.waitingVehicles.empty()) {
            lane.waitingAge = 0.0f;
        }
        return true;
    }
    return false;
}

void TrafficSignal::setEmergencyOverride(int fromNeighbor) {
    if (findLane(fromNeighbor) == nullptr ||
        emergencyNeighbor_ == fromNeighbor) {
        return;
    }
    emergencyNeighbor_ = fromNeighbor;
    if (sameAxis(greenNeighbor_, fromNeighbor) &&
        phase_ == TrafficLightPhase::Green) {
        greenNeighbor_ = fromNeighbor;
        greenElapsed_ = 0.0f;
        return;
    }
    beginTransition(fromNeighbor);
}

void TrafficSignal::clearEmergencyOverride(int fromNeighbor) {
    if (emergencyNeighbor_ != fromNeighbor) {
        return;
    }
    emergencyNeighbor_ = -1;
    greenElapsed_ = 0.0f;
    refreshHeap();
}

TrafficLightPhase TrafficSignal::phaseForNeighbor(int fromNeighbor) const {
    if (findLane(fromNeighbor) == nullptr) {
        return TrafficLightPhase::Red;
    }
    if (phase_ == TrafficLightPhase::Yellow) {
        return sameAxis(greenNeighbor_, fromNeighbor)
                   ? TrafficLightPhase::Yellow
                   : TrafficLightPhase::Red;
    }
    return isGreenNeighbor(fromNeighbor) ? TrafficLightPhase::Green
                                         : TrafficLightPhase::Red;
}

std::vector<LanePriority> TrafficSignal::heapOrder() const {
    std::vector<LanePriority> ordered;
    ordered.reserve(priorityHeap_.size());
    MaxHeap<LanePriority, LanePriorityHigher> copy;
    copy.build(priorityHeap_.storage());
    while (!copy.empty()) {
        LanePriority item;
        if (!copy.pop(item)) {
            break;
        }
        ordered.push_back(std::move(item));
    }
    return ordered;
}

std::string TrafficSignal::greenNeighborName() const {
    const LaneState* lane = findLane(greenNeighbor_);
    return lane == nullptr ? "none" : lane->name;
}

std::string TrafficSignal::greenAxisName() const {
    if (emergencyNeighbor_ >= 0) {
        return phase_ == TrafficLightPhase::Yellow ? "EMERGENCY YELLOW"
                                                   : "EMERGENCY GREEN";
    }
    const LaneState* lane = findLane(greenNeighbor_);
    if (lane == nullptr) {
        return "none";
    }
    const std::string axis = lane->axis == RoadAxis::NorthSouth ? "N-S" : "E-W";
    return phase_ == TrafficLightPhase::Yellow ? axis + " YELLOW"
                                               : axis + " GREEN";
}

int TrafficSignal::liveVehicleCount() const {
    int total = 0;
    for (const LaneState& lane : lanes_) {
        total += lane.liveVehicleCount();
    }
    return total;
}

bool TrafficSignal::hasLane(int fromNeighbor) const {
    return findLane(fromNeighbor) != nullptr;
}

}  // namespace traffic
