/*
 * Longest Increasing Subsequence (LIS)
 * 
 * Problem: Find the length of the longest increasing subsequence
 * 
 * Approach: Dynamic programming with binary search optimization
 * Time Complexity: O(n log n)
 * Space Complexity: O(n)
 */

#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

int lengthOfLIS(vector<int>& nums) {
    if (nums.empty()) return 0;
    
    vector<int> tails; // tails[i] = smallest tail of all increasing subsequences of length i+1
    
    for (int num : nums) {
        auto it = lower_bound(tails.begin(), tails.end(), num);
        
        if (it == tails.end()) {
            tails.push_back(num);
        } else {
            *it = num;
        }
    }
    
    return tails.size();
}

int main() {
    vector<int> nums = {10, 9, 2, 5, 3, 7, 101, 18};
    
    int result = lengthOfLIS(nums);
    
    cout << "Length of LIS: " << result << endl;
    cout << "Example LIS: [2, 3, 7, 101]" << endl;
    
    return 0;
}
