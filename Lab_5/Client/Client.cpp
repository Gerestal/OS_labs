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
            getline(cin, choice);

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
                        continue;
                    }
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    break;
                }

               
                string request = "1:" + to_string(num);
                DWORD bytesWritten;

                if (!WriteFile(hPipe, request.c_str(), request.length() + 1, &bytesWritten, NULL)) {
                    cerr << "Error sending request to server" << endl;
                    break;
                }

                employee currentEmp;
                DWORD bytesRead;

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
                        continue;
                    }
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    break;
                }

                strncpy_s(newEmp.name, sizeof(newEmp.name), name.c_str(), 9);
                newEmp.name[9] = '\0';
                newEmp.hours = hours;

      
                bool modifying = true;
                while (modifying) {
                    cout << "\nActions:" << endl;
                    cout << "1 - Send modified record to server" << endl;
                    cout << "2 - Complete access to this record (without saving)" << endl;
                    cout << "Select action: ";

                    string modChoice;
                    getline(cin, modChoice);

                    if (modChoice == "1") {
                        if (!WriteFile(hPipe, &newEmp, sizeof(employee), &bytesWritten, NULL)) {
                            cerr << "Error sending modified data" << endl;
                            break;
                        }

                        char response[100];
                        if (ReadFile(hPipe, response, sizeof(response) - 1, &bytesRead, NULL)) {
                            response[bytesRead] = '\0';
                            cout << "Modification result: " << response << endl;
                        }

                        modifying = false;
                        cout << "Modification completed. Returning to main menu." << endl;

                    }
                    else if (modChoice == "2") {
                       
                        string completeCmd = "COMPLETE:" + to_string(num);
                        WriteFile(hPipe, completeCmd.c_str(), completeCmd.length() + 1, &bytesWritten, NULL);

                            cout << "Access to record " << num << " completed without saving." << endl;

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
                        continue;
                    }
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    break;
                }

                
                string request = "2:" + to_string(num);
                DWORD bytesWritten;

                if (!WriteFile(hPipe, request.c_str(), request.length() + 1, &bytesWritten, NULL)) {
                    cerr << "Error sending request to server" << endl;
                    break;
                }

                
                employee emp;
                DWORD bytesRead;

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
                    getline(cin, readChoice);

                    if (readChoice == "1") {
                        string completeCmd = "COMPLETE_READ:" + to_string(num);
                        WriteFile(hPipe, completeCmd.c_str(), completeCmd.length() + 1, &bytesWritten, NULL);

                        
                        cout << "Access to record " << num << " completed." << endl;
                       

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