#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>

using namespace std;

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            throw invalid_argument(string("Usage: ") + argv[0] + " <filename>");
        }

        const char* filename = argv[1];

        ofstream in(filename, ios::binary | ios::app);
        if (!in) {
            cerr << "The file does not open: " << filename << '\n';
            return 1;
        }

        const char* startName = "Global\\StartSemaphore";
        HANDLE hStart = OpenSemaphoreA(SEMAPHORE_MODIFY_STATE, FALSE, startName);
        if (!hStart) {
            cerr << "OpenSemaphoreA(Start) failed: " << GetLastError() << '\n';
            return 1;
        }
        if (!ReleaseSemaphore(hStart, 1, nullptr)) {
            cerr << "ReleaseSemaphore(Start) failed: " << GetLastError() << '\n';
            CloseHandle(hStart);
            return 2;
        }
        CloseHandle(hStart);

        
        const char* mutexName = "Global\\mutexSemaphore";
        HANDLE hMutex = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, mutexName);
        if (!hMutex) {
            cerr << "OpenSemaphoreA(Mutex) failed: " << GetLastError() << '\n';
            return 1;
        }
        const char* emptyName = "Global\\emptySemaphore";
        HANDLE hEmpty = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, emptyName);
        if (!hEmpty) {
            cerr << "OpenSemaphoreA(Empty) failed: " << GetLastError() << '\n';
            CloseHandle(hMutex);
            return 1;
        }
        const char* fullName = "Global\\fullSemaphore";
        HANDLE hFull = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, fullName);
        if (!hFull) {
            cerr << "OpenSemaphoreA(Full) failed: " << GetLastError() << '\n';
            CloseHandle(hMutex);
            CloseHandle(hEmpty);
            return 1;
        }


        while (true) {
            cout << "Enter command:\n";
            cout << "  1 - send a message to file\n";
            cout << "  2 - exit\n";

            string cmd;
            if (!getline(cin, cmd)) {
                cerr << "Input error. Exiting.\n";
                break;
            }

            while (!cmd.empty() && isspace((unsigned char)cmd.back())) cmd.pop_back();
            size_t pos = 0;
            while (pos < cmd.size() && isspace((unsigned char)cmd[pos])) ++pos;
            if (pos > 0) cmd.erase(0, pos);

            if (cmd == "1") {
               
                string line;
                while (true) {
                    cout << "Enter the message (each <= 20 characters). Empty line is an input: ";
                    if (!getline(cin, line)) {
                        cerr << "Input error while reading message. Cancelled.\n";
                        break;
                    }

                    if (line.empty()) {
                        throw runtime_error("Massege required");
                    }

                    if (line.size() > 20) {
                        cout << "The message must be less than 21 characters long. Try again.\n";
                        continue;
                    }

                    
                    DWORD wt = WaitForSingleObject(hEmpty, INFINITE);
                    if (wt != WAIT_OBJECT_0) {
                        cerr << "WaitForSingleObject(empty) failed: " << GetLastError() << '\n';
                        break;
                    }

                    wt = WaitForSingleObject(hMutex, INFINITE);
                    if (wt != WAIT_OBJECT_0) {
                        cerr << "WaitForSingleObject(mutex) failed: " << GetLastError() << '\n';
                        ReleaseSemaphore(hEmpty, 1, nullptr);
                        break;
                    }

                    in.write(line.data(), static_cast<streamsize>(line.size()));
                    in.put('\n');
                    in.flush();

                    
                    if (!ReleaseSemaphore(hMutex, 1, NULL)) {
                        cerr << "ReleaseSemaphore(mutex) failed: " << GetLastError() << '\n';
                    }
                    if (!ReleaseSemaphore(hFull, 1, NULL)) {
                        cerr << "ReleaseSemaphore(full) failed: " << GetLastError() << '\n';
                    }

                    if (!in) {
                        cerr << "File write error\n";
                    }

                    break; 
                }
            }
            else if (cmd == "2") {
                cout << "Exiting by user command.\n";
                break;
            }
            else {
                cout << "Unknown command. Please enter 1 or 2.\n";
                continue;
            }
        }

       
        CloseHandle(hMutex);
        CloseHandle(hEmpty);
        CloseHandle(hFull);
        in.close();
    }
    catch (const exception& ex) {
        cerr << "Exception: " << ex.what() << '\n';
        return 1;
    }
    return 0;
}
