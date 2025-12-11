/*
 * Binary Tree Traversals
 * 
 * Problem: Implement inorder, preorder, and postorder traversals
 * 
 * Time Complexity: O(n) for each traversal
 * Space Complexity: O(h) where h is height (recursion stack)
 */

#include <iostream>
#include <vector>

using namespace std;

struct TreeNode {
    int val;
    TreeNode* left;
    TreeNode* right;
    TreeNode(int x) : val(x), left(nullptr), right(nullptr) {}
};

void inorder(TreeNode* root, vector<int>& result) {
    if (!root) return;
    inorder(root->left, result);
    result.push_back(root->val);
    inorder(root->right, result);
}

void preorder(TreeNode* root, vector<int>& result) {
    if (!root) return;
    result.push_back(root->val);
    preorder(root->left, result);
    preorder(root->right, result);
}

void postorder(TreeNode* root, vector<int>& result) {
    if (!root) return;
    postorder(root->left, result);
    postorder(root->right, result);
    result.push_back(root->val);
}

void printVector(const string& name, const vector<int>& v) {
    cout << name << ": [";
    for (int i = 0; i < v.size(); ++i) {
        cout << v[i];
        if (i < v.size() - 1) cout << ", ";
    }
    cout << "]" << endl;
}

int main() {
    // Build tree:     1
    //                / \
    //               2   3
    //              / \
    //             4   5
    
    TreeNode* root = new TreeNode(1);
    root->left = new TreeNode(2);
    root->right = new TreeNode(3);
    root->left->left = new TreeNode(4);
    root->left->right = new TreeNode(5);
    
    vector<int> inorder_result, preorder_result, postorder_result;
    
    inorder(root, inorder_result);
    preorder(root, preorder_result);
    postorder(root, postorder_result);
    
    printVector("Inorder", inorder_result);
    printVector("Preorder", preorder_result);
    printVector("Postorder", postorder_result);
    
    // Cleanup
    delete root->left->left;
    delete root->left->right;
    delete root->left;
    delete root->right;
    delete root;
    
    return 0;
}
