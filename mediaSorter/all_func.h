#pragma once
#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include "common_includes.h"
#include "my_structs.h"
#include "list_func.h"
#include "getPrevSort.h"
#include "updateThrdAchiev.h"
#include "lock_unlock_files.h"
#include "threadSort.h"

#define max(a, b) ((a) > (b) ? (a) : (b))

void notChosenAnymore(IfAscendingParams* ifAscendingParams, int nodeId);
int check_conditions(IfAscendingParams* ifAscendingParams, Node* node);
int getNodes(IfAscendingParams* ifAscendingParams);
int openTxtFile(HANDLE* hFile, OVERLAPPED* ov, FILE* file, char* fileContent, cJSON* json, char* file_path);
void cleanFile(HANDLE* hFile, OVERLAPPED* ov, FILE* file, char* fileContent, cJSON* json, const char* file_path);
void chkIfSameSheetID(IfAscendingParams* ifAscendingParams);
int modifyFile(IfAscendingParams* ifAscendingParams, int* stopAll);
void chooseWithResRules(IfAscendingParams* ifAscendingParams);
void updateErrors(IfAscendingParams* ifAscendingParams, Node* node);
void chooseNextOpt(IfAscendingParams* ifAscendingParams);
int updateBefUpd(IfAscendingParams* ifAscendingParams);
int updateThread(IfAscendingParams* ifAscendingParams);
int reallocVar(IfAscendingParams* ifAscendingParams);
void waitForJob(IfAscendingParams* ifAscendingParams);
void chg_nb_process(IfAscendingParams* ifAscendingParams, int new_nb_process);
int giveJob(IfAscendingParams* ifAscendingParams);
void updateThrdAchiev(IfAscendingParams* ifAscendingParams, cJSON* json, cJSON* sheetIDToBuffer);
int lockFile(HANDLE* hFile, OVERLAPPED* ov, const char* filePath);
void unlockFile(HANDLE* hFile, OVERLAPPED* ov);
void test(HANDLE* hFile, OVERLAPPED* ov);
void test2(HANDLE* hFile, OVERLAPPED* ov);

void initList(GenericList* list, size_t elemSize, size_t initialCapacity);
void* getLastElement(GenericList* list);
void addElement(GenericList* list, void* element);
void removeElement(GenericList* list, int element);
void removeInd(void** list, int* listSize, int ind, size_t elemSize);
void freeList(GenericList* list);

#endif /* STRING_UTILS_H */