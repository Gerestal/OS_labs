#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <stdexcept>

using namespace std;

extern vector<HANDLE> Processes;

bool DoProcess(const wstring& appPath, const wstring& params = L"");

wstring CreateCommandLine(const wstring& appPath, const wstring& params);
