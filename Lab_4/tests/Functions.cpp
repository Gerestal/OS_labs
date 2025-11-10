#include "Header.h"

vector<HANDLE> Processes;

wstring CreateCommandLine(const wstring& appPath, const wstring& params) {
    return L"\"" + appPath + L"\" " + params;
}

bool DoProcess(const wstring& appPath, const wstring& params) {
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    wstring cmdLine = CreateCommandLine(appPath, params);

    if (!CreateProcessW(
        nullptr,
        &cmdLine[0],
        NULL, NULL, FALSE,
        CREATE_NEW_CONSOLE,
        NULL, NULL,
        &si, &pi)) {
        throw runtime_error("Cannot start process: " +
            string(appPath.begin(), appPath.end()));
    }

    Processes.push_back(pi.hProcess);
    CloseHandle(pi.hThread); 

    return true;
}