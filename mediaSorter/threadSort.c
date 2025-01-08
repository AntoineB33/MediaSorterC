#include "all_func.h"




void ifAscending(IfAscendingParams* ifAscendingParams) {
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

    (*depth)++;
    if (*depth == n) {
        if (updateThread(ifAscendingParams, 0, 1)) {
            return;
        }
        ascending = 0;
    }
    else {
        int nodId = (*result)[*depth - 1].nodeId;

        // ********* update the remaining elements *********

        // copy the remaining indexes before the chosen one
        for (int i = 0; i < (*rests)[*depth - 1].size; i++) {
            if (((int*)(*rests)[*depth - 1].data)[i] != nodId) {
                addElement(&((*rests)[*depth]), &((int*)(*rests)[*depth - 1].data)[i]);
            }
        }

        // ********* consequences of the choice in previous depth *********

        busyNodes[nodId]++;

        // update the reservation rules
        if ((*nodes)[nodId].nb_allowedPlaces) {
            int highest = ((allowedPlaceSt*)getLastElement(&(*nodes)[nodId].allowedPlaces))->end;
            int placeInd = highest;
            int placeIndPrev;
            do {
                placeIndPrev = placeInd;
                placeInd = (*reservationLists)[placeIndPrev].prevListInd;
            } while ((*reservationLists)[placeIndPrev].out == 0 && (placeIndPrev != highest || placeIndPrev - placeInd >= (*reservationLists)[placeIndPrev].data.size));
            placeInd++;
            if (placeIndPrev == highest) {
                if ((*reservationRules)[placeInd].all == 1) {
                    int placeInd2 = placeIndPrev - (*reservationLists)[placeIndPrev].data.size + 1;
                    (*reservationRules)[placeInd2].all = 1;
                }
                do {
                    placeIndPrev = placeInd;
                    placeInd = (*reservationLists)[placeIndPrev].prevListInd;
                } while ((*reservationLists)[placeIndPrev].out == 0);
            }
            placeInd++;
            (*reservationRules)[placeInd].resList = -1;
            (*reservationLists)[placeIndPrev].prevListInd++;
            placeInd++;
            if (placeInd <= highest && (*reservationRules)[placeInd].all == 0) {
                do {
                    placeIndPrev = placeInd;
                    placeInd = (*reservationLists)[placeInd].nextListInd;
                } while (placeInd != highest && (*reservationLists)[placeInd].data.size == placeInd - (*reservationLists)[placeInd].prevListInd);
                (*reservationLists)[placeIndPrev].nextListInd = -1;
                (*reservationLists)[placeInd].out = 1;
            }
            removeElement(&((*reservationLists)[highest].data), nodId);
        }
        for (int j = 0; j < (*nodes)[nodId].nb_ulteriors; j++) {
            int element = (*nodes)[nodId].ulteriors[j];
            (*busyNodes)[element]--;
            if ((*busyNodes)[element] == 0) {
                addElement(&(rests[*depth]), &element);
            }
        }

        // nodes that can't be chosen there but could before
        for (int i = 0; i < (*startExcl)[*depth].size; i++) {
            int element = ((int*)(*endExcl)[*depth].data)[i];
            (*busyNodes)[element]++;
            if ((*busyNodes)[element] == 1) {
                removeElement(&(rests[*depth]), element);
            }
        }

        // nodes that can be chosen there but couldn't before
        for (int i = 0; i < (*endExcl)[*depth].size; i++) {
            int element = ((int*)(*startExcl)[*depth].data)[i];
            (*busyNodes)[element]--;
            if ((*busyNodes)[element] == 0) {
                addElement(&((*rests)[*depth]), &element);
            }
        }
    }
}

void ifDescending(IfAscendingParams* ifAscendingParams) {
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

    *ascending = 1;
    (*result)[*depth].result.result = -1;
    (*depth)--;
    (*befUpdErr)--;
    reachedEnd = *depth == *fstSharedDepth;
    if ((!*befUpdErr || reachedEnd) && updateThread(ifAscendingParams, reachedEnd, 0)) {
        return;
    }
    if (n == 0) {
        return 0;
    }
    int nodId = (*result)[*depth].nodeId;
    (*nodes)[nodId].nbPost--;
    notChosenAnymore(nodes, reservationRules, reservationLists, nodId);
    for (int j = 0; j < (*nodes)[nodId].nb_ulteriors; j++) {
        int ulterior = ((int*)(*nodes)[nodId].nb_ulteriors)[j];
        (*nodes)[ulterior].nbPost++;
    }

    // what being at this depth implies
    for (int i = 0; i < (*startExcl)[*depth].size; i++) {
        (*nodes)[((int*)(*startExcl)[*depth].data)[i]].nbPost--;
    }
    for (int i = 0; i < (*endExcl)[*depth].size; i++) {
        (*nodes)[((int*)(*endExcl)[*depth].data)[i]].nbPost++;
    }
    (*depth)--;
}

// Recursive function to try all possible ways to sort the integer
DWORD WINAPI threadSort(LPVOID lpParam) {
    int ascending = 1;
    int depth = 0;
    int n;
    // Cast lpParam to the appropriate structure type
    ThreadParamsRank* threadParamsRank = (ThreadParamsRank*)lpParam;

    int rank = threadParamsRank->rank;

    MeetPoint* meetPoint = threadParamsRank->meetPoint;

    char* sort_info_path = meetPoint->sort_info_path;
    problemSt** allProblems = meetPoint->allProblems;
    int nb_allProblems = meetPoint->nb_allProblems;
	HANDLE* mainSemaphore = meetPoint->mainSemaphore;
    HANDLE* semaphore = meetPoint->semaphore;
    HANDLE* threadEvent = &meetPoint->threadEvents[rank];
    int nb_process = meetPoint->nb_process;
    int* awaitingThrds = meetPoint->awaitingThrds;
    int nb_awaitingThrds = meetPoint->nb_awaitingThrds;
    int* activeThrd = meetPoint->activeThrd;
	int error = meetPoint->error;
	ThreadParams* allParams = meetPoint->allParams;
	int nb_params = meetPoint->nb_params;

	ThreadParams* currentParam = &allParams[rank];


    int prbInd;
    char* sheetID;
    char* sheetPath;
    attributeSt* attributes;
    int nb_att;

    ThreadParams* currentParam = &allParams[rank];
    int fstSharedDepth = currentParam->fstSharedDepth - 1;
    sharing* result = currentParam->result;
    int** sharers = currentParam->sharers;

    int lastSharedDepth;
    int found = 0;
    int currentElement;
    int befUpdErr;
    int startInd = 0;

    int* const busyNodes = NULL;
    GenericList* const rests = NULL;
    ReservationRule* const reservationRules = NULL;
    ReservationList* const reservationLists = NULL;
    GenericList* startExcl = NULL;
    GenericList* endExcl = NULL;
    int* errors = NULL;
    int errorThrd;

    int errorThrd;
    int initBefUpdErr;
    int stopAll;
    int reachedEnd;

    if (updateThread(threadParamsRank, 0, 0)) {
        return;
    }
    // main loop
    while (1) {
        chooseNextOpt(threadParamsRank, &depth, &ascending);
        if (ascending == 1) {
			IfAscendingParams ifAscendingParams = {
				.threadParamsRank = threadParamsRank,
				.rests = rests,
				.startExcl = startExcl,
				.endExcl = endExcl,
				.reservationLists = reservationLists,
				.reservationRules = reservationRules,
				.busyNodes = busyNodes,
				.depth = &depth,
				.ascending = &ascending,
			};
			ifAscending(ifAscendingParams);
        }
        if (ascending == 0) {
			ifDescending(threadParamsRank, &depth, &ascending);
        }
    }
}
