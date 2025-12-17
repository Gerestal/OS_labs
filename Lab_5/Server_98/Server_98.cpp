#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#undef max
#include <conio.h>
#include <iostream>
#include <string>
#include <fstream>
#include <exception>
#include <limits>
#include <vector>
#include <map>
#include <cstdlib> 
#include <cstring> 

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
CRITICAL_SECTION recordMapCS; 


struct ThreadArg {
    HANDLE hPipe;
    string filename;
};


bool DoProcess(const wstring& appPath, const wstring& params)
{
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    
    std::wstring cmdLine = L"\"" + appPath + L"\" " + params;
    wchar_t* cmd = new wchar_t[cmdLine.size() + 1];
    wcscpy(cmd, cmdLine.c_str());

    BOOL ok = CreateProcessW(
        NULL,
        cmd,
        NULL, NULL, FALSE,
        CREATE_NEW_CONSOLE,
        NULL, NULL,
        &si, &pi);

    delete[] cmd;

    if (!ok)
    {
        throw runtime_error("Cannot start process");
    }

    
    CloseHandle(pi.hThread);
    Processes.push_back(pi.hProcess);

    return true;
}

void PrintFile(const string& fileName)
{
    employee emp;
    ifstream file(fileName.c_str(), ios::binary);
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
    ifstream file(filename.c_str(), ios::binary);
    if (!file) {
        employee empty = { 0, "", 0.0 };
        return empty;
    }

    employee emp;
    while (file.read(reinterpret_cast<char*>(&emp), sizeof(employee))) {
        if (emp.num == employeeNum) {
            file.close();
            return emp;
        }
    }

    file.close();
    employee empty = { 0, "", 0.0 };
    return empty;
}

bool ModifyEmployeeInFile(int employeeNum, const employee& newEmp, const string& filename) {
    fstream file(filename.c_str(), ios::binary | ios::in | ios::out);
    if (!file) {
        return false;
    }

    employee emp;
    streampos pos = 0;
    while (file.read(reinterpret_cast<char*>(&emp), sizeof(employee))) {
        if (emp.num == employeeNum) {
            file.seekp(pos);
            file.write(reinterpret_cast<const char*>(&newEmp), sizeof(employee));
            file.close();
            return true;
        }
        pos = file.tellg();
    }

    file.close();
    return false;
}


RecordLock& GetRecordLock(int employeeNum) {
    EnterCriticalSection(&recordMapCS);
    RecordLock& rl = recordLocks[employeeNum];
    LeaveCriticalSection(&recordMapCS);
    return rl;
}


DWORD WINAPI ThreadProc(LPVOID lpParam);


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

        cout << "Request from client: " << zapros << " for employee number " << employeeNumStr << endl;

        int employeeNum = atoi(employeeNumStr.c_str());

        if (operationStr == "1") { // modification
            RecordLock& recordLock = GetRecordLock(employeeNum);

            
            AcquireSRWLockExclusive(&recordLock.lock);

            
            employee emp = FindEmployeeInFile(employeeNum, filename);

            DWORD bytesWritten;
            WriteFile(hPipe, &emp, sizeof(employee), &bytesWritten, NULL);

            
            ReleaseSRWLockExclusive(&recordLock.lock);

            success = ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL);
            if (!success || bytesRead == 0) {
                cout << "Client disconnected during modification" << endl;
                break;
            }

            if (bytesRead == sizeof(employee)) {
                employee newEmp;
                memcpy(&newEmp, buffer, sizeof(employee));

                if (newEmp.num == employeeNum) {
                    
                    AcquireSRWLockExclusive(&recordLock.lock);
                    bool result = ModifyEmployeeInFile(employeeNum, newEmp, filename);
                    ReleaseSRWLockExclusive(&recordLock.lock);

                    string resultStr = result ? "SUCCESS" : "FAILED";
                    WriteFile(hPipe, resultStr.c_str(), (DWORD)resultStr.length() + 1, &bytesWritten, NULL);

                    cout << "Record " << employeeNum << " modified: " << (result ? "success" : "failed") << endl;

                   
                    success = ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL);
                    if (success && bytesRead > 0) {
                        buffer[bytesRead] = '\0';
                        if (string(buffer).find("COMPLETE:") == 0) {
                            cout << "Client completed access to record " << employeeNum << endl;
                        }
                    }
                }
                else {
                    cout << "Received employee data with mismatched id" << endl;
                }
            }
            else {
               
                buffer[bytesRead] = '\0';
                string response(buffer);
                if (response.find("COMPLETE:") == 0) {
                    cout << "Client completed access to record " << employeeNum << " without changes" << endl;
                }
            }
        }

        else if (operationStr == "2") { // reading
            RecordLock& recordLock = GetRecordLock(employeeNum);

            AcquireSRWLockShared(&recordLock.lock);

            employee emp = FindEmployeeInFile(employeeNum, filename);

            DWORD bytesWritten;
            WriteFile(hPipe, &emp, sizeof(employee), &bytesWritten, NULL);

           
            ReleaseSRWLockShared(&recordLock.lock);

           
            success = ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL);
            if (success && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                string response(buffer);
                if (response.find("COMPLETE_READ:") == 0) {
                    cout << "Client completed read access to record " << employeeNum << endl;
                }
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


DWORD WINAPI ThreadProc(LPVOID lpParam) {
    ThreadArg* arg = (ThreadArg*)lpParam;
    if (arg) {
        HandleClient(arg->hPipe, arg->filename);
        delete arg;
    }
    return 0;
}

int main() {
    try {
        
        InitializeCriticalSection(&recordMapCS);

        string filename;
        cout << "Enter filename: ";
        if (!getline(cin, filename) || filename.empty()) {
            throw runtime_error("Filename required");
        }

        int eCount;
        while (true) {
            cout << "Enter the number of employees: ";
            if (!(cin >> eCount)) {
                throw runtime_error("EOF or bad input");
            }
            if (eCount < 0) {
                cout << "Error: Number of employees can't be negative.\n";
                continue;
            }
            break;
        }

        
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        ofstream fout(filename.c_str(), ios::binary);
        if (!fout) {
            throw runtime_error("Cannot open file for writing");
        }

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

                
                memset(emp.name, 0, sizeof(emp.name));
                strncpy(emp.name, tmp.c_str(), sizeof(emp.name) - 1);
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
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                break;
            }

            fout.write(reinterpret_cast<const char*>(&emp), sizeof(emp));
        }

        fout.close();
        cout << "Done. Wrote " << eCount << " records into \"" << filename << "\"\n";
        cout << "File content: " << endl;
        PrintFile(filename);

        int num_prosses;
        cout << "Enter number of processes:";
        if (!(cin >> num_prosses)) {
            throw runtime_error("Incorrect array size.");
        }
        if (num_prosses <= 0) {
            throw runtime_error("Incorrect array size.");
        }

        
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        
        wstring senderPath = L"Client_98.exe";
        
        wstring wfilename(filename.begin(), filename.end());
        wstring senderParams = wfilename + L" " + to_wstring(eCount); 

        
        char tmpbuf[32];
        _snprintf(tmpbuf, sizeof(tmpbuf), "%d", eCount);
        wstring wparams = wfilename + L" " + std::wstring(tmpbuf, tmpbuf + strlen(tmpbuf));

       
        for (int i = 0; i < num_prosses; ++i) {
            
            DoProcess(senderPath, wparams);
        }

       
        vector<HANDLE> Pipes(num_prosses);
        vector<HANDLE> clientThreads;

        for (int i = 0; i < num_prosses; ++i) {
            Pipes[i] = CreateNamedPipeW(
                L"\\\\.\\pipe\\NamedPipe",
                PIPE_ACCESS_DUPLEX,
                PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
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

            
            ThreadArg* arg = new ThreadArg();
            arg->hPipe = Pipes[i];
            arg->filename = filename;

            HANDLE hThread = CreateThread(NULL, 0, ThreadProc, arg, 0, NULL);
            if (hThread == NULL) {
                cerr << "CreateThread failed: " << GetLastError() << endl;
                delete arg;
                CloseHandle(Pipes[i]);
                return 1;
            }

            clientThreads.push_back(hThread);
        }

        
        if (!Processes.empty()) {
            WaitForMultipleObjects((DWORD)Processes.size(), Processes.data(), TRUE, INFINITE);
        }

        cout << "All client processes completed. Server stops requests processing." << endl;

        
        if (!clientThreads.empty()) {
            WaitForMultipleObjects((DWORD)clientThreads.size(), clientThreads.data(), TRUE, INFINITE);
        }

        for (size_t i = 0; i < Processes.size(); ++i) {
            CloseHandle(Processes[i]);
        }
        Processes.clear();

        for (size_t i = 0; i < Pipes.size(); ++i) {
            CloseHandle(Pipes[i]);
        }

        for (size_t i = 0; i < clientThreads.size(); ++i) {
            CloseHandle(clientThreads[i]);
        }

        cout << "\nFile content: " << endl;
        PrintFile(filename);

        cout << "Enter 1 to shutdown server: " << endl;

        string command;
        while (true) {
            if (!getline(cin, command)) break;
            if (command == "1") {
                break;
            }
            cout << "Unknown command. Enter 1 to shutdown server: " << endl;
        }

        cout << "Server shutdown complete." << endl;

        
        DeleteCriticalSection(&recordMapCS);

        return 0;
    }

    catch (const exception& ex) {
        cerr << "Exception: " << ex.what() << '\n';

        for (size_t i = 0; i < Processes.size(); ++i) {
            CloseHandle(Processes[i]);
        }
        
        DeleteCriticalSection(&recordMapCS);
        return 1;
    }

    return 0;
}
