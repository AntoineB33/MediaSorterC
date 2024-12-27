#include "all_func.h"


inline int lockFile(HANDLE* hFile, OVERLAPPED* ov, const char* filePath) {
    *hFile = CreateFileA(
        filePath,
        GENERIC_READ | GENERIC_WRITE,
        0,              // No sharing
        NULL,           // Default security
        OPEN_EXISTING,  // Open an existing file
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Failed to open file. Error: %ld\n", GetLastError());
        return -1;
    }

    ZeroMemory(ov, sizeof(OVERLAPPED));

    // Keep trying to lock until successful
    while (!LockFileEx(hFile, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &ov)) {
        Sleep(1000); // Wait for 1 second before retrying
    }

    return 0;
}

inline void unlockFile(HANDLE* hFile, OVERLAPPED* ov) {
    UnlockFileEx(*hFile, 0, MAXDWORD, MAXDWORD, ov);
    CloseHandle(*hFile);
}