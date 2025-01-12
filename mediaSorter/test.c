#include "all_func.h"

void test(HANDLE* hFile, OVERLAPPED* ov) {
    UnlockFileEx(*hFile, 0, MAXDWORD, MAXDWORD, ov);
    CloseHandle(*hFile);
}