#include "Header_functions.h"
#include <iostream>
#include <vector>
#include <set>
#include <stdexcept>
#include <string>
#include <sstream>
using namespace std;

int main() {
    try{
    int n;
    cout << "Enter the number:" << endl;
    if (!(cin >> n)) {
        throw runtime_error("Invalid input");
    }
    if (n <= 0) {
        throw invalid_argument("The number must be > 0");
    }

    auto facts = Tasks::first_factorials(n);
    for (size_t i = 0; i < facts.size(); ++i) {
        if (i) cout << ' ';
        cout << facts[i];
    }
    cout << endl;

    cout << "Enter the size of the array:" << endl;
    int size;
    if (!(cin >> size)) {
        throw runtime_error("Invalid input for size"); 
    }
    if (size <= 0) { 
        throw invalid_argument("Size must be > 0");
    }

    vector<int> v;
    v.reserve(size);
    cout << "Enter "<< size << " elements:" << endl;
    for (int i = 0; i < size; ++i) { 
        int a; 
        if (!(cin >> a)) { 
            throw runtime_error("Invalid input for array element");
        }
        v.push_back(a); 
    }

    set <int> result = Tasks::remove_duplicates(v);
    cout << "Without duplicates:" << endl;
    for (int x : result) {
        cout << x << " ";
    }
    cout << endl;

    cout << "Enter the size of the array:" << endl;
    int size2;
    if (!(cin >> size2)) {
        throw runtime_error("Invalid input for size");
    }
    if (size2 <= 0) {
        throw invalid_argument("Size must be > 0");
    }
    Tasks list;
    cout << "Enter " << size << " elements:" << endl;
    for (int i = 0; i < size; ++i) {
        int a;
        if (!(cin >> a)) {
            throw runtime_error("Invalid input for array element");
        }
        list.push(a);
    }
    list.reverseList();
    vector<int> result2 = list.listToVector();
    cout << "The inverted list:" << endl;
    for (int x : result2) {
        cout << x << " ";
    }
    cout << endl;


    return 0;
    }
    catch (const invalid_argument& e) {
    cerr << "Invalid argument: " << e.what() << '\n';
    return 2;
    }
    catch (const runtime_error& e) {
    cerr << "Runtime error: " << e.what() << '\n';
    return 3;
    }
    catch (const exception& e) {
    cerr << "Error: " << e.what() << '\n';
    return 4;
    }
    catch (...) {
    cerr << "Unknown error\n";
    return 5;
    }
}
