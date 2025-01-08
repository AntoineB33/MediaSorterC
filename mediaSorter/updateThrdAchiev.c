#include "all_func.h"

void updateThrdAchiev(IfAscendingParams* ifAscendingParams, cJSON* json, cJSON* sheetIDToBuffer) {
    ThreadParamsRank* threadParamsRank = ifAscendingParams->threadParamsRank;
    GenericList** rests = ifAscendingParams->rests;
    GenericList** startExcl = ifAscendingParams->startExcl;
    GenericList** endExcl = ifAscendingParams->endExcl;
    ReservationList** reservationLists = ifAscendingParams->reservationLists;
    ReservationRule** reservationRules = ifAscendingParams->reservationRules;
    int** busyNodes = ifAscendingParams->busyNodes;
    int** errors = ifAscendingParams->errors;
    int* depth = ifAscendingParams->depth;
    int* ascending = ifAscendingParams->ascending;
    int* lastSharedDepth = ifAscendingParams->lastSharedDepth;
    int* errorThrd = ifAscendingParams->errorThrd;
    int* befUpdErr = ifAscendingParams->befUpdErr;
    int* initBefUpdErr = ifAscendingParams->initBefUpdErr;
    int notMain = ifAscendingParams->notMain;
    int reachedEnd = ifAscendingParams->reachedEnd;
    int isCompleteSorting = ifAscendingParams->isCompleteSorting;

    int rank = threadParamsRank->rank;
    MeetPoint* meetPoint = threadParamsRank->meetPoint;

    char** sort_info_path = &meetPoint->sort_info_path;
    problemSt** allProblems = &meetPoint->allProblems;
    int* nb_allProblems = &meetPoint->nb_allProblems;
    HANDLE* mainSemaphore = meetPoint->mainSemaphore;
    HANDLE* semaphore = meetPoint->semaphore;
    HANDLE*** threadEvents = &meetPoint->threadEvents;
    int* nb_process = &meetPoint->nb_process;
    int** awaitingThrds = &meetPoint->awaitingThrds;
    int* nb_awaitingThrds = &meetPoint->nb_awaitingThrds;
    int** activeThrd = &meetPoint->activeThrd;
    int* error = &meetPoint->error;
    ThreadParams** allParams = &meetPoint->allParams;
    int* nb_params = &meetPoint->nb_params;

    ThreadParams* currentParam = allParams[rank];
    problemSt* problem = currentParam->problem;
    int* fstSharedDepth = &currentParam->fstSharedDepth;
    int* lstSharedDepth = &currentParam->lstSharedDepth;
    sharing** result = currentParam->result;
    sharingRef*** sharers = currentParam->sharers;

    char** sheetPath = &problem->sheetPath;
    char** sheetID = &problem->sheetID;
    Node** nodes = &problem->nodes;
    int* n = &problem->n;
    attributeSt** attributes = &problem->attributes;
    int* nb_att = &problem->nb_att;
    int* prbInd = &problem->prbInd;
    ThreadParams** allParams = &problem->allParams;
    int* nb_params = &problem->nb_params;
    int* prbInd = &problem->prbInd;

    cJSON* sheetPathToSheetID = cJSON_GetObjectItem(json, "sheetPathToSheetID");
    cJSON* sheetIDJS = cJSON_GetObjectItem(sheetPathToSheetID, *sheetPath);
    if (!sheetIDJS || strcmp(sheetIDJS->valuestring, *sheetID)) {
        return;
    }
    int toUpdate = 0;
    cJSON* bufferJS = cJSON_GetObjectItemCaseSensitive(sheetIDToBuffer, *sheetID);
    cJSON* toUpdateJS = cJSON_GetObjectItem(bufferJS, "toUpdate");
    toUpdate = toUpdateJS->valueint;
    if (isCompleteSorting) {
        cJSON* errorJS = cJSON_GetObjectItem(bufferJS, "error");
        cJSON_ReplaceItemInObject(bufferJS, "error", cJSON_CreateNumber(*error));
        cJSON* resultNodesArray = cJSON_GetObjectItem(bufferJS, "resultNodes");
        for (int j = 0; j < *n; j++) {
            cJSON_ReplaceItemInArray(resultNodesArray, j, cJSON_CreateNumber((*result)[j].nodeId));
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
    for (int k = 0; k < *depth; k++) {
        cJSON* resultElmnt = cJSON_CreateObject();
        cJSON_AddItemToObject(resultElmnt, "result", cJSON_CreateNumber((*result)[k].result.result));
        cJSON_AddItemToObject(resultElmnt, "reservationListInd", cJSON_CreateNumber((*result)[k].result.reservationListInd));
        cJSON_AddItemToArray(resultArray, resultElmnt);
        snprintf(address, sizeof(address), "%p", (void*)(*sharers)[k]);
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
	cJSON_AddItemToObject(thread, "fstSharedDepth", cJSON_CreateNumber(*fstSharedDepth));
	cJSON_AddArrayToObject(thread, "lstSharedDepth", cJSON_CreateNumber(*lstSharedDepth));
    cJSON_AddItemToObject(thread, "resultArray", resultArray);
	cJSON_AddItemToObject(thread, "sharersArray", sharersArray);
    // remove sharerRefs
    for (int i = cJSON_GetArraySize(threadsArray) - 1; i <= rank; i++) {
        cJSON_AddItemToArray(threadsArray, cJSON_CreateNull());
    }
    // remove too much threads
    cJSON_ReplaceItemInArray(threadsArray, rank, thread);
}
