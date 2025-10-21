#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include <windows.h>
#include <vector>
#include <thread>
#include <chrono>
#include "Header.h" 


extern HANDLE startMutex;
extern HANDLE arrayMutex;
extern HANDLE* continueEvents;
extern HANDLE* stopEvents;
extern bool* threadStopped;
extern LONG activeThreads;

const int TIMEOUT_MS = 5000;

TEST_CASE("marker thread notifies and cleans up on stop", "[marker][thread][winapi]") {
    const int n = 10;    
    const int kol = 1;   

    
    int* arr = new int[n];
    for (int i = 0; i < n; ++i) arr[i] = 0;

    
    activeThreads = kol;

    startMutex = CreateMutex(NULL, TRUE, NULL); 
    REQUIRE(startMutex != NULL);

    arrayMutex = CreateMutex(NULL, FALSE, NULL);
    REQUIRE(arrayMutex != NULL);

    continueEvents = new HANDLE[kol];
    stopEvents = new HANDLE[kol];
    threadStopped = new bool[kol];

    continueEvents[0] = CreateEvent(NULL, TRUE, FALSE, NULL); 
    stopEvents[0] = CreateEvent(NULL, TRUE, FALSE, NULL);     
    threadStopped[0] = false;

    
    ThreadArgs* args = new ThreadArgs{ arr, 0, n };

    HANDLE hThread = CreateThread(NULL, 0, marker, args, 0, NULL);
    REQUIRE(hThread != NULL);

   
    ReleaseMutex(startMutex);

    
    {
        WaitForSingleObject(arrayMutex, INFINITE);
        arr[0] = -1; 
        ReleaseMutex(arrayMutex);
    }

    
    DWORD waitResult = WaitForSingleObject(stopEvents[0], TIMEOUT_MS);
    REQUIRE(waitResult == WAIT_OBJECT_0); 

    threadStopped[0] = true;

    
    SetEvent(continueEvents[0]);

    
    DWORD joinRes = WaitForSingleObject(hThread, TIMEOUT_MS);
    REQUIRE(joinRes == WAIT_OBJECT_0);

    
    bool foundThreadMark = false;
    WaitForSingleObject(arrayMutex, INFINITE);
    for (int i = 0; i < n; ++i) {
        if (arr[i] == 0) continue;
        if (arr[i] == 0) { foundThreadMark = true; break; }
        if (arr[i] == args->index) { foundThreadMark = true; break; } 
    }
    ReleaseMutex(arrayMutex);

    
    REQUIRE(foundThreadMark == false);

    
    CloseHandle(hThread);
    CloseHandle(startMutex);
    CloseHandle(arrayMutex);
    CloseHandle(continueEvents[0]);
    CloseHandle(stopEvents[0]);

    delete[] continueEvents;
    delete[] stopEvents;
    delete[] threadStopped;
    delete[] arr;
    delete args;
}
