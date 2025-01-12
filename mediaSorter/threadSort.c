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
    int* reachedEnd = ifAscendingParams->reachedEnd;
    int* isCompleteSorting = ifAscendingParams->isCompleteSorting;
    int notMain = ifAscendingParams->notMain;
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
    ThreadParams** allWaitingParams = &problem->allWaitingParams;
    int* nb_params = &problem->nb_params;

    (*depth)++;
    if (*depth == n) {
		*isCompleteSorting = 1;
		*reachedEnd = 1;
		int res = updateThread(ifAscendingParams);
        *reachedEnd = 0;
        if (res) {
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
    int* reachedEnd = ifAscendingParams->reachedEnd;
    int* isCompleteSorting = ifAscendingParams->isCompleteSorting;
    int notMain = ifAscendingParams->notMain;
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
    ThreadParams** allWaitingParams = &problem->allWaitingParams;
    int* nb_params = &problem->nb_params;

    *ascending = 1;
    (*result)[*depth].result.result = -1;
    (*depth)--;
    (*befUpdErr)--;
    *reachedEnd = *depth == *fstSharedDepth;
    if ((!*befUpdErr || *reachedEnd) && updateThread(ifAscendingParams)) {
        return;
    }
    if (n == 0) {
        return 0;
    }
    int nodId = (*result)[*depth].nodeId;
    (*nodes)[nodId].nbPost--;
    notChosenAnymore(ifAscendingParams, nodId);
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
    // Cast lpParam to the appropriate structure type
    ThreadParamsRank* threadParamsRank = (ThreadParamsRank*)lpParam;

    GenericList* const rests = NULL;
    GenericList* startExcl = NULL;
    GenericList* endExcl = NULL;
    ReservationList* const reservationLists = NULL;
    ReservationRule* const reservationRules = NULL;
    int* busyNodes = NULL;
    int* errors = NULL;
	int depth = 0;
    int _ascending = 1;
	int lastSharedDepth;
	int errorThrd;
	int befUpdErr;
	int initBefUpdErr;
	int reachedEnd;
	int isCompleteSorting;

    IfAscendingParams ifAscendingParams = {
        .threadParamsRank = threadParamsRank,
        .rests = &rests,
        .startExcl = &startExcl,
        .endExcl = &endExcl,
        .reservationLists = &reservationLists,
        .reservationRules = &reservationRules,
        .busyNodes = &busyNodes,
		.errors = &errors,
        .depth = &depth,
        .ascending = &_ascending,
		.lastSharedDepth = &lastSharedDepth,
		.errorThrd = &errorThrd,
		.befUpdErr = &befUpdErr,
		.initBefUpdErr = &initBefUpdErr,
		.reachedEnd = &reachedEnd,
		.isCompleteSorting = &isCompleteSorting,
		.notMain = 0,
    };

    if (updateThread(&ifAscendingParams)) {
        return;
    }

	int* ascending = ifAscendingParams.ascending;

    // main loop
    while (1) {
        chooseNextOpt(&ifAscendingParams);
        if (*ascending) {
			ifAscending(&ifAscendingParams);
        }
        if (!*ascending) {
			ifDescending(&ifAscendingParams);
        }
    }
}
