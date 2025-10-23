#include "Header_functions.h"
#include <iostream>
#include <vector>
using namespace std;

int main() {
    try{
    unsigned int n;
    if (!(cin >> n)) {
        throw runtime_error("Invalid input");
    }
    if (n == 0) {
        throw invalid_argument("n must be > 0");
    }

    auto facts = Tasks::first_factorials(n);
    for (size_t i = 0; i < facts.size(); ++i) {
        if (i) cout << ' ';
        cout << facts[i];
    }
    cout << '\n';
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
