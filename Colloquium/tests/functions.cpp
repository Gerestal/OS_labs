#include "Header_functions.h"
using namespace std;

unsigned long long Tasks::fact_mod(int n)
{
    if (n == 0) return 1;
    return (fact_mod(n - 1) * (n % ULLONG_MAX)) % ULLONG_MAX;
}

vector<unsigned long long> Tasks::first_factorials(unsigned int n) {
    vector<unsigned long long> res;
    res.reserve(n);
    unsigned long long cur = 1;
    for (unsigned int i = 1; i <= n; ++i) {
        res.push_back(fact_mod(i));
    }
    return res;
}

set<int> Tasks::remove_duplicates(vector<int>& v) {
    set<int> result;
    for (int x : v) {
        result.insert(x);
    }
    return result;
}

void Tasks::push(int data) {

    if (head == NULL) {
        head = new Node(data);
    }
    else {
        Node* temp = head;
        while (temp->Next) {
            temp = temp->Next;
        }
        temp->Next = new Node(data);
    }
}

void Tasks::reverseList() {
    head = reverseRecursive(head);
}

Tasks::Node* Tasks::reverseRecursive(Node* current) {
       
if (current == nullptr || current->Next == nullptr) {
   return current;
}
Node* newHead = reverseRecursive(current->Next);

current->Next->Next = current;
current->Next = nullptr;

return newHead;
}

vector<int> Tasks::listToVector() {
    vector<int> result;
    Node* current = head;

    while (current != nullptr) {
        result.push_back(current->data);
        current = current->Next;
    }

    return result;
}