#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "Header.h"
#include <windows.h>
#include <fstream>

using namespace std;


bool FileExists(const wstring& path) {
    DWORD attributes = GetFileAttributesW(path.c_str());
    return (attributes != INVALID_FILE_ATTRIBUTES &&
        !(attributes & FILE_ATTRIBUTE_DIRECTORY));
}

TEST_CASE("CreateCommandLine function") {
    SUBCASE("Command line with path containing spaces") {
        wstring appPath = L"C:\\Program Files\\My App\\app.exe";
        wstring params = L"--param1 value1 --param2 value2";
        wstring result = CreateCommandLine(appPath, params);
        wstring expected = L"\"C:\\Program Files\\My App\\app.exe\" --param1 value1 --param2 value2";
        CHECK(result == expected);
    }

    SUBCASE("Command line with empty parameters") {
        wstring appPath = L"notepad.exe";
        wstring params = L"";
        wstring result = CreateCommandLine(appPath, params);
        wstring expected = L"\"notepad.exe\" ";
        CHECK(result == expected);
    }

    SUBCASE("Command line with simple path") {
        wstring appPath = L"cmd.exe";
        wstring params = L"/c dir";
        wstring result = CreateCommandLine(appPath, params);
        wstring expected = L"\"cmd.exe\" /c dir";
        CHECK(result == expected);
    }
}

TEST_CASE("DoProcess function integration tests") {
   
    size_t initialProcessCount = Processes.size();

    SUBCASE("Successful process creation with valid executable") {
        
        wstring systemDir(MAX_PATH, L'\0');
        GetSystemDirectoryW(&systemDir[0], MAX_PATH);
        systemDir.resize(wcslen(systemDir.c_str()));
        wstring notepadPath = systemDir + L"\\notepad.exe";

        if (FileExists(notepadPath)) {
            CHECK_NOTHROW(DoProcess(notepadPath));

           
            CHECK(Processes.size() == initialProcessCount + 1);

            
            CHECK(Processes.back() != NULL);
            CHECK(Processes.back() != INVALID_HANDLE_VALUE);
        }
        else {
            WARN("notepad.exe not found, skipping test");
        }
    }

    SUBCASE("Process creation with parameters") {
        wstring systemDir(MAX_PATH, L'\0');
        GetSystemDirectoryW(&systemDir[0], MAX_PATH);
        systemDir.resize(wcslen(systemDir.c_str()));
        wstring cmdPath = systemDir + L"\\cmd.exe";

        if (FileExists(cmdPath)) {
            CHECK_NOTHROW(DoProcess(cmdPath, L"/c echo Hello World"));
            CHECK(Processes.size() == initialProcessCount + 1);
        }
        else {
            WARN("cmd.exe not found, skipping test");
        }
    }

    SUBCASE("Process creation with non-existent executable throws exception") {
        wstring invalidPath = L"C:\\Nonexistent\\Path\\fake_executable_12345.exe";
        CHECK_THROWS_AS(DoProcess(invalidPath), runtime_error);

       
        CHECK(Processes.size() == initialProcessCount);
    }

    SUBCASE("Process creation with empty path throws exception") {
        CHECK_THROWS_AS(DoProcess(L""), runtime_error);
        CHECK(Processes.size() == initialProcessCount);
    }

    SUBCASE("Process creation with relative path") {
       
        wstring calcPath = L"calc.exe";

       
        if (FileExists(L"C:\\Windows\\System32\\calc.exe")) {
            CHECK_NOTHROW(DoProcess(calcPath));
            CHECK(Processes.size() == initialProcessCount + 1);
        }
        else {
            WARN("calc.exe not found, skipping test");
        }
    }

   
    for (size_t i = initialProcessCount; i < Processes.size(); ++i) {
        if (Processes[i] && Processes[i] != INVALID_HANDLE_VALUE) {
           
            DWORD exitCode;
            if (GetExitCodeProcess(Processes[i], &exitCode) && exitCode == STILL_ACTIVE) {
                TerminateProcess(Processes[i], 0);
                WaitForSingleObject(Processes[i], 1000);
            }
            CloseHandle(Processes[i]);
        }
    }

    
    if (Processes.size() > initialProcessCount) {
        Processes.erase(Processes.begin() + initialProcessCount, Processes.end());
    }
}

TEST_CASE("Process handles management") {
    size_t initialCount = Processes.size();

    SUBCASE("Multiple process creation") {
        wstring systemDir(MAX_PATH, L'\0');
        GetSystemDirectoryW(&systemDir[0], MAX_PATH);
        systemDir.resize(wcslen(systemDir.c_str()));
        wstring cmdPath = systemDir + L"\\cmd.exe";

        if (FileExists(cmdPath)) {
           
            CHECK_NOTHROW(DoProcess(cmdPath, L"/c timeout 1"));
            size_t count1 = Processes.size();
            CHECK_NOTHROW(DoProcess(cmdPath, L"/c timeout 1"));
            size_t count2 = Processes.size();

            CHECK(count1 == initialCount + 1);
            CHECK(count2 == initialCount + 2);

            
            for (size_t i = initialCount; i < Processes.size(); ++i) {
                if (Processes[i]) {
                    DWORD exitCode;
                    if (GetExitCodeProcess(Processes[i], &exitCode) && exitCode == STILL_ACTIVE) {
                        TerminateProcess(Processes[i], 0);
                        WaitForSingleObject(Processes[i], 1000);
                    }
                    CloseHandle(Processes[i]);
                }
            }
            Processes.erase(Processes.begin() + initialCount, Processes.end());
        }
        else {
            WARN("cmd.exe not found, skipping test");
        }
    }
}

TEST_CASE("Error handling") {
    SUBCASE("Invalid handle values are not added") {
        size_t initialCount = Processes.size();

       
        CHECK_THROWS(DoProcess(L"nonexistent_executable_xyz.exe"));

        
        CHECK(Processes.size() == initialCount);
    }
}