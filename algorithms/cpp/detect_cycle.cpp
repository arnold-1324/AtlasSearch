/*
 * Detect Cycle in Graph
 * 
 * Problem: Detect if a directed graph contains a cycle
 * 
 * Approach: DFS with color marking (white, gray, black)
 * Time Complexity: O(V + E)
 * Space Complexity: O(V)
 */

#include <iostream>
#include <vector>

using namespace std;

enum Color { WHITE, GRAY, BLACK };

bool hasCycleDFS(int node, vector<vector<int>>& graph, vector<Color>& colors) {
    colors[node] = GRAY;
    
    for (int neighbor : graph[node]) {
        if (colors[neighbor] == GRAY) {
            return true; // Back edge found - cycle exists
        }
        
        if (colors[neighbor] == WHITE) {
            if (hasCycleDFS(neighbor, graph, colors)) {
                return true;
            }
        }
    }
    
    colors[node] = BLACK;
    return false;
}

bool hasCycle(int n, vector<vector<int>>& graph) {
    vector<Color> colors(n, WHITE);
    
    for (int i = 0; i < n; ++i) {
        if (colors[i] == WHITE) {
            if (hasCycleDFS(i, graph, colors)) {
                return true;
            }
        }
    }
    
    return false;
}

int main() {
    int n = 4;
    vector<vector<int>> graph(n);
    
    // Add edges (directed graph)
    graph[0] = {1};
    graph[1] = {2};
    graph[2] = {3};
    graph[3] = {1}; // Creates cycle: 1 -> 2 -> 3 -> 1
    
    bool has_cycle = hasCycle(n, graph);
    
    cout << "Graph has " << n << " vertices" << endl;
    cout << "Edges:" << endl;
    for (int i = 0; i < n; ++i) {
        for (int j : graph[i]) {
            cout << "  " << i << " -> " << j << endl;
        }
    }
    
    cout << "\nCycle detected? " << (has_cycle ? "Yes" : "No") << endl;
    
    return 0;
}
