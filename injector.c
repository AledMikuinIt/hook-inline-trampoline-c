#include <windows.h>
#include <stdio.h>

int main(int argc, char *argv[]) {

    if(argc<3){
        printf("Usage: <PATH_TO_EXE> <PATH_TO_DLL>");
        return 1;
    }

    char *ExePath = argv[1];
    char *DllPath = argv[2];
    printf("Path to exe: %s\n", ExePath);
    printf("Path to dll: %s\n", DllPath);

    STARTUPINFOA si = {sizeof(si)};
    PROCESS_INFORMATION pi;



    BOOL succes = CreateProcessA(
        ExePath,
        NULL,
        NULL,
        NULL,
        FALSE,
        CREATE_SUSPENDED,
        NULL,
        NULL,
        &si,
        &pi
    );
    if(!succes) {
        printf("[ERROR] CreateProcessA, %lu\n", GetLastError());
        return 1;
    }
    printf("[PROCESS] Create Process Successful\n");


    LPVOID remoteMem = VirtualAllocEx(
        pi.hProcess,
        NULL,
        (SIZE_T)(strlen(DllPath) +1),
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
    );
    if(!remoteMem) {
        printf("[ERROR] VirutalAlloCEx, %lu\n", GetLastError());
        return 1;
    }
    printf("[MEMORY] Virtual Alloc Successful\n");

    SIZE_T bytesWritten;
    BOOL wpm = WriteProcessMemory(
        pi.hProcess,
        remoteMem,
        DllPath,
        (SIZE_T)(strlen(DllPath) +1),
        &bytesWritten
    );
    if(!wpm) {
        printf("[ERROR] WriteProcessMemory, %lu", GetLastError());
        return 1;
    }
    printf("[MEMORY] Write Process Memory Successful\n");

    HANDLE hThread = CreateRemoteThread(
        pi.hProcess,
        NULL,
        0,
        (LPTHREAD_START_ROUTINE)LoadLibraryA,
        remoteMem,
        0,
        NULL
    );
    if(!hThread) {
        printf("[ERROR] CreateRemoteThread, %lu", GetLastError());
        return 1;
    }
    printf("[THREAD] Remote Thread Created Successfully\n");
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    ResumeThread(pi.hThread);

    printf("[SUCCEED]\n");

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return 0;


}