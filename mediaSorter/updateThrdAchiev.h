#pragma once
#ifdef UPDATE_THRD_ACHIEV_H
#define UPDATE_THRD_ACHIEV_H

void updateThrdAchiev(cJSON* sheetIDToBuffer, char* sheetID, char* sheetPath, int isCompleteSorting, int* error, cJSON* json, int rank, int n, sharing* result, sharingRef** sharers, int reachedEnd);

#endif