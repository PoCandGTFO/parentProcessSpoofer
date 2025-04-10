#include <windows.h>
#include <iostream>
#include <TlHelp32.h>

HANDLE GetProcessHandleByName(const wchar_t* procName) {
    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(PROCESSENTRY32W);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return NULL;

    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (wcscmp(entry.szExeFile, procName) == 0) {
                HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
                CloseHandle(snapshot);
                return hProc;
            }
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return NULL;
}

int wmain() {
    STARTUPINFOEXW siex = { 0 };
    PROCESS_INFORMATION pi = { 0 };
    SIZE_T attrSize = 0;
    siex.StartupInfo.cb = sizeof(STARTUPINFOEXW);

    HANDLE hParent = GetProcessHandleByName(L"explorer.exe");
    if (!hParent) {
        std::wcerr << L"Could not find explorer.exe\n";
        return 1;
    }

    // Initialize attribute list
    InitializeProcThreadAttributeList(NULL, 1, 0, &attrSize);
    siex.lpAttributeList = (LPPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(
        GetProcessHeap(), 0, attrSize);
    InitializeProcThreadAttributeList(siex.lpAttributeList, 1, 0, &attrSize);
    UpdateProcThreadAttribute(siex.lpAttributeList, 0,
        PROC_THREAD_ATTRIBUTE_PARENT_PROCESS, &hParent, sizeof(HANDLE), NULL, NULL);

    if (!CreateProcessW(
        L"C:\\Windows\\System32\\notepad.exe", NULL,
        NULL, NULL, FALSE,
        EXTENDED_STARTUPINFO_PRESENT,
        NULL, NULL, &siex.StartupInfo, &pi)) {
        std::wcerr << L"CreateProcess failed: " << GetLastError() << "\n";
    }
    else {
        std::wcout << L"Process created with spoofed parent!\n";
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    DeleteProcThreadAttributeList(siex.lpAttributeList);
    HeapFree(GetProcessHeap(), 0, siex.lpAttributeList);
    CloseHandle(hParent);

    return 0;
}
