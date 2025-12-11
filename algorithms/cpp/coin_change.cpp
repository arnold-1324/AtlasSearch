/*
 * Coin Change - Minimum Coins
 * 
 * Problem: Find minimum number of coins to make amount
 * 
 * Approach: Dynamic programming (bottom-up)
 * Time Complexity: O(n * amount)
 * Space Complexity: O(amount)
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <limits>

using namespace std;

int coinChange(vector<int>& coins, int amount) {
    vector<int> dp(amount + 1, INT_MAX);
    dp[0] = 0;
    
    for (int i = 1; i <= amount; ++i) {
        for (int coin : coins) {
            if (coin <= i && dp[i - coin] != INT_MAX) {
                dp[i] = min(dp[i], dp[i - coin] + 1);
            }
        }
    }
    
    return dp[amount] == INT_MAX ? -1 : dp[amount];
}

int main() {
    vector<int> coins = {1, 2, 5};
    int amount = 11;
    
    int result = coinChange(coins, amount);
    
    cout << "Coins: [";
    for (int i = 0; i < coins.size(); ++i) {
        cout << coins[i];
        if (i < coins.size() - 1) cout << ", ";
    }
    cout << "]" << endl;
    
    cout << "Amount: " << amount << endl;
    
    if (result == -1) {
        cout << "Cannot make amount with given coins" << endl;
    } else {
        cout << "Minimum coins needed: " << result << endl;
        cout << "Example: 5 + 5 + 1 = 11 (3 coins)" << endl;
    }
    
    return 0;
}
