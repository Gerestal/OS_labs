#include <iostream>
#include <windows.h>
#include <string>
#include <fstream>
#include <vector>
#undef max
using namespace std;

vector<HANDLE> Processes;

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


int main() {
	try {
		string filename;
        cout << "Enter filename: ";
        if (!getline(cin, filename) || filename.empty()) {
            throw runtime_error("Filename required");
        }
        int num_mess;
		cout << "Enter number of messages:";
	    cin >> num_mess;
		if (cin.fail() || num_mess <= 0) {
			throw runtime_error("Incorrect array size.");
		}
		int num_prosses;
        cout << "Enter number of prosses:";
		cin >> num_prosses;
		if (cin.fail() || num_prosses <= 0) {
			throw runtime_error("Incorrect array size.");
		}

        const char* mutex = "Global\\mutexSemaphore";
        const char* empty = "Global\\emptySemaphore";
        const char* full = "Global\\fullSemaphore";
        HANDLE mut = CreateSemaphoreA(nullptr, 1, 1, mutex);
        HANDLE emp = CreateSemaphoreA(nullptr, num_mess, num_mess, empty);
        HANDLE f = CreateSemaphoreA(nullptr, 0, num_mess, full);
        if (!mut || !emp || !f) {
            throw runtime_error("Failed to create one of semaphores. Error: " + to_string(GetLastError()));
        }

        const char* name = "Global\\StartSemaphore";

        HANDLE h = CreateSemaphoreA(nullptr, 0, LONG_MAX, name);

        wstring senderPath = L"Sender.exe";
        wstring senderParams = wstring(filename.begin(), filename.end());
        

		for (int i = 0; i < num_prosses; ++i) {
            DoProcess(senderPath, senderParams);
		}

        
        if (!h) {
            throw runtime_error("CreateSemaphoreA(StartSemaphore) failed: " + to_string(GetLastError()));
        }
        for (int i = 0; i < num_prosses; ++i) {
            DWORD r = WaitForSingleObject(h, INFINITE);
            if (r != WAIT_OBJECT_0) {
                throw runtime_error("WaitForSingleObject failed: " + to_string(GetLastError()));
            }
           
        }
        cout << "All processes are ready to work.\n";
        CloseHandle(h);

        ifstream out(filename, ios::binary);
        if (!out) {
            throw runtime_error("Cannot open file for reading: " + filename);
        }
        out.seekg(0, ios::beg);
        string line;
        while (true) {
            int cmd = 0;
                cout << "Enter 1 to continue, 2 to exit:";
                if (!(cin >> cmd)) {
                    cerr << "Input error. Exiting.\n";
                    break;
                }

                if (cmd == 1) {
                    while (true) {
                        WaitForSingleObject(f, INFINITE);
                        WaitForSingleObject(mut, INFINITE);
                        if (!getline(out, line)) {
                            if (out.eof()) {
                                ReleaseSemaphore(mut, 1, nullptr);
                                ReleaseSemaphore(emp, 1, nullptr);
                                continue;
                            }
                            else {
                                ReleaseSemaphore(mut, 1, nullptr);
                                ReleaseSemaphore(emp, 1, nullptr);
                                throw runtime_error("Error reading from file");
                            }
                        }
                        cout << "Received line: " << line << '\n';

                        if (!ReleaseSemaphore(mut, 1, nullptr)) {
                            throw runtime_error("ReleaseSemaphore(mutex) failed: " + to_string(GetLastError()));
                        }
                        if (!ReleaseSemaphore(emp, 1, nullptr)) {
                            throw runtime_error("ReleaseSemaphore(empty) failed: " + to_string(GetLastError()));
                        }
                        break;
                    }
                }
               
                
                else if (cmd == 2) {
                    cout << "Exiting by user command.\n";
                    break;
                }
                else {
                    cout << "Unknown command. Please enter 1 or 2.\n";
                    continue;
                }
        }

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