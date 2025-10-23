#pragma once
#ifndef TASKS
#define TASKS

#include <vector>
#include <climits>
#include <set>

using namespace std;

class Tasks {
    struct Node {
        int data;
        Node* Next;
        Node(int key) : data(key), Next(NULL) {}
    };
    Node* head = nullptr;


public:
    static unsigned long long fact_mod(int n);
    static vector<unsigned long long> first_factorials(unsigned int n);
    static set<int> remove_duplicates(vector<int>& v);
    void push(int a);
    void reverseList();
    Node* reverseRecursive(Node* current);
    vector<int> listToVector();
};

#endif 

