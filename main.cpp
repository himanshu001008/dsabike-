#include <iostream>
#include <vector>
#include <unordered_map>
#include <queue>
#include <stack>
#include <string>
#include <algorithm>

using namespace std;

// PHASE 1: CORE BLUEPRINTS (Data Structures)

struct Bike {
    int id;
    string currentStation;
    string status; // "locked", "unlocked", "in-use", "broken"
};

struct Station {
    string name;
    int totalSlots;
    int availableBikes;
    
    // Helper to calculate empty slots
    int openSlots() const {
        return totalSlots - availableBikes;
    }
};

// For Undo Checkout feature
struct UndoAction {
    int bikeId;
    string previousStatus;
    string previousStation;
};

// For City Map Graph
struct Edge {
    int destinationIntersection;
    double safetyWeight; // Lower is safer
};

// PHASE 2 - 6: THE SYSTEM CONTROLLER

class BikeSystem {
private:
    // Feature 1 & 4: Bike Registry & Tracker (O(1) lookup)
    unordered_map<int, Bike> bikeRegistry;
    
    // Feature 3: Repair Line (FIFO)
    queue<int> repairQueue;
    
    // Feature 2: Undo Checkout (LIFO)
    stack<UndoAction> checkoutHistory;
    
    // Feature 5: Station Sorter data
    vector<Station> stations;
    
    // Feature 6: City Map (Adjacency List)
    unordered_map<int, vector<Edge>> cityMap;

public:
    // --- Feature 1: Bike Registry ---
    void addBike(int id, string station, string status) {
        bikeRegistry[id] = {id, station, status};
        cout << "Bike #" << id << " added to registry." << endl;
    }

    // Feature 4: Bike Tracker 
    void trackBike(int id) {
        if (bikeRegistry.find(id) != bikeRegistry.end()) {
            Bike b = bikeRegistry[id];
            cout << "Tracker - Bike #" << b.id << " | Station: " << b.currentStation 
                 << " | Status: " << b.status << endl;
        } else {
            cout << "Bike #" << id << " not found!" << endl;
        }
    }

    // --- Feature 2: Undo Checkout ---
    void checkoutBike(int id) {
        if (bikeRegistry.find(id) != bikeRegistry.end()) {
            if (bikeRegistry[id].status == "locked") {
                // Save state before changing
                checkoutHistory.push({id, bikeRegistry[id].status, bikeRegistry[id].currentStation});
                
                // Update state
                bikeRegistry[id].status = "in-use";
                cout << "Bike #" << id << " checked out successfully." << endl;
            } else {
                cout << "Cannot checkout Bike #" << id << ". Current status is: " << bikeRegistry[id].status << endl;
            }
        } else {
            cout << "Bike #" << id << " not found!" << endl;
        }
    }

    void undoLastCheckout() {
        if (!checkoutHistory.empty()) {
            UndoAction lastAction = checkoutHistory.top();
            checkoutHistory.pop();
            
            // Restore previous state
            bikeRegistry[lastAction.bikeId].status = lastAction.previousStatus;
            bikeRegistry[lastAction.bikeId].currentStation = lastAction.previousStation;
            
            cout << "Undo successful! Bike #" << lastAction.bikeId << " restored to " 
                 << lastAction.previousStatus << "." << endl;
        } else {
            cout << "No actions to undo." << endl;
        }
    }

    // --- Feature 3: Repair Line ---
    void reportBrokenBike(int id) {
        if (bikeRegistry.find(id) != bikeRegistry.end()) {
            bikeRegistry[id].status = "broken";
            repairQueue.push(id);
            cout << "Bike #" << id << " reported broken and added to repair queue." << endl;
        } else {
            cout << "Bike #" << id << " not found!" << endl;
        }
    }

    void fixNextBike() {
        if (!repairQueue.empty()) {
            int bikeToFix = repairQueue.front();
            repairQueue.pop();
            bikeRegistry[bikeToFix].status = "locked"; // Fixed!
            cout << "Bike #" << bikeToFix << " has been repaired and is ready for use." << endl;
        } else {
            cout << "Repair queue is empty. No bikes need fixing!" << endl;
        }
    }

    // --- Feature 5: Station Sorter ---
    void addStation(string name, int total, int available) {
        stations.push_back({name, total, available});
    }

    void sortStationsByNeed() {
        if (stations.empty()) {
            cout << "No stations registered." << endl;
            return;
        }
        
        // Sorts stations based on who has the most open slots (needs bikes the most)
        sort(stations.begin(), stations.end(), [](const Station& a, const Station& b) {
            return a.openSlots() > b.openSlots(); 
        });

        cout << "\n--- Stations Needing Bikes (Emptiest First) ---" << endl;
        for (const auto& s : stations) {
            cout << s.name << " | Open Slots: " << s.openSlots() << " (Total Capacity: " << s.totalSlots << ")" << endl;
        }
    }

    // Feature 6 & 7: City Map & Safest Route (Dijkstra)
    void addPath(int from, int to, double dangerLevel) {
        cityMap[from].push_back({to, dangerLevel});
        cityMap[to].push_back({from, dangerLevel}); // Assuming undirected paths
    }

    void findSafestRoute(int startNode, int endNode) {
        // Priority queue prioritizing lowest danger weight (Min-Heap)
        priority_queue<pair<double, int>, vector<pair<double, int>>, greater<pair<double, int>>> pq;
        unordered_map<int, double> minDanger;
        
        // Initialize all distances to infinity
        for (auto const& pair : cityMap) {
            minDanger[pair.first] = 1e9; // representing infinity
        }
        
        pq.push({0.0, startNode});
        minDanger[startNode] = 0.0;

        while (!pq.empty()) {
            int current = pq.top().second;
            double currentDanger = pq.top().first;
            pq.pop();

            if (current == endNode) {
                cout << "Safest route from Intersection " << startNode << " to " << endNode 
                     << " found with a danger rating of: " << currentDanger << endl;
                return;
            }
            for (auto& edge : cityMap[current]) {
                double newDanger = currentDanger + edge.safetyWeight;
                if (newDanger < minDanger[edge.destinationIntersection]) {
                    minDanger[edge.destinationIntersection] = newDanger;
                    pq.push({newDanger, edge.destinationIntersection});
                }
            }
        }
        cout << "No path found between these intersections." << endl;
    }

    // --- Feature 8: Truck Loader (0/1 Knapsack) ---
    void optimizeTruckLoad(int truckCapacity, vector<int> bikeWeights, vector<int> priorityValues) {
        int n = bikeWeights.size();
        vector<vector<int>> dp(n + 1, vector<int>(truckCapacity + 1, 0));

        for (int i = 1; i <= n; i++) {
            for (int w = 1; w <= truckCapacity; w++) {
                if (bikeWeights[i - 1] <= w) {
                    dp[i][w] = max(dp[i - 1][w], dp[i - 1][w - bikeWeights[i - 1]] + priorityValues[i - 1]);
                } else {
                    dp[i][w] = dp[i - 1][w];
                }
            }
        }
        cout << "\n--- TRUCK LOADER OPTIMIZATION ---" << endl;
        cout << "Maximized priority value for truck capacity " << truckCapacity 
             << " is: " << dp[n][truckCapacity] << endl;
    }
};


// MAIN FUNCTION (Interactive CLI)

int main() {
    BikeSystem limeSystem;
    int choice;

    // Adding some initial data so the system isn't empty upon launching
    limeSystem.addBike(101, "Downtown Station", "locked");
    limeSystem.addBike(102, "University Campus", "locked");
    limeSystem.addBike(103, "Main Square", "locked");
    
    limeSystem.addStation("Downtown Station", 50, 48);   // 2 open slots
    limeSystem.addStation("University Campus", 100, 10); // 90 open slots
    limeSystem.addStation("Main Square", 30, 15);        // 15 open slots

    // Adding some paths for the City Map
    limeSystem.addPath(1, 2, 5.0); // Busy road
    limeSystem.addPath(1, 3, 1.0); // Bike lane
    limeSystem.addPath(3, 2, 1.0); // Bike lane

    do {
        cout << "\n=====================================" << endl;
        cout << "     BIKE DOCKING SYSTEM MENU      " << endl;
        cout << "=====================================" << endl;
        cout << "1. Track a Bike" << endl;
        cout << "2. Checkout a Bike" << endl;
        cout << "3. Undo Last Checkout" << endl;
        cout << "4. Report a Broken Bike" << endl;
        cout << "5. Fix Next Broken Bike" << endl;
        cout << "6. View Emptiest Stations" << endl;
        cout << "7. Find Safest Route (Test 1 -> 2)" << endl;
        cout << "8. Run Truck Loader Optimization" << endl;
        cout << "0. Exit System" << endl;
        cout << "Enter your choice: ";
        
        // Input validation
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(10000, '\n');
            choice = -1;
        }

        int bikeId;
        switch (choice) {
            case 1:
                cout << "Enter Bike ID to track (e.g., 101, 102): ";
                cin >> bikeId;
                limeSystem.trackBike(bikeId);
                break;
            case 2:
                cout << "Enter Bike ID to checkout: ";
                cin >> bikeId;
                limeSystem.checkoutBike(bikeId);
                break;
            case 3:
                limeSystem.undoLastCheckout();
                break;
            case 4:
                cout << "Enter Bike ID to report broken: ";
                cin >> bikeId;
                limeSystem.reportBrokenBike(bikeId);
                break;
            case 5:
                limeSystem.fixNextBike();
                break;
            case 6:
                limeSystem.sortStationsByNeed();
                break;
            case 7:
                // Hardcoded test case for demonstration
                limeSystem.findSafestRoute(1, 2);
                break;
            case 8: {
                // Hardcoded test case for the Knapsack problem demonstration
                vector<int> weights = {1, 2, 3}; 
                vector<int> values = {10, 15, 40}; 
                limeSystem.optimizeTruckLoad(6, weights, values);
                break;
            }
            case 0:
                cout << "Shutting down system. Goodbye!" << endl;
                break;
            default:
                cout << "Invalid choice. Please try again." << endl;
        }
    } while (choice != 0);

    return 0;
}