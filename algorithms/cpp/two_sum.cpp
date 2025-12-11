/*
 * Two Sum
 * 
 * Problem: Given an array of integers and a target, return indices of two numbers
 * that add up to the target.
 * 
 * Approach: Use hash map to store complement values
 * Time Complexity: O(n)
 * Space Complexity: O(n)
 */

#include <iostream>
#include <vector>
#include <unordered_map>

using namespace std;

vector<int> twoSum(vector<int>& nums, int target) {
    unordered_map<int, int> seen;
    
    for (int i = 0; i < nums.size(); ++i) {
        int complement = target - nums[i];
        
        if (seen.find(complement) != seen.end()) {
            return {seen[complement], i};
        }
        
        seen[nums[i]] = i;
    }
    
    return {};
}

int main() {
    vector<int> nums = {2, 7, 11, 15};
    int target = 9;
    
    vector<int> result = twoSum(nums, target);
    
    cout << "Two Sum indices: [" << result[0] << ", " << result[1] << "]" << endl;
    cout << "Values: " << nums[result[0]] << " + " << nums[result[1]] 
         << " = " << target << endl;
    
    return 0;
}
