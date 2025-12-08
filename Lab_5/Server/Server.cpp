#include <windows.h>
#undef max
#include <conio.h>
#include <iostream>
#include <string>
#include <fstream>
#include <exception>
#include <limits>
#include <vector>
#include <mutex>
#include <map>

using namespace std;


struct employee {
    int num;
    char name[10];
    double hours;
};

vector<HANDLE> Processes;



struct RecordLock {
    SRWLOCK lock;
    RecordLock() {
        InitializeSRWLock(&lock);
    }
};

map<int, RecordLock> recordLocks;
mutex recordMapMutex; 


bool DoProcess(const wstring& appPath, const wstring& params)
{
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));


    wstring cmdLine = L"\"" + appPath + L"\" " + params;


    if (!CreateProcessW(
        nullptr,
        &cmdLine[0],
        NULL, NULL, FALSE,
        CREATE_NEW_CONSOLE,
        NULL, NULL,
        &si, &pi))
    {
        throw runtime_error("Cannot start process: " +
            string(appPath.begin(), appPath.end()));
    }

    Processes.push_back(pi.hProcess);

    return true;
}

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


RecordLock& GetRecordLock(int employeeNum) {
    lock_guard<mutex> lock(recordMapMutex);
    return recordLocks[employeeNum];
}


void HandleClient(HANDLE hPipe, const string& filename) {
    char buffer[512];
    DWORD bytesRead;

    while (true) {
        
        BOOL success = ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL);
        if (!success || bytesRead == 0) {
            cout << "Client disconnected" << endl;
            break;
        }

        buffer[bytesRead] = '\0';
        string request(buffer);
       

        
        size_t colonPos = request.find(":");
        if (colonPos == string::npos) {
            cout << "Invalid request format" << endl;
            continue;
        }

        string operationStr = request.substr(0, colonPos);
        string employeeNumStr = request.substr(colonPos + 1);

        string zapros;

        if (operationStr == "1") {
            zapros = "Modification";
        }
        else zapros = "Reading";

        cout << "Request from client: " << zapros <<" for employee number "<< employeeNumStr<< endl;



        int employeeNum;
        try {
            employeeNum = stoi(employeeNumStr);
        }
        catch (...) {
            cout << "Error parsing employee number" << endl;
            continue;
        }

        if (operationStr == "1") { // modification
           
            RecordLock& recordLock = GetRecordLock(employeeNum);

            
            AcquireSRWLockExclusive(&recordLock.lock);

            try {
                
                employee emp = FindEmployeeInFile(employeeNum, filename);

                DWORD bytesWritten;
                WriteFile(hPipe, &emp, sizeof(employee), &bytesWritten, NULL);

                
                success = ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL);
                if (!success || bytesRead == 0) {
                    cout << "Client disconnected during modification" << endl;
                    ReleaseSRWLockExclusive(&recordLock.lock);
                    break;
                }

                buffer[bytesRead] = '\0';
                string response(buffer);

                if (response.find("COMPLETE:") == 0) {
                    cout << "Client completed access to record " << employeeNum << " without changes" << endl;
                }
                else if (bytesRead == sizeof(employee)) {
                   
                    employee newEmp;
                    memcpy(&newEmp, buffer, sizeof(employee));

                    if (newEmp.num == employeeNum) {
                        
                        bool result = ModifyEmployeeInFile(employeeNum, newEmp, filename);

                        string resultStr = result ? "SUCCESS" : "FAILED";
                        WriteFile(hPipe, resultStr.c_str(), resultStr.length() + 1, &bytesWritten, NULL);

                        cout << "Record " << employeeNum << " modified: " << (result ? "success" : "failed") << endl;

                        success = ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL);
                        if (success && bytesRead > 0) {
                            buffer[bytesRead] = '\0';
                            if (string(buffer).find("COMPLETE:") == 0) {
                                cout << "Client completed access to record " << employeeNum << endl;
                            }
                        }
                    }
                }

                ReleaseSRWLockExclusive(&recordLock.lock);

            }
            catch (...) {
                
                ReleaseSRWLockExclusive(&recordLock.lock);
                throw; 
            }

        }
        else if (operationStr == "2") { // reading
           
            RecordLock& recordLock = GetRecordLock(employeeNum);

            AcquireSRWLockShared(&recordLock.lock);

            try {
                
                employee emp = FindEmployeeInFile(employeeNum, filename);

                DWORD bytesWritten;
                WriteFile(hPipe, &emp, sizeof(employee), &bytesWritten, NULL);

                success = ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL);
                if (success && bytesRead > 0) {
                    buffer[bytesRead] = '\0';
                    string response(buffer);
                    if (response.find("COMPLETE_READ:") == 0) {
                        cout << "Client completed read access to record " << employeeNum << endl;
                    }
                }

                ReleaseSRWLockShared(&recordLock.lock);

            }
            catch (...) {
                ReleaseSRWLockShared(&recordLock.lock);
                throw;
            }
        }
        else if (operationStr == "COMPLETE" || operationStr == "COMPLETE_READ") {
            cout << "Received completion command for record " << employeeNum << endl;
        }
        else {
            cout << "Unknown operation: " << operationStr << endl;
        }
    }

    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);
}


int main() {
    try {
        string filename;
        cout << "Enter filename: ";
        if (!getline(cin, filename) || filename.empty()) {
            throw runtime_error("Filename required");
        }

        int eCount;
        while (true) {
            cout << "Enter the number of employees: ";
            cin >> eCount;
            if (!cin) throw runtime_error("EOF or bad input");
            if (eCount < 0) {
                cout << "Error: Number of employees can't be negative.\n";
                continue;
            }
            break;
        }


        ofstream fout(filename, ios::binary);
        fout.exceptions(ofstream::failbit | ofstream::badbit);

        for (int i = 0; i < eCount; ++i) {
            employee emp;
            string tmp;

            cout << "Enter information about employee number " << (i + 1) << "\n";

            while (true) {
                cout << "Employee's id number: ";
                if (!(cin >> emp.num)) {
                    cout << "Error: please enter a valid integer for employee id.\n";
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    continue;
                }
                if (emp.num < 0) {
                    cout << "Error: id can't be negative.\n";
                    continue;
                }
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                break;
            }

            while (true) {
                cout << "Employee's name: ";
                if (!getline(cin, tmp)) {
                    cout << "Input error. Please try again.\n";
                    continue;
                }

                size_t start = tmp.find_first_not_of(" \t");
                size_t end = tmp.find_last_not_of(" \t");

                if (start == string::npos) {
                    cout << "Name can't be empty. Please try again.\n";
                    continue;
                }

                tmp = tmp.substr(start, end - start + 1);


                if (tmp.length() > 9) {
                    cout << "Error: Name is too long. Maximum length is 9 characters.\n";
                    continue;
                }


                strcpy_s(emp.name, sizeof(emp.name), tmp.c_str());
                break;
            }

            while (true) {
                cout << "Number of working hours: ";
                if (!(cin >> emp.hours)) {
                    cout << "Error: please enter a valid count of hours.\n";
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    continue;
                }
                if (emp.hours < 0) {
                    cout << "Error: Working hours can't be negative.\n";
                    continue;
                }
                break;
            }

            fout.write(reinterpret_cast<const char*>(&emp), sizeof(emp));
        }

        fout.close();
        cout << "Done. Wrote " << eCount << " records into \"" << filename << "\"\n";
        cout << "File content: " << endl;
        PrintFile(filename);

        int num_prosses;
        cout << "Enter number of prosses:";
        cin >> num_prosses;
        if (cin.fail() || num_prosses <= 0) {
            throw runtime_error("Incorrect array size.");
        }

        wstring senderPath = L"Client.exe";
        wstring senderParams = wstring(filename.begin(), filename.end()) + L" " + to_wstring(eCount);


        for (int i = 0; i < num_prosses; ++i) {
            DoProcess(senderPath, senderParams);
        }

        vector<HANDLE> Pipes (num_prosses);
        vector<thread> clientThreads;

        for (int i = 0; i < num_prosses; ++i) {
            Pipes[i] = CreateNamedPipeW(
                L"\\\\.\\pipe\\NamedPipe",                
                PIPE_ACCESS_DUPLEX,     
                PIPE_TYPE_MESSAGE |    
                PIPE_READMODE_MESSAGE | 
                PIPE_WAIT,              
                PIPE_UNLIMITED_INSTANCES,
                512,                   
                512,                    
                0,                     
                NULL);                  

            if (Pipes[i] == INVALID_HANDLE_VALUE) {
                cerr << "Pipe's creating error: " << GetLastError() << endl;
                return 1;
            }

            BOOL connected = ConnectNamedPipe(Pipes[i], NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

            clientThreads.emplace_back(HandleClient, Pipes[i], filename);

        }

        WaitForMultipleObjects(Processes.size(), Processes.data(), TRUE, INFINITE);

        cout << "All client processes completed. Server stops requests processing." << endl;
        for (auto& thread : clientThreads) {
            if (thread.joinable()) {
                thread.join();
            }
        }

        for (auto& hProcess : Processes) {
            CloseHandle(hProcess);
        }
        Processes.clear();

        for (auto& hPipe : Pipes) {
            CloseHandle(hPipe);
        }

        cout << "\nFile content: " << endl;
        PrintFile(filename);

        cout << "Enter 1 to shutdown server: " << endl;

        string command;
        while (true) {
            getline(cin, command);
            if (command == "1") {
                break;
            }
            cout << "Unknown command. Enter 1 to shutdown server: " << endl;
        }

        cout << "Server shutdown complete." << endl;
        return 0;
      
    }

    catch (const exception& ex) {
            cerr << "Exception: " << ex.what() << '\n';

            for (auto& h : Processes) {
                CloseHandle(h);
            }
            return 1;
        }
        return 0;
    }