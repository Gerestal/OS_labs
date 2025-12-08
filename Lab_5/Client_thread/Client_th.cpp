#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <windows.h>
#undef max
#include <stdexcept>
#include <thread>
#include <chrono>

using namespace std;

struct employee {
    int num;
    char name[10];
    double hours;
};

void DisplayEmployee(const employee& emp) {
    if (emp.num == 0) {
        cout << "The employee was not found!" << endl;
        return;
    }

    cout << "\nData about the employee:" << endl;
    cout << "ID: " << emp.num << endl;
    cout << "Name: " << emp.name << endl;
    cout << "Number of working hours: " << emp.hours << endl;
}

bool ReadIntFromConsole(const string& prompt, int& value, bool allowEmpty = false) {
    while (true) {
        cout << prompt;
        string input;
        getline(cin, input);

        if (allowEmpty && input.empty()) {
            return false;
        }

        try {
            value = stoi(input);
            if (value < 0) {
                cout << "Error: Value can't be negative. Please try again." << endl;
                continue;
            }
            return true;
        }
        catch (...) {
            cout << "Error: Please enter a valid integer. Try again." << endl;
        }
    }
}

bool ReadDoubleFromConsole(const string& prompt, double& value) {
    while (true) {
        cout << prompt;
        string input;
        getline(cin, input);

        try {
            value = stod(input);
            if (value < 0) {
                cout << "Error: Value can't be negative. Please try again." << endl;
                continue;
            }
            return true;
        }
        catch (...) {
            cout << "Error: Please enter a valid number. Try again." << endl;
        }
    }
}

string ReadNameFromConsole(const string& prompt) {
    string name;
    while (true) {
        cout << prompt;
        getline(cin, name);

        
        size_t start = name.find_first_not_of(" \t");
        size_t end = name.find_last_not_of(" \t");

        if (start == string::npos) {
            cout << "Error: Name cannot be empty. Please try again." << endl;
            continue;
        }

        name = name.substr(start, end - start + 1);

        if (name.length() > 9) {
            cout << "Error: Name is too long. Maximum length is 9 characters. Try again." << endl;
            continue;
        }

        return name;
    }
}

bool SendRequest(HANDLE hPipe, const string& request) {
    DWORD bytesWritten;
    if (!WriteFile(hPipe, request.c_str(), static_cast<DWORD>(request.length() + 1), &bytesWritten, NULL)) {
        cerr << "Error sending request to server: " << GetLastError() << endl;
        return false;
    }
    return true;
}

bool ReceiveEmployee(HANDLE hPipe, employee& emp) {
    DWORD bytesRead;
    if (!ReadFile(hPipe, &emp, sizeof(employee), &bytesRead, NULL)) {
        cerr << "Error receiving data from server: " << GetLastError() << endl;
        return false;
    }
    return true;
}

void HandleModification(HANDLE hPipe) {
    int employeeNum;
    if (!ReadIntFromConsole("\nEmployee's id number: ", employeeNum)) {
        return;
    }

    
    string request = "1:" + to_string(employeeNum);
    if (!SendRequest(hPipe, request)) {
        return;
    }

    
    employee currentEmp;
    if (!ReceiveEmployee(hPipe, currentEmp)) {
        return;
    }

    DisplayEmployee(currentEmp);

    if (currentEmp.num == 0) {
        cout << "Cannot modify - employee not found!" << endl;
        return;
    }

    
    employee newEmp = currentEmp;
    string name = ReadNameFromConsole("\nEnter new name (max 9 chars): ");

    double hours;
    ReadDoubleFromConsole("Enter new number of working hours: ", hours);

    strncpy_s(newEmp.name, sizeof(newEmp.name), name.c_str(), _TRUNCATE);
    newEmp.hours = hours;

    
    while (true) {
        cout << "\nActions:" << endl;
        cout << "1 - Send modified record to server" << endl;
        cout << "2 - Complete access to this record (without saving)" << endl;
        cout << "Select action: ";

        string choice;
        getline(cin, choice);

        if (choice == "1") {
            
            DWORD bytesWritten;
            if (!WriteFile(hPipe, &newEmp, sizeof(employee), &bytesWritten, NULL)) {
                cerr << "Error sending modified data: " << GetLastError() << endl;
                break;
            }

            
            char response[100];
            DWORD bytesRead;
            if (ReadFile(hPipe, response, sizeof(response) - 1, &bytesRead, NULL)) {
                response[bytesRead] = '\0';
                cout << "Modification result: " << response << endl;
            }

            
            this_thread::sleep_for(chrono::milliseconds(100));
            cout << "Modification completed. Returning to main menu." << endl;
            break;

        }
        else if (choice == "2") {
            
            string completeCmd = "COMPLETE:" + to_string(employeeNum);
            SendRequest(hPipe, completeCmd);
            cout << "Access to record " << employeeNum << " completed without saving." << endl;
            cout << "Returning to main menu." << endl;
            break;

        }
        else {
            cout << "Incorrect action. Try again." << endl;
        }
    }
}

void HandleReading(HANDLE hPipe) {
    int employeeNum;
    if (!ReadIntFromConsole("\nEmployee's id number: ", employeeNum)) {
        return;
    }

    
    string request = "2:" + to_string(employeeNum);
    if (!SendRequest(hPipe, request)) {
        return;
    }

    
    employee emp;
    if (!ReceiveEmployee(hPipe, emp)) {
        return;
    }

    DisplayEmployee(emp);

    
    while (true) {
        cout << "\nActions:" << endl;
        cout << "1 - Complete access to this record" << endl;
        cout << "Select action: ";

        string choice;
        getline(cin, choice);

        if (choice == "1") {
            string completeCmd = "COMPLETE_READ:" + to_string(employeeNum);
            SendRequest(hPipe, completeCmd);
            cout << "Access to record " << employeeNum << " completed." << endl;
            cout << "Returning to main menu." << endl;
            break;

        }
        else {
            cout << "Incorrect action. Try again." << endl;
        }
    }
}

int main() {
    try {
       
        HANDLE hPipe = CreateFileW(
            L"\\\\.\\pipe\\NamedPipe",
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);

        if (hPipe == INVALID_HANDLE_VALUE) {
            DWORD error = GetLastError();
            cerr << "Cannot connect to pipe. Error code: " << error << endl;

            if (error == ERROR_FILE_NOT_FOUND) {
                cerr << "Server is not available. Make sure server is running." << endl;
            }
            else if (error == ERROR_PIPE_BUSY) {
                cerr << "Pipe is busy. Try again later." << endl;
            }

            return 1;
        }

        cout << "Connected to server successfully." << endl;

        bool running = true;

        while (running) {
            cout << "\nOperations:" << endl;
            cout << "1 - Record modification" << endl;
            cout << "2 - Reading a record" << endl;
            cout << "3 - Exit" << endl;
            cout << "Select an operation: ";

            string choice;
            getline(cin, choice);

            if (choice == "1") {
                HandleModification(hPipe);
            }
            else if (choice == "2") {
                HandleReading(hPipe);
            }
            else if (choice == "3") {
                running = false;
                cout << "\nCompletion of work. Exiting." << endl;
            }
            else {
                cout << "Incorrect input. Enter 1, 2 or 3." << endl;
            }
        }

        CloseHandle(hPipe);
        return 0;

    }
    catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
}