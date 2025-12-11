/*
 * Top K Frequent Elements
 * 
 * Problem: Find k most frequent elements in array
 * 
 * Approach: Hash map + min heap
 * Time Complexity: O(n log k)
 * Space Complexity: O(n)
 */

#include <iostream>
#include <vector>
#include <unordered_map>
#include <queue>

using namespace std;

vector<int> topKFrequent(vector<int>& nums, int k) {
    // Count frequencies
    unordered_map<int, int> freq;
    for (int num : nums) {
        freq[num]++;
    }
    
    // Min heap of size k
    auto cmp = [](const pair<int, int>& a, const pair<int, int>& b) {
        return a.second > b.second;
    };
    priority_queue<pair<int, int>, vector<pair<int, int>>, decltype(cmp)> pq(cmp);
    
    for (const auto& [num, count] : freq) {
        pq.push({num, count});
        if (pq.size() > k) {
            pq.pop();
        }
    }
    
    // Extract results
    vector<int> result;
    while (!pq.empty()) {
        result.push_back(pq.top().first);
        pq.pop();
    }
    
    return result;
}

int main() {
    vector<int> nums = {1, 1, 1, 2, 2, 3, 4, 4, 4, 4};
    int k = 2;
    
    auto result = topKFrequent(nums, k);
    
    cout << "Array: [";
    for (int i = 0; i < nums.size(); ++i) {
        cout << nums[i];
        if (i < nums.size() - 1) cout << ", ";
    }
    cout << "]" << endl;
    
    cout << "Top " << k << " frequent elements: [";
    for (int i = 0; i < result.size(); ++i) {
        cout << result[i];
        if (i < result.size() - 1) cout << ", ";
    }
    cout << "]" << endl;
    
    return 0;
}
