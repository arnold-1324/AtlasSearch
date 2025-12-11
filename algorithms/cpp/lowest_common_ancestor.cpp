/*
 * Lowest Common Ancestor (LCA) in Binary Tree
 * 
 * Problem: Find the lowest common ancestor of two nodes
 * 
 * Approach: Recursive DFS
 * Time Complexity: O(n)
 * Space Complexity: O(h) where h is height
 */

#include <iostream>

using namespace std;

struct TreeNode {
    int val;
    TreeNode* left;
    TreeNode* right;
    TreeNode(int x) : val(x), left(nullptr), right(nullptr) {}
};

TreeNode* lowestCommonAncestor(TreeNode* root, TreeNode* p, TreeNode* q) {
    if (!root || root == p || root == q) {
        return root;
    }
    
    TreeNode* left = lowestCommonAncestor(root->left, p, q);
    TreeNode* right = lowestCommonAncestor(root->right, p, q);
    
    if (left && right) {
        return root; // Both found in different subtrees
    }
    
    return left ? left : right; // Return non-null child
}

int main() {
    // Build tree:     3
    //                / \
    //               5   1
    //              / \ / \
    //             6  2 0  8
    //               / \
    //              7   4
    
    TreeNode* root = new TreeNode(3);
    TreeNode* node5 = new TreeNode(5);
    TreeNode* node1 = new TreeNode(1);
    TreeNode* node6 = new TreeNode(6);
    TreeNode* node2 = new TreeNode(2);
    TreeNode* node0 = new TreeNode(0);
    TreeNode* node8 = new TreeNode(8);
    TreeNode* node7 = new TreeNode(7);
    TreeNode* node4 = new TreeNode(4);
    
    root->left = node5;
    root->right = node1;
    node5->left = node6;
    node5->right = node2;
    node1->left = node0;
    node1->right = node8;
    node2->left = node7;
    node2->right = node4;
    
    TreeNode* lca1 = lowestCommonAncestor(root, node5, node1);
    cout << "LCA of 5 and 1: " << lca1->val << endl;
    
    TreeNode* lca2 = lowestCommonAncestor(root, node5, node4);
    cout << "LCA of 5 and 4: " << lca2->val << endl;
    
    // Cleanup
    delete node7;
    delete node4;
    delete node2;
    delete node6;
    delete node0;
    delete node8;
    delete node5;
    delete node1;
    delete root;
    
    return 0;
}
