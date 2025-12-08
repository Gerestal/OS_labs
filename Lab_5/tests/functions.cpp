#include "Header.h"
#include <windows.h>
#undef max
#include <conio.h>
#include <iostream>
#include <string>
#include <fstream>
#include <exception>
#include <limits>
#include <vector>


using namespace std;


void PrintFile(const string& fileName)
{
    employee emp;
    ifstream file(fileName, ios::binary);
    if (!file)
    {
        cerr << "Couldn't open the file: " << fileName << endl;
        return;
    }

    int index = 1;
    while (file.read(reinterpret_cast<char*>(&emp), sizeof(employee))) {
        cout << "\nEmployee " << index++ << endl;
        cout << "ID: " << emp.num << endl;
        cout << "Name: " << emp.name << endl;
        cout << "Hours: " << emp.hours << endl;
    }
    file.close();

    cout << endl;
}

employee FindEmployeeInFile(int employeeNum, const string& filename) {
    ifstream file(filename, ios::binary);
    if (!file) {
        return employee{ 0, "", 0.0 };
    }

    employee emp;
    while (file.read(reinterpret_cast<char*>(&emp), sizeof(employee))) {
        if (emp.num == employeeNum) {
            return emp;
        }
    }

    return employee{ 0, "", 0.0 };
}

bool ModifyEmployeeInFile(int employeeNum, const employee& newEmp, const string& filename) {
    fstream file(filename, ios::binary | ios::in | ios::out);
    if (!file) {
        return false;
    }

    employee emp;
    streampos pos = 0;
    while (file.read(reinterpret_cast<char*>(&emp), sizeof(employee))) {
        if (emp.num == employeeNum) {
            file.seekp(pos);
            file.write(reinterpret_cast<const char*>(&newEmp), sizeof(employee));
            return true;
        }
        pos = file.tellg();
    }

    return false;
}


