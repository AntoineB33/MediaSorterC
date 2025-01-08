#include "all_func.h"

void getPrevSort(IfAscendingParams* ifAscendingParams, cJSON* bufferJS) {
    const int fstNbItBefUpdErr = 50;

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
    HANDLE hFile = ifAscendingParams->hFile;
    OVERLAPPED ov = ifAscendingParams->ov;
    FILE* file = ifAscendingParams->file;
    char* fileContent = ifAscendingParams->fileContent;
    cJSON* json = ifAscendingParams->json;
    cJSON* waitingSheets = ifAscendingParams->waitingSheets;
    cJSON* sheetIDToBuffer = ifAscendingParams->sheetIDToBuffer;
    int* fileCleaned = ifAscendingParams->fileCleaned;

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

    if (openTxtFile(&hFile, &ov, file, fileContent, json, sort_info_path)) {
        return -1;
    }

    cJSON* threadsArray = cJSON_GetObjectItem(bufferJS, "threads");
    *nb_params = max(cJSON_GetArraySize(threadsArray), 1);
    *allParams = malloc(*nb_params * sizeof(ThreadParams));
    (*allParams)[0].fstSharedDepth = 0;

    cJSON* sharerRefsJS = cJSON_GetObjectItem(bufferJS, "sharerRefs");
    for (int i = 0; i < cJSON_GetArraySize(sharerRefsJS); i++) {
        (*allParams)[i].sharers = malloc(*n * sizeof(int*));
        for (int j = 0; j < *n; j++) {
            (*allParams)[i].sharers[j] = NULL;
        }
    }
    int i = 0;
    cJSON* thread;
    cJSON_ArrayForEach(thread, threadsArray) {
        cJSON* fstSharedDepth = cJSON_GetObjectItem(thread, "fstSharedDepth");
        (*allParams)[i].fstSharedDepth = fstSharedDepth->valueint;
        cJSON* resultJS = cJSON_GetObjectItem(thread, "result");
        (*allParams)[i].result = malloc(*n * sizeof(sharing));
        int k = 0;
        cJSON* cJSONEl;
        cJSON_ArrayForEach(cJSONEl, resultJS) {
            cJSON* resultAtt = cJSON_GetObjectItem(cJSONEl, "result");
            cJSON* reservationListInd = cJSON_GetObjectItem(cJSONEl, "reservationListInd");
            (*allParams)[i].result[k].result.result = resultAtt->valueint;
            (*allParams)[i].result[k].result.reservationListInd = reservationListInd->valueint;
            k++;
        }
        cJSON* sharersJS = cJSON_GetObjectItem(thread, "sharers");
        int array_size = cJSON_GetArraySize(sharersJS);
        (*allParams)[i].lstSharedDepth = (*allParams)[i].fstSharedDepth + array_size - 1;
        for (int k = 0; k < array_size; k++) {
            int p = k + (*allParams)[i].fstSharedDepth;
            if ((*allParams)[i].sharers[p] == NULL) {
                int m;
                (*allParams)[i].sharers[p] = &m;
                cJSON* sharerAtt = cJSON_GetArrayItem(sharersJS, k);
                cJSON* sharePointed = cJSON_GetObjectItem(sharerRefsJS, sharerAtt->valuestring);
                cJSON* oneSharer;
                cJSON_ArrayForEach(oneSharer, sharePointed) {
                    int ind = oneSharer->valueint;
                    if (ind != i) {
                        (*allParams)[ind].sharers[p] = &m;
                    }
                }
            }
        }
        i++;
    }
    cleanFile(&hFile, &ov, file, fileContent, json, *sort_info_path);
}