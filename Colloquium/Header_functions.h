#pragma once
#ifndef TASKS
#define TASKS

#include <vector>
#include <climits>

using namespace std;

class Tasks {
public:
   
    static unsigned long long fact_mod(int n);
    static vector<unsigned long long> first_factorials(unsigned int n);
};

#endif 

