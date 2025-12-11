/*
 * Trie (Prefix Tree) for Autocomplete
 * 
 * Problem: Implement a trie for efficient prefix-based word search
 * 
 * Time Complexity: O(m) for insert/search where m is word length
 * Space Complexity: O(ALPHABET_SIZE * N * M) worst case
 */

#include <iostream>
#include <string>
#include <vector>
#include <memory>

using namespace std;

class TrieNode {
public:
    bool isEndOfWord;
    unique_ptr<TrieNode> children[26];
    
    TrieNode() : isEndOfWord(false) {
        for (int i = 0; i < 26; ++i) {
            children[i] = nullptr;
        }
    }
};

class Trie {
private:
    unique_ptr<TrieNode> root;
    
    void collectWords(TrieNode* node, string prefix, vector<string>& results) {
        if (!node) return;
        
        if (node->isEndOfWord) {
            results.push_back(prefix);
        }
        
        for (int i = 0; i < 26; ++i) {
            if (node->children[i]) {
                collectWords(node->children[i].get(), prefix + char('a' + i), results);
            }
        }
    }

public:
    Trie() : root(make_unique<TrieNode>()) {}
    
    void insert(const string& word) {
        TrieNode* curr = root.get();
        
        for (char c : word) {
            int idx = c - 'a';
            if (!curr->children[idx]) {
                curr->children[idx] = make_unique<TrieNode>();
            }
            curr = curr->children[idx].get();
        }
        
        curr->isEndOfWord = true;
    }
    
    bool search(const string& word) {
        TrieNode* curr = root.get();
        
        for (char c : word) {
            int idx = c - 'a';
            if (!curr->children[idx]) {
                return false;
            }
            curr = curr->children[idx].get();
        }
        
        return curr->isEndOfWord;
    }
    
    vector<string> autocomplete(const string& prefix) {
        vector<string> results;
        TrieNode* curr = root.get();
        
        // Navigate to prefix
        for (char c : prefix) {
            int idx = c - 'a';
            if (!curr->children[idx]) {
                return results; // No words with this prefix
            }
            curr = curr->children[idx].get();
        }
        
        // Collect all words with this prefix
        collectWords(curr, prefix, results);
        return results;
    }
};

int main() {
    Trie trie;
    
    vector<string> words = {"apple", "app", "application", "apply", "banana", "band", "bandana"};
    
    cout << "Inserting words:" << endl;
    for (const string& word : words) {
        trie.insert(word);
        cout << "  " << word << endl;
    }
    
    cout << "\nSearch tests:" << endl;
    cout << "  'apple' found? " << (trie.search("apple") ? "Yes" : "No") << endl;
    cout << "  'app' found? " << (trie.search("app") ? "Yes" : "No") << endl;
    cout << "  'appl' found? " << (trie.search("appl") ? "Yes" : "No") << endl;
    
    cout << "\nAutocomplete 'app':" << endl;
    auto suggestions = trie.autocomplete("app");
    for (const string& word : suggestions) {
        cout << "  " << word << endl;
    }
    
    cout << "\nAutocomplete 'ban':" << endl;
    suggestions = trie.autocomplete("ban");
    for (const string& word : suggestions) {
        cout << "  " << word << endl;
    }
    
    return 0;
}
