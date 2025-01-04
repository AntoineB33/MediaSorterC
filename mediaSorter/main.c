#include "all_func.h"



inline void notChosenAnymore(Node* node, ReservationRule* reservationRules, ReservationList* reservationLists, int nodeId) {
    if (node->nb_allowedPlaces) {
        int highest = node->allowedPlaces[node->nb_allowedPlaces - 1].end;
        int resListInd = reservationRules[highest].resList;
        if (resListInd == -1) {
            reservationRules[highest].resList = highest;
            reservationRules[highest].all = 0;
            reservationLists[highest].out = 1;
            reservationLists[highest].prevListInd = highest - 1;
            reservationLists[highest].nextListInd = -1;
            addElement(&(reservationLists[highest].data), &nodeId);
        }
        else {
            int resThatWasThere = reservationRules[highest].resList;
            if (resThatWasThere != highest) {
                reservationLists[highest].prevListInd = reservationLists[resThatWasThere].prevListInd;
                reservationLists[resThatWasThere].prevListInd = highest;
                reservationLists[resThatWasThere].out = 0;
                reservationLists[highest].nextListInd = resThatWasThere;
            }
            addElement(&(reservationLists[highest].data), &nodeId);
            int placeInd = highest;
            int placeIndPrev;
            do {
                do {
                    placeIndPrev = placeInd;
                    placeInd = reservationLists[placeIndPrev].prevListInd;
                } while (reservationLists[placeIndPrev].out == 0);
                if (reservationRules[placeInd].resList == -1) {
                    break;
                }
                reservationLists[placeIndPrev].prevListInd = placeInd;
                reservationLists[placeIndPrev].out = 0;
                reservationLists[placeInd].nextListInd = placeIndPrev;
            } while (1);
            reservationRules[placeInd].resList = placeIndPrev;
            reservationRules[placeInd].all = (placeIndPrev != highest);
        }
    }
}

inline int check_conditions(sharing* result, Node* node, int depth) {
	int is_true = 1;
    char* condition;
	for (int condInd = 0; condInd < node->nb_conditions; condInd++) {
		condition = node->conditions[condInd];
        is_true = 1;
        if (!is_true) {
            break;
        }
	}
    for (int i = 0; i < node->nb_groups; i++) {
        is_true = check_conditions(result, node->groups[i], depth);
		if (!is_true) {
			break;
		}
    }
	return is_true;
}

inline int getNodes(char* sheetPath, Node* nodes, attributeSt* attributes, int* n, int *nb_att) {
	const char* infoFolder = "C:/Users/abarb/Documents/health/news_underground/mediaSorter/programs/data/info";


    // Allocate memory for the full file path
    size_t filePathSize = strlen(infoFolder) + strlen(sheetPath) + strlen(".txt") + 1;
    char* filePath = (char*)malloc(filePathSize);

    if (!filePath) {
        printf("Failed to allocate memory for file path.\n");
        return -1; // Memory allocation error
    }

    // Construct the full file path
    snprintf(filePath, filePathSize, "%s%s.txt", infoFolder, sheetPath);
    

    HANDLE hFile;
    OVERLAPPED ov;
    FILE* file;
    char* fileContent;
    cJSON* json;
    if (openTxtFile(&hFile, &ov, file, fileContent, json, filePath)) {
        return -1;
    }


    cJSON* nodesArray = cJSON_GetObjectItem(json, "nodes");
    *n = cJSON_GetArraySize(nodesArray);
	nodes = malloc(*n * sizeof(Node));
    int i = 0;
    cJSON* node;
    cJSON_ArrayForEach(node, nodesArray) {
		cJSON* ulteriorsArray = cJSON_GetObjectItem(node, "ulteriors");
        nodes[i].nb_ulteriors = cJSON_GetArraySize(ulteriorsArray);
        nodes[i].ulteriors = malloc(nodes[i].nb_ulteriors * sizeof(int));
		cJSON* cJSONEl;
		cJSON_ArrayForEach(cJSONEl, ulteriorsArray) {
			nodes[i].ulteriors[i] = cJSONEl->valueint;
		}
		cJSON* conditionsArray = cJSON_GetObjectItem(node, "conditions");
        nodes[i].nb_conditions = cJSON_GetArraySize(conditionsArray);
		nodes[i].conditions = malloc(nodes[i].nb_conditions * sizeof(char*));
		cJSON_ArrayForEach(cJSONEl, conditionsArray) {
			nodes[i].conditions[i] = cJSONEl->valuestring;
		}
		cJSON* groupsArray = cJSON_GetObjectItem(node, "groups");
		nodes[i].nb_groups = cJSON_GetArraySize(groupsArray);
		nodes[i].groups = malloc(nodes[i].nb_groups * sizeof(Node*));
		cJSON_ArrayForEach(cJSONEl, groupsArray) {
			nodes[i].groups[i] = nodes + cJSONEl->valueint;
		}
		cJSON* allowedPlacesArray = cJSON_GetObjectItem(node, "allowedPlaces");
		nodes[i].nb_allowedPlaces = cJSON_GetArraySize(allowedPlacesArray);
		nodes[i].allowedPlaces = malloc(nodes[i].nb_allowedPlaces * sizeof(allowedPlaceSt));
		cJSON_ArrayForEach(cJSONEl, allowedPlacesArray) {
			nodes[i].allowedPlaces[i].start = cJSON_GetObjectItem(cJSONEl, "start")->valueint;
			nodes[i].allowedPlaces[i].end = cJSON_GetObjectItem(cJSONEl, "end")->valueint;
		}
		cJSON* errorFunctionsArray = cJSON_GetObjectItem(node, "errorFunctions");
		nodes[i].nb_errorFunctions = cJSON_GetArraySize(errorFunctionsArray);
		nodes[i].errorFunctions = malloc(nodes[i].nb_errorFunctions * sizeof(char*));
		cJSON_ArrayForEach(cJSONEl, errorFunctionsArray) {
			nodes[i].errorFunctions[i] = cJSONEl->valuestring;
		}
		nodes[i].nbPost = cJSON_GetObjectItem(node, "nbPost")->valueint;
		i++;
    }

	cJSON* attributesArray = cJSON_GetObjectItem(json, "attributes");
	*nb_att = cJSON_GetArraySize(attributesArray);
	attributes = malloc(*nb_att * sizeof(attributeSt));
	int i = 0;
	cJSON* attribute;
	cJSON_ArrayForEach(attribute, attributesArray) {
		attributes[i].lastPlace = -1;
		attributes[i].sep = cJSON_GetObjectItem(attribute, "sep")->valueint;
		i++;
	}

	cleanFile(&hFile, &ov, file, fileContent, json, filePath);
	return 0;
}

inline int openTxtFile(HANDLE* hFile, OVERLAPPED* ov, FILE* file, char* fileContent, cJSON* json, char* file_path) {
    if (lockFile(hFile, ov, file_path)) {
        return -1;
    }

    // Open the file for reading
    file = fopen(file_path, "r");
    if (!file) {
        printf("Failed to open file for reading: %s\n", file_path);
        return -1;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    // Read file content into a buffer
    fileContent = (char*)malloc(fileSize + 1);
    if (!fileContent) {
        printf("Failed to allocate memory for file content.\n");
        return -1; // Memory allocation error
    }
    fread(fileContent, 1, fileSize, file);
    fileContent[fileSize] = '\0'; // Null-terminate the JSON string
    fclose(file);

    // Parse JSON content
    json = cJSON_Parse(fileContent);
    if (!json) {
        printf("Failed to parse JSON: %s\n", cJSON_GetErrorPtr());
        return -1; // JSON parsing error
    }

    return 0;
}

inline void cleanFile(HANDLE* hFile, OVERLAPPED* ov, FILE* file, char* fileContent, cJSON* json, const char* file_path) {
    if (json) {
        // Serialize the JSON object to a string
        char* updatedContent = cJSON_Print(json);
        if (!updatedContent) {
            printf("Failed to serialize JSON.\n");
        }
        else {
            // Reopen the file in write mode to update it
            file = fopen(file_path, "w");
            if (!file) {
                printf("Failed to open file for writing: %s\n", file_path);
            }
            else {
                fputs(updatedContent, file);
                fclose(file);
            }
            free(updatedContent);
        }
    }

    // Cleanup JSON object
    if (json) {
        cJSON_Delete(json);
    }

    // Cleanup file content buffer
    if (fileContent) {
        free(fileContent);
    }

    // Unlock file if previously locked
    if (hFile && ov) {
        unlockFile(hFile, ov);
    }
}

void chg_pb_pointers(int* prbInd, problemSt** allProblems, int* nb_allProblems, Node** nodes, attributeSt** attributes, int* n, int** nb_att, char** sheetPath, char* sort_info_path, int* nb_process, ThreadParams* allParams, int* initBefUpdErr, int rank, ThreadParams* paramP, sharing** result, int* fstSharedDepth, int** busyNodes, GenericList** rests, ReservationList** reservationLists, ReservationRule** reservationRules, GenericList** startExcl, GenericList** endExcl, int** errors, int* depth) {
    *nodes = &(*allProblems)[*prbInd].nodes;
    *attributes = &(*allProblems)[*prbInd].attributes;
    *n = (*allProblems)[*prbInd].n;
    *nb_att = &(*allProblems)[*prbInd].nb_att;
    reallocVar(busyNodes, *n, &rests, &reservationLists, &reservationRules, startExcl, endExcl, errors, *nodes, result, depth);
}

void chkIfSameSheetID(HANDLE* hFile, OVERLAPPED* ov, FILE* file, char* fileContent, cJSON* json, char* file_path, int reachedEnd, int* fileCleaned, cJSON* waitingSheets, char** sheetID, cJSON* sheetIDToBuffer, int* prbInd, problemSt** allProblems, int* nb_allProblems, Node** nodes, attributeSt** attributes, int* n, int** nb_att, char** sheetPath, char* sort_info_path, int* nb_process, ThreadParams* allParams, int* initBefUpdErr, int* activeThrd, int rank, ThreadParams* paramP, sharing** result, sharingRef*** sharers, int* fstSharedDepth, int** busyNodes, GenericList** rests, ReservationList** reservationLists, ReservationRule** reservationRules, GenericList** startExcl, GenericList** endExcl, int** errors, int* depth, HANDLE* semaphore, int* nb_awaitingThrds, int* awaitingThrds, int* lastSharedDepth, int* befUpdErr, int* errorThrd, int* error, int* stopAll, ThreadEvent* threadEvent) {
    const int fstNbItBefUpdErr = 50;


    if (!cJSON_GetArraySize(waitingSheets) || cJSON_GetObjectItem(json, "stop")->valueint) {
		activeThrd[rank] = 0;
		*n = 0;
    }

    cJSON* waitingSheet = cJSON_GetArrayItem(waitingSheets, 0);
    int diff_sheet = !*sheetID || strcmp(waitingSheet->valuestring, *sheetID);
    if (!*sheetID || diff_sheet || !*n || reachedEnd) {
        if (*sheetID && diff_sheet) {
            (*allProblems)[*prbInd].nb_process_on_it--;
            if (!(*allProblems)[*prbInd].nb_process_on_it) {
                // Remove a Problem (because this thread was the last one to work on it)
                for (int i = 0; i < (*allProblems)[*prbInd].nb_params; i++) {
                    free((*allProblems)[*prbInd].allParams[i].result);
                    free((*allProblems)[*prbInd].allParams[i].sharers);
                }
                free((*allProblems)[*prbInd].allParams);
                removeInd(*allProblems, nb_allProblems, *prbInd, sizeof(problemSt));
				for (int i = *prbInd; i < *nb_allProblems; i++) {
					(*allProblems)[i].prbInd--;
				}
            }
        }
        if (!*n) {
            return;
        }
        int fstThrdOnPb = 0;
        if (diff_sheet) {
            int found = 0;
            *sheetID = realloc(*sheetID, strlen(waitingSheet->valuestring) + 1);
            strcpy(*sheetID, waitingSheet->valuestring);
            for (int i = 0; i < *nb_allProblems; i++) {
                if (strcmp((*allProblems)[i].sheetID, *sheetID)) {
                    *prbInd = i;
                    found = 1;
                    break;
                }
            }
            if (!found) {
                *prbInd = *nb_allProblems;
                *allProblems = realloc(allProblems, (*nb_allProblems - 1) * sizeof(problemSt));
				(*allProblems)[*prbInd].prbInd = *prbInd;
                cJSON* bufferJS = cJSON_GetObjectItemCaseSensitive(sheetIDToBuffer, *sheetID);
                *sheetPath = cJSON_GetObjectItem(bufferJS, "sheetPath")->valuestring;
                getNodes(*sheetPath, (*allProblems)[*nb_allProblems].nodes, (*allProblems)[*nb_allProblems].attributes, &(*allProblems)[*nb_allProblems].n, &(*allProblems)[*nb_allProblems].nb_att);
                *nb_allProblems++;
                getPrevSort(sort_info_path, bufferJS, &(*allProblems)[*prbInd].nb_params, (*allProblems)[*prbInd].allParams);
            }
            (*allProblems)[*prbInd].nb_process_on_it++;
            *nodes = &(*allProblems)[*prbInd].nodes;
            *attributes = &(*allProblems)[*prbInd].attributes;
            *n = (*allProblems)[*prbInd].n;
            *nb_att = &(*allProblems)[*prbInd].nb_att;
            reallocVar(busyNodes, *n, &rests, &reservationLists, &reservationRules, startExcl, endExcl, errors, *nodes, result, depth);
            *initBefUpdErr = fstNbItBefUpdErr;
        }
        int paramInd = (*allProblems)[*prbInd].nb_params - 1;
        if (paramInd == -1) {
            cleanFile(hFile, ov, file, fileContent, json, file_path);
			waitForJob(semaphore, allParams[rank].fstSharedDepth, nb_awaitingThrds, awaitingThrds, rank, threadEvent, lastSharedDepth, sort_info_path, sheetID, 0, befUpdErr, errorThrd, error, n, allParams, result, sharers, nb_process, nodes, stopAll, sheetPath, initBefUpdErr, prbInd, allProblems, nb_allProblems, attributes, nb_att, errors, 1, awaitingThrds, waitingSheets, sheetIDToBuffer, paramP, fstSharedDepth, busyNodes, rests, reservationLists, reservationRules, startExcl, endExcl, depth);
			*fileCleaned = 1;
        }
        else {
			*result = (*allProblems)[*prbInd].allParams[paramInd].result;
			*sharers = (*allProblems)[*prbInd].allParams[paramInd].sharers;
            *fstSharedDepth = (*allProblems)[*prbInd].allParams[paramInd].fstSharedDepth;
            (*nb_allProblems)--;
            *allProblems = realloc(allProblems, *nb_allProblems * sizeof(problemSt));
        }
    }
}

// Function to search and modify the content in the file
inline int modifyFile(int reachedEnd, char* sort_info_path, char** sheetID, int isCompleteSorting, HANDLE* semaphore, int* befUpdErr, int* errorThrd, int* error, int* nb_awaitingThrds, int* n, ThreadParams* allParams, sharing* result, int rank, sharerResult* sharers, int* nb_process, ThreadEvent* threadEvents, Node** nodes, int* stopAll, int** activeThrd, char** sheetPath, int* initBefUpdErr, int* prbInd, problemSt** allProblems, int* nb_allProblems, attributeSt** attributes, int* nb_att, int* errors, int notMain, int* awaitingThrds, ThreadParams* paramP, int* fstSharedDepth, int** busyNodes, GenericList** rests, ReservationList** reservationLists, ReservationRule** reservationRules, GenericList** startExcl, GenericList** endExcl, int* depth, ThreadEvent* threadEvent, int* lastSharedDepth, MeetPoint* meetPoint) {
    HANDLE hFile;
    OVERLAPPED ov;
    FILE* file;
    char* fileContent;
    cJSON* json;
	if (openTxtFile(&hFile, &ov, file, fileContent, json, sort_info_path)) {
		return -1;
	}

    cJSON* waitingSheets = cJSON_GetObjectItem(json, "waitingSheets");
    cJSON* sheetIDToBuffer = cJSON_GetObjectItem(json, "sheetIDToBuffer");


    if (notMain && *error) {
        updateBefUpd(errorThrd, errors, n, error, initBefUpdErr, semaphore, befUpdErr, nb_awaitingThrds);
        if (*sheetID) {
            updateThrdAchiev(sheetIDToBuffer, *sheetID, *sheetPath, isCompleteSorting, error, json, rank, *n, result, sharers, reachedEnd);
        }
    }
    cJSON* nb_processJS = cJSON_GetObjectItem(json, "nb_process");
    int new_nb_process = nb_processJS->valueint;
    int new_nb_process_temp = *nb_process;
    int fileCleaned = 0;
    if (notMain) {
        chkIfSameSheetID(&hFile, &ov, file, fileContent, json, reachedEnd, &fileCleaned, waitingSheets, sheetID, sheetIDToBuffer, prbInd, allProblems, nb_allProblems, nodes, attributes, n, nb_att, sheetPath, sort_info_path, nb_process, allParams, initBefUpdErr, *activeThrd, rank, paramP, result, sharers, fstSharedDepth, busyNodes, rests, reservationLists, reservationRules, startExcl, endExcl, errors, depth, semaphore, nb_awaitingThrds, awaitingThrds, threadEvents, lastSharedDepth, befUpdErr, errorThrd, error, stopAll, threadEvent);
		if (!*n) {
            new_nb_process = 0;
		}
    }

    *stopAll = 0;
    if (new_nb_process < *nb_process) {
        if (rank >= new_nb_process) {
            *stopAll = 2;
            if (rank == *nb_process - 1) {
				new_nb_process_temp = new_nb_process;
                for (int i = *nb_process - 2; i >= new_nb_process; i--) {
                    if ((*activeThrd)[i]) {
                        new_nb_process_temp = i + 1;
                        break;
                    }
                    else {
                        CloseHandle(threadEvents[i]);
						free(allParams[i].result);
						free(allParams[i].sharers);
                    }
                }
            }
            for (int i = 0; i < *nb_awaitingThrds; i++) {
                if (awaitingThrds[i] >= new_nb_process) {
                    SetEvent(threadEvents[rank]);
					removeInd(awaitingThrds, nb_awaitingThrds, i, sizeof(int));
				}
			}
		}
	}
    chg_nb_process(nb_process, allParams, threadEvents, new_nb_process_temp, *activeThrd, meetPoint, rank);

    cleanFile(&hFile, &ov, file, fileContent, json);

    return 0;
}

inline void chooseWithResRules(sharing* result, int depth, ReservationRule* reservationRules, ReservationList* reservationLists, int* busyNodes, Node* nodes, int* ascending) {
    int placeInd;
    int choiceInd;
    if (result[depth].result.result == -1) {
        placeInd = reservationRules[depth].resList;
        choiceInd = 0;
    }
    else {
        placeInd = result[depth].result.reservationListInd;
        choiceInd = result[depth].result.result + 1;
    }
    result[depth].result.result = -1;
    int found = 0;
    do {
        for (int i = choiceInd; i < reservationLists[placeInd].data.size; i++) {
            result[depth].nodeId = ((int*)reservationLists[placeInd].data.data)[i];
            if (busyNodes[result[depth].nodeId] == 0 && check_conditions(result, &nodes[i], i)) {
                result[depth].result.reservationListInd = placeInd;
                result[depth].result.result = i;
                found = 1;
                break;
            }
        }
        if (!found) {
            if (!reservationRules[depth].all) {
                break;
            }
            choiceInd = 0;
            placeInd = reservationLists[placeInd].nextListInd;
        }
    } while (!found && placeInd != -1);
    if (!found) {
        *ascending = 0;
    }
}

inline void updateErrors(sharing* result, Node* node, int* errors, int depth) {
    for (int condInd = 0; condInd < node->nb_errorFunctions; condInd++) {
        errors[depth] += 1;
    }
    for (int i = 0; i < node->nb_groups; i++) {
        updateErrors(result, node->groups[i], errors, depth);
    }
}

void chooseNextOpt(int depth, int* lastSharedDepth, HANDLE* semaphore, sharing* result, sharerResult* sharers, ReservationRule* reservationRules, ReservationList* reservationLists, GenericList* rests, int* ascending, Node* nodes, int* busyNodes, int* errors, ThreadParams* allParams) {


    // if the tree is shared with another thread at this depth
    if (depth == *lastSharedDepth) {
        WaitForSingleObject(semaphore, INFINITE); // Acquire the semaphore
        result[depth].result = sharers[depth].result;
    }

    if (reservationRules[depth].resList == -1) {
        do {
            result[depth].result.result++;
            if (result[depth].result.result == rests[depth].size) {
                *ascending = 0;
                break;
            }
            result[depth].nodeId = ((int*)rests[depth].data)[result[depth].result.result];
			if (check_conditions(result, &nodes[result[depth].nodeId], depth)) {
                break;
            }
        } while (1);
    }
    else {
		chooseWithResRules(result, depth, reservationRules, reservationLists, busyNodes, nodes, ascending);
    }

	updateErrors(result, &nodes[result[depth].nodeId], errors, depth);

    // if the tree is shared with another thread at this depth
    if (depth == *lastSharedDepth) {
        if (*ascending) {
            *sharers[depth] = result[depth].result;
        }
        else {
            (*lastSharedDepth)--;
        }
        ReleaseSemaphore(semaphore, 1, NULL); // Release the semaphore
    }
}

inline int updateBefUpd(int* errorThrd, int* errors, int n, int* error, int* initBefUpdErr, HANDLE* semaphore, int* befUpdErr, int* nb_awaitingThrds) {
    const int incremBefUpdErr = 50;
	const int initBefUpdErrMax = 10000;
	const int initBefUpdErrMin = 50;
	const int leastBefUpdErrToUpd = 30;


    if (*errorThrd != *error || *nb_awaitingThrds > 0) {
        if (initBefUpdErr - incremBefUpdErr >= initBefUpdErrMin) {
            initBefUpdErr -= incremBefUpdErr;
        }
	}
	else if(!*befUpdErr < leastBefUpdErrToUpd && initBefUpdErr + incremBefUpdErr <= initBefUpdErrMax) {
        initBefUpdErr += incremBefUpdErr;
	}
	*befUpdErr = initBefUpdErr;
    if (errors[n - 1] < *error) {
        *error = errors[n - 1];
    }
    errorThrd = *error;
}

inline int updateThread(int reachedEnd, char* sort_info_path, char** sheetID, int isCompleteSorting, HANDLE* semaphore, int* befUpdErr, int* errorThrd, int* depth, int* lastSharedDepth, ReservationList* reservationLists, ReservationRule* reservationRules, int* busyNodes, int* ascending, int* error, int* nb_awaitingThrds, int* n, int* awaitingThrds, ThreadParams* allParams, sharing* result, int rank, sharerResult* sharers, int* nb_process, ThreadEvent* threadEvents, GenericList* rests, Node* nodes, int** activeThrd, char** sheetPath, int* initBefUpdErr, int prbInd, problemSt** allProblems, int* nb_allProblems, attributeSt** attributes, int* nb_att, problemSt* problem, GenericList** startExcl, GenericList** endExcl, int** errors, int* stopAll, ThreadParams* paramP, int* fstSharedDepth, ThreadEvent* threadEvent, MeetPoint* meetPoint) {
    WaitForSingleObject(semaphore, INFINITE); // Acquire the semaphore
    int stopAll;
    modifyFile(reachedEnd, sort_info_path, sheetID, isCompleteSorting, semaphore, befUpdErr, errorThrd, error, nb_awaitingThrds, n, allParams, result, rank, sharers, nb_process, threadEvents, nodes, stopAll, activeThrd, sheetPath, initBefUpdErr, prbInd, allProblems, nb_allProblems, attributes, nb_att, errors, 1, awaitingThrds, paramP, fstSharedDepth, busyNodes, rests, reservationLists, reservationRules, startExcl, endExcl, depth, threadEvent, lastSharedDepth, meetPoint);
    if (stopAll) {
        if (stopAll == 2) {
            ReleaseSemaphore(semaphore, 1, NULL); // Release the semaphore
            return 1;
        }
        *depth = 0;
    }
    if (*nb_awaitingThrds > 0 && *lastSharedDepth < n - 2) {
		giveJob(semaphore, awaitingThrds, nb_awaitingThrds, lastSharedDepth, allParams, result, rank, *sheetID, problem, sharers, threadEvents, &busyNodes, n, &rests, &reservationLists, &reservationRules, startExcl, endExcl, errors, nodes);
    }
    else {
        ReleaseSemaphore(semaphore, 1, NULL);
    }
	return 0;
}

inline int reallocVar(int** busyNodes, int n, GenericList** rests, ReservationList** reservationLists, ReservationRule** reservationRules, GenericList** startExcl, GenericList** endExcl, int** errors, Node* nodes, sharing** result, int* depth) {
	depth = 0;
    *busyNodes = malloc(n * sizeof(int));
    *rests = malloc(n * sizeof(GenericList));
    *reservationRules = malloc((n + 1) * sizeof(ReservationRule));
    *reservationLists = malloc((n - 1) * sizeof(ReservationList));
    *startExcl = malloc(n * sizeof(GenericList));
    *endExcl = malloc(n * sizeof(GenericList));
    *errors = malloc(n * sizeof(int));
    (*errors)[0] = 0;
    for (int i = 0; i < n + 1; i++) {
        (*busyNodes)[i] = nodes[i].nbPost;
        initList(&(*rests)[i], sizeof(int), 1);
        (*reservationRules)[i].resList = -1;
    }
    *result = malloc(n * sizeof(sharing));
    for (int i = 0; i < n; i++) {
        (*result)[i].result.result = -1;
        Node* node = &nodes[i];
        for (int j = 0; j < node->nb_allowedPlaces; i++) {
            if (node->allowedPlaces[j].start != 0) {
                addElement(&(*startExcl)[node->allowedPlaces[j].start], i);
            }
            else if (j == 0) {
                (*busyNodes)[i]++;
            }
            addElement(&(*endExcl)[node->allowedPlaces[j].end], i);
        }
        if ((*busyNodes)[i] == 0 && check_conditions(nodes, node, i)) {
            addElement(&((*rests)[0]), &i);
        }
    }
    for (int i = 0; i < n - 1; i++) {
        initList(&((*reservationLists)[i].data), sizeof(int), 1);
		notChosenAnymore(&nodes[i], *reservationRules, *reservationLists, i);
    }
}

inline void waitForJob(HANDLE* semaphore, int* fstSharedDepth, int* nb_awaitingThrds, int* awaitingThrds, int rank, ThreadEvent* threadEvent, int* lastSharedDepth, char* sort_info_path, char** sheetID, int isCompleteSorting, int* befUpdErr, int* errorThrd, int* error, int* n, ThreadParams* allParams, sharing* result, sharerResult* sharers, int* nb_process, Node** nodes, int* stopAll, char** sheetPath, int* initBefUpdErr, int* prbInd, problemSt** allProblems, int* nb_allProblems, attributeSt** attributes, int* nb_att, int* errors, int notMain, ThreadParams* paramP, int** busyNodes, GenericList** rests, ReservationList** reservationLists, ReservationRule** reservationRules, GenericList** startExcl, GenericList** endExcl, int* depth, ThreadEvent* threadEvents) {
    (*nb_awaitingThrds)++;
    awaitingThrds = realloc(awaitingThrds, *nb_awaitingThrds * sizeof(int));
    awaitingThrds[*nb_awaitingThrds] = rank;
    ReleaseSemaphore(semaphore, 1, NULL);
    WaitForSingleObject(threadEvent->eventHandlesThd, INFINITE);
    *lastSharedDepth = *fstSharedDepth;
	WaitForSingleObject(semaphore, INFINITE);
}

inline void chg_nb_process(int* nb_process, ThreadParams* allParams, HANDLE* threadEvents, int new_nb_process, int** activeThrd, MeetPoint* meetPoint, int rank) {
    *activeThrd = realloc(*activeThrd, new_nb_process * sizeof(int));
    for (int i = *nb_process; i < new_nb_process; i++) {
        (*activeThrd)[i] = 0;
    }
    allParams = realloc(allParams, new_nb_process * sizeof(ThreadParams));
    threadEvents = realloc(threadEvents, new_nb_process * sizeof(HANDLE));
	*nb_process = new_nb_process;
    for (int i = 0; i < new_nb_process; i++) {
        if (!(*activeThrd)[i]) {
            (*activeThrd)[i] = 1;
            ThreadParamsRank threadParamsRank = {
                .rank = i,
                .meetPoint = meetPoint,
            };
            threadEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
            CloseHandle(CreateThread(NULL, 0, threadSort, &allParams[i], 0, NULL));
        }
    }
}

inline int giveJob(HANDLE* semaphore, int* awaitingThrds, int* nb_awaitingThrds, int* lastSharedDepth, ThreadParams* allParams, sharing* result, int rank, char* sheetID, problemSt* problem, int** sharers, HANDLE* threadEvents, int** busyNodes, int n, GenericList** rests, ReservationList** reservationLists, ReservationRule** reservationRules, GenericList** startExcl, GenericList** endExcl, int** errors, Node* nodes) {
    (*nb_awaitingThrds)--;
    awaitingThrds = realloc(awaitingThrds, *nb_awaitingThrds * sizeof(int));
	problem->allParams = realloc(problem->allParams, (problem->nb_params + 1) * sizeof(ThreadParams));
    ThreadParams* otherParams = &problem->allParams[problem->nb_params];
	problem->nb_params++;
    otherParams->fstSharedDepth = ++(*lastSharedDepth);
    otherParams->sharer = rank;
    if (*lastSharedDepth > -1) {
		otherParams->result = malloc(*lastSharedDepth * sizeof(sharing));
        sharers[*lastSharedDepth].sharer = otherRank;
        sharers[*lastSharedDepth].result = result[*lastSharedDepth].result;
    }
    ReleaseSemaphore(semaphore, 1, NULL);
    SetEvent(threadEvents[awaitingThrds[*nb_awaitingThrds]]);
}

int main(int argc, char* argv[]) {
    const char* sort_info_path = "C:/Users/abarb/Documents/health/news_underground/mediaSorter/programs/data/sort_info.txt";

    /*
typedef struct {
    const char* sort_info_path;
    problemSt* allProblems;
    int nb_allProblems;
    HANDLE* mainSemaphore;
    HANDLE* semaphore;
    ThreadEvent* threadEvents;
    int nb_process;
    int* awaitingThrds;
    int nb_awaitingThrds;
    int* activeThrd;
    int error;
    ThreadParams* allParams;
    int nb_params;
} MeetPoint;*/

    MeetPoint meetPoint = {
        .sort_info_path = sort_info_path,
        .mainSemaphore = CreateSemaphore(NULL, 1, 1, NULL),
        .semaphore = CreateSemaphore(NULL, 1, 1, NULL),
        .threadEvents = NULL,
        .nb_process = 0,
        .allParams = NULL,
        .awaitingThrds = 0,
        .nb_awaitingThrds = 0,
        .activeThrd = NULL,
        .nb_allProblems = 0,
        .allProblems = NULL,
        .error = INT_MAX,
    };

	// Create a new thread
	int nb_process = 0;
	int stopAll;
	int* activeThrd = NULL;
	int nb_allProblems = 0;
    modifyFile(0, sort_info_path, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &nb_process, &meetPoint.threadEvents, NULL, &stopAll, *activeThrd, NULL, NULL, NULL, NULL, &nb_allProblems, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &meetPoint);

	WaitForSingleObject(meetPoint.mainSemaphore, INFINITE);

    return 0;
}
