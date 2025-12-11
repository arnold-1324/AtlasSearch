/*
 * Merge Intervals
 * 
 * Problem: Merge all overlapping intervals
 * 
 * Approach: Sort by start time, then merge overlapping intervals
 * Time Complexity: O(n log n)
 * Space Complexity: O(n)
 */

#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

vector<vector<int>> merge(vector<vector<int>>& intervals) {
    if (intervals.empty()) return {};
    
    // Sort by start time
    sort(intervals.begin(), intervals.end());
    
    vector<vector<int>> merged;
    merged.push_back(intervals[0]);
    
    for (int i = 1; i < intervals.size(); ++i) {
        if (intervals[i][0] <= merged.back()[1]) {
            // Overlapping - merge
            merged.back()[1] = max(merged.back()[1], intervals[i][1]);
        } else {
            // Non-overlapping - add new interval
            merged.push_back(intervals[i]);
        }
    }
    
    return merged;
}

int main() {
    vector<vector<int>> intervals = {{1,3}, {2,6}, {8,10}, {15,18}};
    
    auto result = merge(intervals);
    
    cout << "Merged intervals:" << endl;
    for (const auto& interval : result) {
        cout << "[" << interval[0] << ", " << interval[1] << "]" << endl;
    }
    
    return 0;
}
