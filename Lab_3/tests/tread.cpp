#include "Header.h"


const int time_for_Sleep = 5;
HANDLE startMutex;
HANDLE arrayMutex;
HANDLE* continueEvents;
HANDLE* stopEvents;
bool* threadStopped;
LONG activeThreads;

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
            std::cout << "Thread number: " << i + 1 << std::endl;
            std::cout << "Number of marked elements: " << count << std::endl;
            std::cout << "It is not possible to mark an element with an index: " << b << std::endl;

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
        }
    }

    InterlockedDecrement(&activeThreads);
    return 0;
}

