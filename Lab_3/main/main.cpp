#include <windows.h>
#include <iostream>
#include <vector>
using namespace std;

const int time_for_Sleep = 5;

HANDLE startMutex;
HANDLE arrayMutex;
HANDLE* continueEvents;
HANDLE* stopEvents;
bool* threadStopped;
LONG activeThreads;

struct ThreadArgs {
    int* arr;
    int index;
    int len;
};

DWORD WINAPI marker(LPVOID args) {
    ThreadArgs* args1 = static_cast<ThreadArgs*>(args);
    int* arr = args1->arr;
    int i = args1->index;
    int len = args1->len;

    
    WaitForSingleObject(startMutex, INFINITE);
    ReleaseMutex(startMutex);

    srand(i);

    int count = 0;
    bool working = true;

    while (working) {
        int a = rand();
        int b = a % len;

        WaitForSingleObject(arrayMutex, INFINITE);

        
        if (arr[b] == 0) {
            Sleep(time_for_Sleep);
            arr[b] = i; 
            Sleep(time_for_Sleep);
            count++;
            ReleaseMutex(arrayMutex);
        }
        else {
            cout << "Thread number: " << i + 1 << endl;
            cout << "Number of marked elements: " << count << endl;
            cout << "It is not possible to mark an element with an index: " << b << endl;

            ReleaseMutex(arrayMutex);
            SetEvent(stopEvents[i]);
            DWORD result = WaitForSingleObject(continueEvents[i], INFINITE);
            ResetEvent(continueEvents[i]);

            if (threadStopped[i]) {
                working = false;
                WaitForSingleObject(arrayMutex, INFINITE);
                for (int j = 0; j < len; j++) {
                    if (arr[j] == i) {
                        arr[j] = 0;
                    }
                }
                ReleaseMutex(arrayMutex);
            }
            else {
                continue;
            }
        }
    }

    InterlockedDecrement(&activeThreads); //activeThreads --
    return 0;
}

int main() {
  
    int n = 0;
    int kol = 0;
    int* arr = nullptr;
    vector<HANDLE> hThread;
    vector<ThreadArgs*> threadArgs;

    try {
        
        cout << "Enter the size of the array: ";
        cin >> n;
        if (cin.fail() || n <= 0) {
            throw runtime_error("Incorrect array size.");
        }

        arr = new int[n];
        for (int i = 0; i < n; i++) {
            arr[i] = 0;
        }

        cout << "Enter the number of threads: ";
        cin >> kol;
        if (cin.fail() || kol <= 0) {
            throw runtime_error("Incorrect number of threads.");
        }

        activeThreads = kol;

        startMutex = CreateMutex(NULL, TRUE, NULL); 
        arrayMutex = CreateMutex(NULL, FALSE, NULL);

        continueEvents = new HANDLE[kol];
        stopEvents = new HANDLE[kol];
        threadStopped = new bool[kol];

        hThread.resize(kol, NULL);

        for (int i = 0; i < kol; ++i) {
            continueEvents[i] = CreateEvent(NULL, TRUE, FALSE, NULL);  
            stopEvents[i] = CreateEvent(NULL, TRUE, FALSE, NULL);      
            threadStopped[i] = false;
        }

        for (int i = 0; i < kol; ++i) {
            ThreadArgs* a = new ThreadArgs{ arr, i, n };
            threadArgs.push_back(a);

            HANDLE h = CreateThread(NULL, 0, marker, a, 0, NULL);
            if (h == NULL) {
                throw runtime_error("Thread creation error");
            }
            hThread[i] = h;
        }

        ReleaseMutex(startMutex);

        while (activeThreads > 0) {
           
            vector<HANDLE> activeStopEvents;
            for (int i = 0; i < kol; i++) {
                if (!threadStopped[i]) {
                    activeStopEvents.push_back(stopEvents[i]);
                }
            }

            if (activeStopEvents.empty()) {
                break;
            }

            DWORD result = WaitForMultipleObjects(activeStopEvents.size(), activeStopEvents.data(), TRUE, 5000);

            if (result == WAIT_FAILED) {
                DWORD error = GetLastError();
                cerr << "WaitForMultipleObjects failed with error: " << error << endl;
                break;
            }
            else if (result == WAIT_TIMEOUT) {
                cout << "Timeout: Checking if all threads are still working..." << endl;

                bool allWorking = false;
                WaitForSingleObject(arrayMutex, INFINITE);
                for (int i = 0; i < n; i++) {
                    if (arr[i] == 0) {
                        allWorking = true;
                        break;
                    }
                }
                ReleaseMutex(arrayMutex);

                if (!allWorking) {
                    cout << "No zero elements left. Forcing all threads to stop." << endl;
                    for (int i = 0; i < kol; i++) {
                        if (!threadStopped[i]) {
                            threadStopped[i] = true;
                            SetEvent(continueEvents[i]);
                            WaitForSingleObject(hThread[i], 1000);
                            CloseHandle(hThread[i]);
                            hThread[i] = NULL;
                        }
                    }
                    break;
                }
                else {
                    continue;
                }
            }

            WaitForSingleObject(arrayMutex, INFINITE);
            cout << "Array contents: ";
            for (int i = 0; i < n; i++) {
                cout << arr[i] << " ";
            }
            cout << endl;
            ReleaseMutex(arrayMutex);

            int konec;
            cout << "Enter the thread number to complete: ";
            cin >> konec;

            if (cin.fail() || konec < 1 || konec > kol) {
                cout << "Incorrect thread number." << endl;
                for (int i = 0; i < kol; i++) {
                    if (!threadStopped[i]) {
                        ResetEvent(stopEvents[i]);
                        SetEvent(continueEvents[i]);
                    }
                }
                continue;
            }

            int threadIndex = konec - 1;

            if (threadStopped[threadIndex]) {
                cout << "Thread " << konec << " is already stopped." << endl;
                for (int i = 0; i < kol; i++) {
                    if (!threadStopped[i]) {
                        ResetEvent(stopEvents[i]);
                        SetEvent(continueEvents[i]);
                    }
                }
                continue;
            }

            threadStopped[threadIndex] = true;

            SetEvent(continueEvents[threadIndex]); 
            WaitForSingleObject(hThread[threadIndex], INFINITE);

            ResetEvent(stopEvents[threadIndex]);
            ResetEvent(continueEvents[threadIndex]);
            CloseHandle(hThread[threadIndex]);
            hThread[threadIndex] = NULL;

            WaitForSingleObject(arrayMutex, INFINITE);
            cout << "Array contents after stopping thread " << konec << ": ";
            for (int i = 0; i < n; i++) {
                cout << arr[i] << " ";
            }
            cout << endl;
            ReleaseMutex(arrayMutex);

            for (int i = 0; i < kol; i++) {
                if (!threadStopped[i]) {
                    ResetEvent(stopEvents[i]);
                    SetEvent(continueEvents[i]);
                }
            }
        }

        cout << "All threads have been completed." << endl;
    }
    catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        if (arr) delete[] arr;
        if (continueEvents) delete[] continueEvents;
        if (stopEvents) delete[] stopEvents;
        if (threadStopped) delete[] threadStopped;

        for (HANDLE h : hThread) {
            if (h != NULL) CloseHandle(h);
        }
        for (ThreadArgs* a : threadArgs) {
            delete a;
        }

        if (startMutex) CloseHandle(startMutex);
        if (arrayMutex) CloseHandle(arrayMutex);

        return 1;
    }

    if (arr) {
        delete[] arr;
        arr = nullptr;
    }

    for (int i = 0; i < kol; i++) {
        if (hThread[i] != NULL) {
            CloseHandle(hThread[i]);
            hThread[i] = NULL;
        }
        if (continueEvents && continueEvents[i]) {
            CloseHandle(continueEvents[i]);
        }
        if (stopEvents && stopEvents[i]) {
            CloseHandle(stopEvents[i]);
        }
    }

    for (ThreadArgs* a : threadArgs) {
        delete a;
    }
    threadArgs.clear();

    if (continueEvents) {
        delete[] continueEvents;
        continueEvents = nullptr;
    }

    if (stopEvents) {
        delete[] stopEvents;
        stopEvents = nullptr;
    }

    if (threadStopped) {
        delete[] threadStopped;
        threadStopped = nullptr;
    }

    if (startMutex) {
        CloseHandle(startMutex);
        startMutex = NULL;
    }

    if (arrayMutex) {
        CloseHandle(arrayMutex);
        arrayMutex = NULL;
    }

    return 0;
}