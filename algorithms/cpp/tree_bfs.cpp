/*
 * Binary Tree Level Order Traversal (BFS)
 * 
 * Problem: Traverse tree level by level
 * 
 * Approach: Queue-based BFS
 * Time Complexity: O(n)
 * Space Complexity: O(w) where w is max width
 */

#include <iostream>
#include <vector>
#include <queue>

using namespace std;

struct TreeNode {
    int val;
    TreeNode* left;
    TreeNode* right;
    TreeNode(int x) : val(x), left(nullptr), right(nullptr) {}
};

vector<vector<int>> levelOrder(TreeNode* root) {
    vector<vector<int>> result;
    if (!root) return result;
    
    queue<TreeNode*> q;
    q.push(root);
    
    while (!q.empty()) {
        int level_size = q.size();
        vector<int> level;
        
        for (int i = 0; i < level_size; ++i) {
            TreeNode* node = q.front();
            q.pop();
            
            level.push_back(node->val);
            
            if (node->left) q.push(node->left);
            if (node->right) q.push(node->right);
        }
        
        result.push_back(level);
    }
    
    return result;
}

int main() {
    // Build tree:     3
    //                / \
    //               9  20
    //                 /  \
    //                15   7
    
    TreeNode* root = new TreeNode(3);
    root->left = new TreeNode(9);
    root->right = new TreeNode(20);
    root->right->left = new TreeNode(15);
    root->right->right = new TreeNode(7);
    
    auto result = levelOrder(root);
    
    cout << "Level order traversal:" << endl;
    for (int i = 0; i < result.size(); ++i) {
        cout << "Level " << i << ": [";
        for (int j = 0; j < result[i].size(); ++j) {
            cout << result[i][j];
            if (j < result[i].size() - 1) cout << ", ";
        }
        cout << "]" << endl;
    }
    
    // Cleanup
    delete root->right->right;
    delete root->right->left;
    delete root->right;
    delete root->left;
    delete root;
    
    return 0;
}
