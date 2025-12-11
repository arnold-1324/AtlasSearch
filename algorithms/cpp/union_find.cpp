/*
 * Union-Find (Disjoint Set Union)
 * 
 * Problem: Efficiently track and merge disjoint sets
 * 
 * Approach: Path compression + union by rank
 * Time Complexity: O(α(n)) amortized, where α is inverse Ackermann
 * Space Complexity: O(n)
 */

#include <iostream>
#include <vector>

using namespace std;

class UnionFind {
private:
    vector<int> parent;
    vector<int> rank;
    int components;

public:
    UnionFind(int n) : parent(n), rank(n, 0), components(n) {
        for (int i = 0; i < n; ++i) {
            parent[i] = i;
        }
    }
    
    int find(int x) {
        if (parent[x] != x) {
            parent[x] = find(parent[x]); // Path compression
        }
        return parent[x];
    }
    
    bool unite(int x, int y) {
        int px = find(x);
        int py = find(y);
        
        if (px == py) return false; // Already in same set
        
        // Union by rank
        if (rank[px] < rank[py]) {
            parent[px] = py;
        } else if (rank[px] > rank[py]) {
            parent[py] = px;
        } else {
            parent[py] = px;
            rank[px]++;
        }
        
        components--;
        return true;
    }
    
    bool connected(int x, int y) {
        return find(x) == find(y);
    }
    
    int getComponents() {
        return components;
    }
};

int main() {
    int n = 10;
    UnionFind uf(n);
    
    cout << "Initial components: " << uf.getComponents() << endl;
    
    // Unite some elements
    uf.unite(0, 1);
    uf.unite(1, 2);
    uf.unite(3, 4);
    uf.unite(5, 6);
    uf.unite(6, 7);
    
    cout << "After unions: " << uf.getComponents() << " components" << endl;
    
    cout << "0 and 2 connected? " << (uf.connected(0, 2) ? "Yes" : "No") << endl;
    cout << "0 and 3 connected? " << (uf.connected(0, 3) ? "Yes" : "No") << endl;
    cout << "5 and 7 connected? " << (uf.connected(5, 7) ? "Yes" : "No") << endl;
    
    return 0;
}
