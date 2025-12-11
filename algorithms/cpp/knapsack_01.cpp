/*
 * 0/1 Knapsack Problem
 * 
 * Problem: Maximize value of items in knapsack with weight constraint
 * 
 * Approach: Dynamic programming with 2D table
 * Time Complexity: O(n * W)
 * Space Complexity: O(n * W)
 */

#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

int knapsack(vector<int>& weights, vector<int>& values, int capacity) {
    int n = weights.size();
    vector<vector<int>> dp(n + 1, vector<int>(capacity + 1, 0));
    
    for (int i = 1; i <= n; ++i) {
        for (int w = 0; w <= capacity; ++w) {
            if (weights[i-1] <= w) {
                // Can include item i
                dp[i][w] = max(
                    dp[i-1][w],  // Don't include
                    dp[i-1][w - weights[i-1]] + values[i-1]  // Include
                );
            } else {
                // Can't include item i
                dp[i][w] = dp[i-1][w];
            }
        }
    }
    
    return dp[n][capacity];
}

int main() {
    vector<int> weights = {2, 3, 4, 5};
    vector<int> values = {3, 4, 5, 6};
    int capacity = 8;
    
    int max_value = knapsack(weights, values, capacity);
    
    cout << "Items:" << endl;
    for (int i = 0; i < weights.size(); ++i) {
        cout << "  Item " << i << ": weight=" << weights[i] 
             << ", value=" << values[i] << endl;
    }
    
    cout << "\nKnapsack capacity: " << capacity << endl;
    cout << "Maximum value: " << max_value << endl;
    
    return 0;
}
