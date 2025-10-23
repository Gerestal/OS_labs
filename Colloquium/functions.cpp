#include "Header_functions.h"

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