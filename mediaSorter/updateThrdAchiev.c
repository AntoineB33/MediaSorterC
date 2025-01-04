#include "all_func.h"

void updateThrdAchiev(cJSON* sheetIDToBuffer, char* sheetID, char* sheetPath, int isCompleteSorting, int* error, cJSON* json, int rank, int n, sharing* result, sharingRef** sharers, int reachedEnd, int fstSharedDepth, int lstSharedDepth, int depth) {
    cJSON* sheetPathToSheetID = cJSON_GetObjectItem(json, "sheetPathToSheetID");
    cJSON* sheetIDJS = cJSON_GetObjectItem(sheetPathToSheetID, sheetPath);
    if (!sheetIDJS || strcmp(sheetIDJS->valuestring, sheetID)) {
        return;
    }
    int toUpdate = 0;
    cJSON* bufferJS = cJSON_GetObjectItemCaseSensitive(sheetIDToBuffer, sheetID);
    cJSON* toUpdateJS = cJSON_GetObjectItem(bufferJS, "toUpdate");
    toUpdate = toUpdateJS->valueint;
    if (isCompleteSorting) {
        cJSON* errorJS = cJSON_GetObjectItem(bufferJS, "error");
        cJSON_ReplaceItemInObject(bufferJS, "error", cJSON_CreateNumber(*error));
        cJSON* resultNodesArray = cJSON_GetObjectItem(bufferJS, "resultNodes");
        for (int j = 0; j < n; j++) {
            cJSON_ReplaceItemInArray(resultNodesArray, j, cJSON_CreateNumber(result[j].nodeId));
        }
        if (toUpdate) {
            // call VBA
        }
    }
    if (!*error) {
        return;
    }
    cJSON* threadsArray = cJSON_GetObjectItem(bufferJS, "threads");
    cJSON* sharerRefsJS = cJSON_GetObjectItem(bufferJS, "sharerRefs");
    char address[20];
    cJSON* thread = cJSON_CreateObject();
    cJSON* resultArray = cJSON_CreateArray();
    cJSON* sharersArray = cJSON_CreateArray();
    for (int k = 0; k < depth; k++) {
        cJSON* resultElmnt = cJSON_CreateObject();
        cJSON_AddItemToObject(resultElmnt, "result", cJSON_CreateNumber(result[k].result.result));
        cJSON_AddItemToObject(resultElmnt, "reservationListInd", cJSON_CreateNumber(result[k].result.reservationListInd));
        cJSON_AddItemToArray(resultArray, resultElmnt);
        snprintf(address, sizeof(address), "%p", (void*)sharers[k]);
        cJSON_AddItemToArray(sharersArray, cJSON_CreateString(address));
        cJSON* sharerRefs = cJSON_GetObjectItem(sharerRefsJS, address);
        if (!sharerRefs) {
            cJSON* allSharers = cJSON_CreateArray();
            cJSON_AddItemToArray(allSharers, cJSON_CreateNumber(rank));
            cJSON_AddItemToObject(sharerRefsJS, address, allSharers);
        }
        else {
            cJSON_AddItemToArray(sharerRefs, cJSON_CreateNumber(rank));
        }
    }
	cJSON_AddItemToObject(thread, "fstSharedDepth", cJSON_CreateNumber(fstSharedDepth));
	cJSON_AddArrayToObject(thread, "lstSharedDepth", cJSON_CreateNumber(lstSharedDepth));
    cJSON_AddItemToObject(thread, "resultArray", resultArray);
	cJSON_AddItemToObject(thread, "sharersArray", sharersArray);
    // remove sharerRefs
    for (int i = cJSON_GetArraySize(threadsArray) - 1; i <= rank; i++) {
        cJSON_AddItemToArray(threadsArray, cJSON_CreateNull());
    }
    // remove too much threads
    cJSON_ReplaceItemInArray(threadsArray, rank, thread);
}
