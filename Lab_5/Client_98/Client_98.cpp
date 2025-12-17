#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <windows.h>
#undef max

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
            cerr << "Cannot connect to pipe: " << GetLastError() << endl;
            return 1;
        }

        bool running = true;

        while (running) {
            cout << "\nOperations:" << endl;
            cout << "1 - Record modification" << endl;
            cout << "2 - Reading a record" << endl;
            cout << "3 - Exit" << endl;

            cout << "Select an operation: ";

            string choice;
            if (!getline(cin, choice)) {
                
                break;
            }

            if (choice == "1") { // modification
                int num;

                while (true) {
                    cout << "\nEmployee's id number: ";
                    if (!(cin >> num)) {
                        cout << "Error: please enter a valid integer for employee id." << endl;
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        continue;
                    }
                    if (num < 0) {
                        cout << "Error: id can't be negative." << endl;
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        continue;
                    }
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    break;
                }

                
                char numbuf[32];
                _snprintf(numbuf, sizeof(numbuf), "%d", num);
                string request = string("1:") + string(numbuf);
                DWORD bytesWritten = 0;

                if (!WriteFile(hPipe, request.c_str(), (DWORD)request.length() + 1, &bytesWritten, NULL)) {
                    cerr << "Error sending request to server" << endl;
                    break;
                }

                employee currentEmp;
                DWORD bytesRead = 0;

                if (!ReadFile(hPipe, &currentEmp, sizeof(employee), &bytesRead, NULL)) {
                    cerr << "Error receiving data from server" << endl;
                    continue;
                }

                DisplayEmployee(currentEmp);

                if (currentEmp.num == 0) {
                    cout << "Cannot modify - employee not found!" << endl;
                    continue;
                }

                employee newEmp = currentEmp;
                string name;
                double hours;

                while (true) {
                    cout << "\nEnter new name (max 9 chars): ";
                    if (!getline(cin, name)) {
                        cout << "Input error. Please try again." << endl;
                        continue;
                    }
                    if (name.length() > 9) {
                        cout << "Error: Name is too long. Maximum length is 9 characters." << endl;
                        continue;
                    }
                    if (name.empty()) {
                        cout << "Error: Name cannot be empty." << endl;
                        continue;
                    }
                    break;
                }

                while (true) {
                    cout << "Enter new number of working hours: ";
                    if (!(cin >> hours)) {
                        cout << "Error: please enter a valid count of hours." << endl;
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        continue;
                    }
                    if (hours < 0) {
                        cout << "Error: Working hours can't be negative." << endl;
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        continue;
                    }
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    break;
                }

               
                memset(newEmp.name, 0, sizeof(newEmp.name));
                strncpy(newEmp.name, name.c_str(), sizeof(newEmp.name) - 1);
                newEmp.name[sizeof(newEmp.name) - 1] = '\0';
                newEmp.hours = hours;

                bool modifying = true;
                while (modifying) {
                    cout << "\nActions:" << endl;
                    cout << "1 - Send modified record to server" << endl;
                    cout << "2 - Complete access to this record (without saving)" << endl;
                    cout << "Select action: ";

                    string modChoice;
                    if (!getline(cin, modChoice)) {
                        
                        modifying = false;
                        break;
                    }

                    if (modChoice == "1") {
                        if (!WriteFile(hPipe, &newEmp, sizeof(employee), &bytesWritten, NULL)) {
                            cerr << "Error sending modified data" << endl;
                            break;
                        }

                        char response[100];
                        DWORD respRead = 0;
                        if (ReadFile(hPipe, response, sizeof(response) - 1, &respRead, NULL)) {
                            if (respRead >= sizeof(response)) respRead = sizeof(response) - 1;
                            response[respRead] = '\0';
                            cout << "Modification result: " << response << endl;
                        }
                        else {
                            cerr << "No response from server after modification" << endl;
                        }

                        modifying = false;
                        cout << "Modification completed. Returning to main menu." << endl;
                    }
                    else if (modChoice == "2") {
                        char cmdbuf[64];
                        _snprintf(cmdbuf, sizeof(cmdbuf), "COMPLETE:%d", num);
                        if (!WriteFile(hPipe, cmdbuf, (DWORD)strlen(cmdbuf) + 1, &bytesWritten, NULL)) {
                            cerr << "Error sending COMPLETE command" << endl;
                        }
                        else {
                            cout << "Access to record " << num << " completed without saving." << endl;
                        }

                        modifying = false;
                        cout << "Returning to main menu." << endl;
                    }
                    else {
                        cout << "Incorrect action. Try again." << endl;
                    }
                }
            }
            else if (choice == "2") { // reading
                int num;

                while (true) {
                    cout << "\nEmployee's id number: ";
                    if (!(cin >> num)) {
                        cout << "Error: please enter a valid integer for employee id." << endl;
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        continue;
                    }
                    if (num < 0) {
                        cout << "Error: id can't be negative." << endl;
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        continue;
                    }
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    break;
                }

                char numbuf[32];
                _snprintf(numbuf, sizeof(numbuf), "%d", num);
                string request = string("2:") + string(numbuf);
                DWORD bytesWritten = 0;

                if (!WriteFile(hPipe, request.c_str(), (DWORD)request.length() + 1, &bytesWritten, NULL)) {
                    cerr << "Error sending request to server" << endl;
                    break;
                }

                employee emp;
                DWORD bytesRead = 0;

                if (ReadFile(hPipe, &emp, sizeof(employee), &bytesRead, NULL)) {
                    DisplayEmployee(emp);
                }
                else {
                    cerr << "Error receiving data from server" << endl;
                }

                bool doing = true;
                while (doing) {
                    cout << "\nActions:" << endl;
                    cout << "1 - Complete access to this record" << endl;
                    cout << "Select action: ";

                    string readChoice;
                    if (!getline(cin, readChoice)) {
                        // EOF or error
                        doing = false;
                        break;
                    }

                    if (readChoice == "1") {
                        char cmdbuf[64];
                        _snprintf(cmdbuf, sizeof(cmdbuf), "COMPLETE_READ:%d", num);
                        DWORD bytesWritten2 = 0;
                        if (!WriteFile(hPipe, cmdbuf, (DWORD)strlen(cmdbuf) + 1, &bytesWritten2, NULL)) {
                            cerr << "Error sending COMPLETE_READ command" << endl;
                        }
                        else {
                            cout << "Access to record " << num << " completed." << endl;
                        }

                        doing = false;
                        cout << "Returning to main menu." << endl;
                    }
                    else {
                        cout << "Incorrect action. Try again." << endl;
                    }
                }
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
