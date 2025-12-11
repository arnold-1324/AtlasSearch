/*
 * Kadane's Algorithm - Maximum Subarray Sum
 * 
 * Problem: Find the contiguous subarray with the largest sum
 * 
 * Approach: Dynamic programming - track current max and global max
 * Time Complexity: O(n)
 * Space Complexity: O(1)
 */

#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

int maxSubArray(vector<int>& nums) {
    int current_max = nums[0];
    int global_max = nums[0];
    
    for (int i = 1; i < nums.size(); ++i) {
        current_max = max(nums[i], current_max + nums[i]);
        global_max = max(global_max, current_max);
    }
    
    return global_max;
}

int main() {
    vector<int> nums = {-2, 1, -3, 4, -1, 2, 1, -5, 4};
    
    int result = maxSubArray(nums);
    
    cout << "Maximum subarray sum: " << result << endl;
    cout << "Example: [4, -1, 2, 1] has sum 6" << endl;
    
    return 0;
}
