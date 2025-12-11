/*
 * Longest Substring Without Repeating Characters
 * 
 * Problem: Find the length of the longest substring without repeating characters
 * 
 * Approach: Sliding window with hash set
 * Time Complexity: O(n)
 * Space Complexity: O(min(n, m)) where m is charset size
 */

#include <iostream>
#include <string>
#include <unordered_set>
#include <algorithm>

using namespace std;

int lengthOfLongestSubstring(string s) {
    unordered_set<char> seen;
    int left = 0;
    int max_len = 0;
    
    for (int right = 0; right < s.length(); ++right) {
        // Shrink window until no duplicates
        while (seen.count(s[right])) {
            seen.erase(s[left]);
            left++;
        }
        
        seen.insert(s[right]);
        max_len = max(max_len, right - left + 1);
    }
    
    return max_len;
}

int main() {
    string s = "abcabcbb";
    
    int result = lengthOfLongestSubstring(s);
    
    cout << "String: " << s << endl;
    cout << "Length of longest substring without repeating: " << result << endl;
    cout << "Example: 'abc' has length 3" << endl;
    
    return 0;
}
