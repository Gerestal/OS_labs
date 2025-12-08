#pragma once
#include <windows.h>
#undef max
#include <conio.h>
#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <map>


using namespace std;


struct employee {
    int num;
    char name[10];
    double hours;
};

struct RecordLock {
    SRWLOCK lock;
    RecordLock() {
        InitializeSRWLock(&lock);
    }
};

extern vector<HANDLE> Processes;
extern map<int, RecordLock> recordLocks;
extern mutex recordMapMutex;


void PrintFile(const string& fileName);
employee FindEmployeeInFile(int employeeNum, const string& filename);
bool ModifyEmployeeInFile(int employeeNum, const employee& newEmp, const string& filename);
RecordLock& GetRecordLock(int employeeNum);


