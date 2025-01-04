#pragma once
#ifndef LOCK_UNLOCK_FILES_H
#define LOCK_UNLOCK_FILES_H

int lockFile(HANDLE* hFile, OVERLAPPED* ov, const char* filePath);
void unlockFile(HANDLE* hFile, OVERLAPPED* ov);

#endif