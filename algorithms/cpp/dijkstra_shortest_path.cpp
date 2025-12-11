/*
 * Dijkstra's Shortest Path Algorithm
 * 
 * Problem: Find shortest path from source to all vertices in weighted graph
 * 
 * Approach: Priority queue (min-heap) with greedy selection
 * Time Complexity: O((V + E) log V)
 * Space Complexity: O(V)
 */

#include <iostream>
#include <vector>
#include <queue>
#include <limits>

using namespace std;

typedef pair<int, int> pii; // {distance, vertex}

vector<int> dijkstra(int n, vector<vector<pii>>& graph, int source) {
    vector<int> dist(n, numeric_limits<int>::max());
    priority_queue<pii, vector<pii>, greater<pii>> pq;
    
    dist[source] = 0;
    pq.push({0, source});
    
    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();
        
        if (d > dist[u]) continue;
        
        for (auto [v, weight] : graph[u]) {
            if (dist[u] + weight < dist[v]) {
                dist[v] = dist[u] + weight;
                pq.push({dist[v], v});
            }
        }
    }
    
    return dist;
}

int main() {
    int n = 5; // vertices 0-4
    vector<vector<pii>> graph(n);
    
    // Add edges: {neighbor, weight}
    graph[0] = {{1, 4}, {2, 1}};
    graph[1] = {{3, 1}};
    graph[2] = {{1, 2}, {3, 5}};
    graph[3] = {{4, 3}};
    
    vector<int> distances = dijkstra(n, graph, 0);
    
    cout << "Shortest distances from vertex 0:" << endl;
    for (int i = 0; i < n; ++i) {
        cout << "To vertex " << i << ": ";
        if (distances[i] == numeric_limits<int>::max()) {
            cout << "INF" << endl;
        } else {
            cout << distances[i] << endl;
        }
    }
    
    return 0;
}
