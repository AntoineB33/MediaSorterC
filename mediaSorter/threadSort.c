#include "all_func.h"




void ifAscending(ThreadParamsRank* threadParamsRank, int* depth, int* ascending) {
    depth++;
    if (depth == n) {
        if (updateThread(0, 1, threadParamsRank)) {
            return;
        }
        ascending = 0;
    }
    else {
        int nodId = result[depth - 1].nodeId;

        // ********* update the remaining elements *********

        // copy the remaining indexes before the chosen one
        for (int i = 0; i < rests[depth - 1].size; i++) {
            if (((int*)rests[depth - 1].data)[i] != nodId) {
                addElement(&(rests[depth]), &((int*)rests[depth - 1].data)[i]);
            }
        }

        // ********* consequences of the choice in previous depth *********

        busyNodes[nodId]++;

        // update the reservation rules
        if (nodes[nodId].nb_allowedPlaces) {
            int highest = ((allowedPlaceSt*)getLastElement(&nodes[nodId].allowedPlaces))->end;
            int placeInd = highest;
            int placeIndPrev;
            do {
                placeIndPrev = placeInd;
                placeInd = reservationLists[placeIndPrev].prevListInd;
            } while (reservationLists[placeIndPrev].out == 0 && (placeIndPrev != highest || placeIndPrev - placeInd >= reservationLists[placeIndPrev].data.size));
            placeInd++;
            if (placeIndPrev == highest) {
                if (reservationRules[placeInd].all == 1) {
                    int placeInd2 = placeIndPrev - reservationLists[placeIndPrev].data.size + 1;
                    reservationRules[placeInd2].all = 1;
                }
                do {
                    placeIndPrev = placeInd;
                    placeInd = reservationLists[placeIndPrev].prevListInd;
                } while (reservationLists[placeIndPrev].out == 0);
            }
            placeInd++;
            reservationRules[placeInd].resList = -1;
            reservationLists[placeIndPrev].prevListInd++;
            placeInd++;
            if (placeInd <= highest && reservationRules[placeInd].all == 0) {
                do {
                    placeIndPrev = placeInd;
                    placeInd = reservationLists[placeInd].nextListInd;
                } while (placeInd != highest && reservationLists[placeInd].data.size == placeInd - reservationLists[placeInd].prevListInd);
                reservationLists[placeIndPrev].nextListInd = -1;
                reservationLists[placeInd].out = 1;
            }
            removeElement(&(reservationLists[highest].data), nodId);
        }
        for (int j = 0; j < nodes[nodId].nb_ulteriors; j++) {
            int element = nodes[nodId].ulteriors[j];
            busyNodes[element]--;
            if (busyNodes[element] == 0) {
                addElement(&(rests[depth]), &element);
            }
        }

        // nodes that can't be chosen there but could before
        for (int i = 0; i < startExcl[depth].size; i++) {
            int element = ((int*)endExcl[depth].data)[i];
            busyNodes[element]++;
            if (busyNodes[element] == 1) {
                removeElement(&(rests[depth]), element);
            }
        }

        // nodes that can be chosen there but couldn't before
        for (int i = 0; i < endExcl[depth].size; i++) {
            int element = ((int*)startExcl[depth].data)[i];
            busyNodes[element]--;
            if (busyNodes[element] == 0) {
                addElement(&(rests[depth]), &element);
            }
        }
    }
}

void ifDescending(ThreadParamsRank* threadParamsRank) {
    ascending = 1;
    result[depth].result.result = -1;
    depth--;
    befUpdErr--;
    reachedEnd = depth == fstSharedDepth;
    if ((!befUpdErr || reachedEnd) && updateThread(reachedEnd, sort_info_path, &sheetID, 0, semaphore, &befUpdErr, &errorThrd, &depth, &lastSharedDepth, reservationLists, reservationRules, busyNodes, &ascending, &error, &nb_awaitingThrds, &n, awaitingThrds, allParams, result, rank, sharers, &nb_process, threadEvent, rests, nodes, &activeThrd, &sheetPath, &initBefUpdErr, &prbInd, allProblems, &nb_allProblems, &attributes, &nb_att, problem, startExcl, endExcl, errors, &stopAll, currentParam, &fstSharedDepth, threadEvent, meetPoint)) {
        return;
    }
    if (n == 0) {
        return 0;
    }
    int nodId = result[depth].nodeId;
    nodes[nodId].nbPost--;
    notChosenAnymore(nodes, reservationRules, reservationLists, nodId);
    for (int j = 0; j < nodes[nodId].nb_ulteriors; j++) {
        int ulterior = ((int*)nodes[nodId].nb_ulteriors)[j];
        nodes[ulterior].nbPost++;
    }

    // what being at this depth implies
    for (int i = 0; i < startExcl[depth].size; i++) {
        nodes[((int*)startExcl[depth].data)[i]].nbPost--;
    }
    for (int i = 0; i < endExcl[depth].size; i++) {
        nodes[((int*)endExcl[depth].data)[i]].nbPost++;
    }
    depth--;
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

    problemSt* problem;


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

    if (updateThread(0, 0, threadParamsRank)) {
        return;
    }
    // main loop
    while (1) {
        chooseNextOpt(threadParamsRank);
        if (ascending == 1) {
			ifAscending(threadParamsRank);
        }
        if (ascending == 0) {
			ifDescending(threadParamsRank);
        }
    }
}
