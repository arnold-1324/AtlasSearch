# Algorithms - C++ Solutions

This directory contains 30+ competitive programming and algorithm solutions in C++17, demonstrating strong fundamentals across various problem categories.

## 📚 Problem Categories

### Arrays & Hashing
1. **two_sum.cpp** - Two Sum using hash map | O(n) time, O(n) space
2. **kadane_max_subarray.cpp** - Maximum subarray sum (Kadane's Algorithm) | O(n) time, O(1) space
3. **merge_intervals.cpp** - Merge overlapping intervals | O(n log n) time
4. **product_except_self.cpp** - Product of array except self | O(n) time, O(1) space
5. **container_most_water.cpp** - Container with most water | O(n) time

### Strings
6. **longest_substring_no_repeat.cpp** - Longest substring without repeating characters | O(n) time
7. **kmp_pattern_matching.cpp** - KMP string matching algorithm | O(n + m) time
8. **valid_anagram.cpp** - Check if two strings are anagrams | O(n) time
9. **group_anagrams.cpp** - Group anagrams together | O(n * k log k) time
10. **palindrome_check.cpp** - Check if string is palindrome | O(n) time

### Binary Trees
11. **tree_traversals.cpp** - Inorder, preorder, postorder traversals | O(n) time
12. **tree_bfs.cpp** - Level-order traversal (BFS) | O(n) time
13. **tree_diameter.cpp** - Diameter of binary tree | O(n) time
14. **lowest_common_ancestor.cpp** - LCA in binary tree | O(n) time
15. **validate_bst.cpp** - Validate binary search tree | O(n) time

### Graphs
16. **dijkstra_shortest_path.cpp** - Dijkstra's algorithm | O((V + E) log V) time
17. **bfs_shortest_path.cpp** - BFS for unweighted shortest path | O(V + E) time
18. **union_find.cpp** - Disjoint set union with path compression | O(α(n)) amortized
19. **detect_cycle.cpp** - Detect cycle in directed/undirected graph | O(V + E) time
20. **topological_sort.cpp** - Topological sort using DFS | O(V + E) time

### Dynamic Programming
21. **longest_increasing_subsequence.cpp** - LIS with binary search | O(n log n) time
22. **knapsack_01.cpp** - 0/1 Knapsack problem | O(n * W) time
23. **coin_change.cpp** - Minimum coins for amount | O(n * amount) time
24. **edit_distance.cpp** - Levenshtein distance | O(m * n) time
25. **longest_common_subsequence.cpp** - LCS | O(m * n) time

### Advanced Data Structures
26. **trie_autocomplete.cpp** - Trie for prefix search and autocomplete | O(m) time
27. **top_k_elements.cpp** - Top K frequent elements using heap | O(n log k) time
28. **lru_cache.cpp** - LRU Cache implementation | O(1) get/put
29. **median_finder.cpp** - Find median from data stream | O(log n) insert, O(1) median
30. **sliding_window_maximum.cpp** - Sliding window maximum using deque | O(n) time

## 🚀 Compiling and Running

Each file is standalone and can be compiled individually:

```bash
# Compile single file
g++ -std=c++17 -O2 -Wall two_sum.cpp -o two_sum

# Run
./two_sum
```

Or compile all at once:

```bash
# Compile all algorithms
for file in *.cpp; do
    g++ -std=c++17 -O2 -Wall "$file" -o "${file%.cpp}"
done
```

## 📊 Complexity Summary

| Problem | Time | Space | Category |
|---------|------|-------|----------|
| Two Sum | O(n) | O(n) | Hash Map |
| Kadane's Algorithm | O(n) | O(1) | DP |
| Dijkstra | O((V+E) log V) | O(V) | Graph |
| LIS | O(n log n) | O(n) | DP + Binary Search |
| Union Find | O(α(n)) | O(n) | Disjoint Set |
| Trie Autocomplete | O(m) | O(ALPHABET * N * M) | Trie |
| KMP | O(n + m) | O(m) | String Matching |

## 🎯 Key Techniques Demonstrated

### 1. **Hash Maps & Sets**
- Two Sum, Group Anagrams
- O(1) average lookup time

### 2. **Two Pointers**
- Container with Most Water, Valid Palindrome
- Reduces O(n²) to O(n)

### 3. **Sliding Window**
- Longest Substring Without Repeating, Sliding Window Maximum
- Efficient substring/subarray problems

### 4. **Binary Search**
- LIS optimization, Search in Rotated Array
- O(log n) search time

### 5. **Dynamic Programming**
- Bottom-up and top-down approaches
- Memoization and tabulation

### 6. **Graph Algorithms**
- BFS, DFS, Dijkstra, Union-Find
- Shortest paths, cycle detection, connectivity

### 7. **Tree Algorithms**
- Traversals, LCA, Diameter
- Recursion and iterative approaches

### 8. **Greedy Algorithms**
- Merge Intervals, Jump Game
- Optimal substructure

### 9. **Advanced Data Structures**
- Trie, Heap, Deque, LRU Cache
- Specialized problem solving

## 📝 Code Style

All solutions follow these principles:

1. **Clean C++17** - Modern idioms, STL usage
2. **Well-commented** - Explanation of approach and complexity
3. **Tested** - Each file includes a `main()` with example usage
4. **Optimal** - Best known time/space complexity
5. **Readable** - Clear variable names and structure

## 🏆 Problem Sources

Problems are inspired by:
- LeetCode
- Codeforces
- HackerRank
- Google/Meta interview questions
- Classic CS algorithms

## 📖 Learning Path

**Beginner:**
1. Two Sum
2. Valid Palindrome
3. Tree Traversals
4. BFS Shortest Path

**Intermediate:**
5. Kadane's Algorithm
6. Merge Intervals
7. Union Find
8. Longest Substring Without Repeating

**Advanced:**
9. Dijkstra's Algorithm
10. LIS with Binary Search
11. Trie Autocomplete
12. LRU Cache

## 🔗 Additional Resources

- [CP-Algorithms](https://cp-algorithms.com/) - Comprehensive algorithm reference
- [LeetCode](https://leetcode.com/) - Practice problems
- [Codeforces](https://codeforces.com/) - Competitive programming
- [GeeksforGeeks](https://www.geeksforgeeks.org/) - Tutorials and explanations

## 💡 Tips for Interviews

1. **Understand the problem** - Ask clarifying questions
2. **Think out loud** - Explain your approach
3. **Start with brute force** - Then optimize
4. **Analyze complexity** - Time and space
5. **Test edge cases** - Empty input, single element, duplicates
6. **Practice regularly** - Consistency is key

---

**Note**: This collection demonstrates proficiency in algorithms and data structures commonly tested in technical interviews at top tech companies (FAANG/MAANG).
