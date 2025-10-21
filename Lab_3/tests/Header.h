#pragma once
#ifndef MARKER_H
#define MARKER_H

#include <windows.h>
#include <iostream>
#include <vector>


extern const int time_for_Sleep;
extern HANDLE startMutex;
extern HANDLE arrayMutex;
extern HANDLE* continueEvents;
extern HANDLE* stopEvents;
extern bool* threadStopped;
extern LONG activeThreads;

struct ThreadArgs {
    int* arr;
    int index;
    int len;
};


DWORD WINAPI marker(LPVOID args);


#endif
