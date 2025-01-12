#include "all_func.h"



void notChosenAnymore(IfAscendingParams* ifAscendingParams, int nodeId) {
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

	Node* node = &(*nodes)[nodeId];
    if (node->nb_allowedPlaces) {
        int highest = node->allowedPlaces[node->nb_allowedPlaces - 1].end;
        int resListInd = (*reservationRules)[highest].resList;
        if (resListInd == -1) {
            (*reservationRules)[highest].resList = highest;
            (*reservationRules)[highest].all = 0;
            (*reservationLists)[highest].out = 1;
            (*reservationLists)[highest].prevListInd = highest - 1;
            (*reservationLists)[highest].nextListInd = -1;
            addElement(&((*reservationLists)[highest].data), &nodeId);
        }
        else {
            int resThatWasThere = (*reservationRules)[highest].resList;
            if (resThatWasThere != highest) {
                (*reservationLists)[highest].prevListInd = (*reservationLists)[resThatWasThere].prevListInd;
                (*reservationLists)[resThatWasThere].prevListInd = highest;
                (*reservationLists)[resThatWasThere].out = 0;
                (*reservationLists)[highest].nextListInd = resThatWasThere;
            }
            addElement(&((*reservationLists)[highest].data), &nodeId);
            int placeInd = highest;
            int placeIndPrev;
            do {
                do {
                    placeIndPrev = placeInd;
                    placeInd = (*reservationLists)[placeIndPrev].prevListInd;
                } while ((*reservationLists)[placeIndPrev].out == 0);
                if ((*reservationRules)[placeInd].resList == -1) {
                    break;
                }
                (*reservationLists)[placeIndPrev].prevListInd = placeInd;
                (*reservationLists)[placeIndPrev].out = 0;
                (*reservationLists)[placeInd].nextListInd = placeIndPrev;
            } while (1);
            (*reservationRules)[placeInd].resList = placeIndPrev;
            (*reservationRules)[placeInd].all = (placeIndPrev != highest);
        }
    }
}

int check_conditions(IfAscendingParams* ifAscendingParams, Node* node) {
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

	int is_true = 1;
    char* condition;
	for (int condInd = 0; condInd < node->nb_conditions; condInd++) {
		condition = node->conditions[condInd];
		// TODO: Implement the logic to check the condition
        is_true = 1;
        if (!is_true) {
            break;
        }
	}
    for (int i = 0; i < node->nb_groups; i++) {
		is_true = check_conditions(ifAscendingParams, node->groups[i]);
		if (!is_true) {
			break;
		}
    }
	return is_true;
}

int getNodes(IfAscendingParams* ifAscendingParams) {
    const char* infoFolder = "C:/Users/abarb/Documents/health/news_underground/mediaSorter/programs/data/info";

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

    // Allocate memory for the full file path
    size_t filePathSize = strlen(infoFolder) + strlen(*sheetPath) + strlen(".txt") + 1;
    char* filePath = (char*)malloc(filePathSize);

    if (!filePath) {
        printf("Failed to allocate memory for file path.\n");
        return -1; // Memory allocation error
    }

    // Construct the full file path
    snprintf(filePath, filePathSize, "%s%s.txt", infoFolder, *sheetPath);
    
    if (openTxtFile(&hFile, &ov, file, fileContent, json, filePath)) {
        return -1;
    }

    cJSON* nodesArray = cJSON_GetObjectItem(json, "nodes");
    *n = cJSON_GetArraySize(nodesArray);
	*nodes = malloc(*n * sizeof(Node));
    int i = 0;
    cJSON* node;
    cJSON_ArrayForEach(node, nodesArray) {
		cJSON* ulteriorsArray = cJSON_GetObjectItem(node, "ulteriors");
        (*nodes)[i].nb_ulteriors = cJSON_GetArraySize(ulteriorsArray);
        (*nodes)[i].ulteriors = malloc((*nodes)[i].nb_ulteriors * sizeof(int));
		cJSON* cJSONEl;
		cJSON_ArrayForEach(cJSONEl, ulteriorsArray) {
			(*nodes)[i].ulteriors[i] = cJSONEl->valueint;
		}
		cJSON* conditionsArray = cJSON_GetObjectItem(node, "conditions");
        (*nodes)[i].nb_conditions = cJSON_GetArraySize(conditionsArray);
		(*nodes)[i].conditions = malloc((*nodes)[i].nb_conditions * sizeof(char*));
		cJSON_ArrayForEach(cJSONEl, conditionsArray) {
			(*nodes)[i].conditions[i] = cJSONEl->valuestring;
		}
		cJSON* groupsArray = cJSON_GetObjectItem(node, "groups");
		(*nodes)[i].nb_groups = cJSON_GetArraySize(groupsArray);
		(*nodes)[i].groups = malloc((*nodes)[i].nb_groups * sizeof(Node*));
		cJSON_ArrayForEach(cJSONEl, groupsArray) {
			(*nodes)[i].groups[i] = *nodes + cJSONEl->valueint;
		}
		cJSON* allowedPlacesArray = cJSON_GetObjectItem(node, "allowedPlaces");
		(*nodes)[i].nb_allowedPlaces = cJSON_GetArraySize(allowedPlacesArray);
		(*nodes)[i].allowedPlaces = malloc((*nodes)[i].nb_allowedPlaces * sizeof(allowedPlaceSt));
		cJSON_ArrayForEach(cJSONEl, allowedPlacesArray) {
			(*nodes)[i].allowedPlaces[i].start = cJSON_GetObjectItem(cJSONEl, "start")->valueint;
			(*nodes)[i].allowedPlaces[i].end = cJSON_GetObjectItem(cJSONEl, "end")->valueint;
		}
		cJSON* errorFunctionsArray = cJSON_GetObjectItem(node, "errorFunctions");
		(*nodes)[i].nb_errorFunctions = cJSON_GetArraySize(errorFunctionsArray);
		(*nodes)[i].errorFunctions = malloc((*nodes)[i].nb_errorFunctions * sizeof(char*));
		cJSON_ArrayForEach(cJSONEl, errorFunctionsArray) {
			(*nodes)[i].errorFunctions[i] = cJSONEl->valuestring;
		}
		(*nodes)[i].nbPost = cJSON_GetObjectItem(node, "nbPost")->valueint;
		i++;
    }

	cJSON* attributesArray = cJSON_GetObjectItem(json, "attributes");
	*nb_att = cJSON_GetArraySize(attributesArray);
	*attributes = malloc(*nb_att * sizeof(attributeSt));
	i = 0;
	cJSON* attribute;
	cJSON_ArrayForEach(attribute, attributesArray) {
        (*attributes)[i].lastPlace = -1;
        (*attributes)[i].sep = cJSON_GetObjectItem(attribute, "sep")->valueint;
		i++;
	}

	cleanFile(&hFile, &ov, file, fileContent, json, filePath);
	return 0;
}

int openTxtFile(HANDLE* hFile, OVERLAPPED* ov, FILE* file, char* fileContent, cJSON* json, char* file_path) {
    if (lockFile(hFile, ov, file_path)) {
        return -1;
    }

    // Open the file for reading
    errno_t err;
    err = fopen_s(&file, file_path, "r");
    if (err) {
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

void cleanFile(HANDLE* hFile, OVERLAPPED* ov, FILE* file, char* fileContent, cJSON* json, const char* file_path) {
	test2(&hFile, ov);
    if (json) {
        // Serialize the JSON object to a string
        char* updatedContent = cJSON_Print(json);
        if (!updatedContent) {
            printf("Failed to serialize JSON.\n");
        }
        else {
            // Reopen the file in write mode to update it
			errno_t err;
            err = fopen_s(&file, file_path, "w");
            if (err) {
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

void chkIfSameSheetID(IfAscendingParams* ifAscendingParams) {
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

    if (!cJSON_GetArraySize(waitingSheets) || cJSON_GetObjectItem(json, "stop")->valueint) {
        (*activeThrd)[rank] = 0;
		*sheetID = NULL;
		*n = 0;
    }

    cJSON* waitingSheet = cJSON_GetArrayItem(waitingSheets, 0);
    int diff_sheet = !*sheetID || strcmp(waitingSheet->valuestring, *sheetID);
    if (!*sheetID || diff_sheet || *reachedEnd) {
        if (*sheetID && diff_sheet) {
            (*allProblems)[*prbInd].nb_process_on_it--;
            if (!(*allProblems)[*prbInd].nb_process_on_it) {
                // Remove a Problem (because this thread was the last one to work on it)
                for (int i = 0; i < (*allProblems)[*prbInd].nb_params; i++) {
                    free((*allProblems)[*prbInd].allWaitingParams[i].result);
                    free((*allProblems)[*prbInd].allWaitingParams[i].sharers);
                }
                free((*allProblems)[*prbInd].allWaitingParams);
                removeInd(*allProblems, *nb_allProblems, *prbInd, sizeof(problemSt));
				for (int i = *prbInd; i < *nb_allProblems; i++) {
					(*allProblems)[i].prbInd--;
				}
            }
        }
        int fstThrdOnPb = 0;
        if (diff_sheet && *n) {
            int found = 0;
            *sheetID = realloc(*sheetID, strlen(waitingSheet->valuestring) + 1);
            strcpy_s(*sheetID, strlen(waitingSheet->valuestring), waitingSheet->valuestring);
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
                getNodes(ifAscendingParams);
                (*nb_allProblems)++;
                getPrevSort(ifAscendingParams, bufferJS);
            }
            (*allProblems)[*prbInd].nb_process_on_it++;
            *nodes = &(*allProblems)[*prbInd].nodes;
            *attributes = &(*allProblems)[*prbInd].attributes;
            *n = (*allProblems)[*prbInd].n;
            *nb_att = &(*allProblems)[*prbInd].nb_att;
            *initBefUpdErr = fstNbItBefUpdErr;
        }
        if (*sheetID) {
            reallocVar(ifAscendingParams);
        }
        else {
			free(*busyNodes);
			free(*rests);
			free(*reservationRules);
			free(*reservationLists);
			free(*startExcl);
			free(*endExcl);
			free(*errors);
			free(*result);
			for (int i = 0; i <= *n; i++) {
				freeList(&(*rests)[i]);
			}
			for (int i = 0; i < *n - 1; i++) {
				freeList(&((*reservationLists)[i].data));
			}
        }
		if (!*n) {
			return;
		}
        int paramInd = (*allProblems)[*prbInd].nb_params - 1;
        if (paramInd == -1) {
            cleanFile(&hFile, &ov, file, fileContent, json, *sort_info_path);
			waitForJob(ifAscendingParams);
			*fileCleaned = 1;
        }
        else {
			*result = (*allProblems)[*prbInd].allWaitingParams[paramInd].result;
			*sharers = (*allProblems)[*prbInd].allWaitingParams[paramInd].sharers;
            *fstSharedDepth = (*allProblems)[*prbInd].allWaitingParams[paramInd].fstSharedDepth;
            (*nb_allProblems)--;
            *allProblems = realloc(*allProblems, *nb_allProblems * sizeof(problemSt));
        }
    }
}

// Function to search and modify the content in the file
int modifyFile(IfAscendingParams* ifAscendingParams, int* stopAll) {
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

	if (openTxtFile(&hFile, &ov, file, fileContent, json, *sort_info_path)) {
		return -1;
	}

    waitingSheets = cJSON_GetObjectItem(json, "waitingSheets");
    sheetIDToBuffer = cJSON_GetObjectItem(json, "sheetIDToBuffer");


    if (notMain && *error) {
        updateBefUpd(ifAscendingParams);
        if (*sheetID) {
            updateThrdAchiev(ifAscendingParams, json, sheetIDToBuffer);
        }
    }
    cJSON* nb_processJS = cJSON_GetObjectItem(json, "nb_process");
    int new_nb_process = nb_processJS->valueint;
    int new_nb_process_temp = *nb_process;
    *fileCleaned = 0;
    if (notMain) {
        chkIfSameSheetID(ifAscendingParams);
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
                        CloseHandle((*threadEvents)[i]);
                        free((*allParams)[i].result);
                        free((*allParams)[i].sharers);
                    }
                }
                if (!new_nb_process_temp) {
					SetEvent(*mainSemaphore);
                }
            }
            for (int i = 0; i < *nb_awaitingThrds; i++) {
                if ((*awaitingThrds)[i] >= new_nb_process) {
                    SetEvent((*threadEvents)[rank]);
					removeInd(*awaitingThrds, nb_awaitingThrds, i, sizeof(int));
				}
			}
			*sheetID = NULL;
			modifyFile(ifAscendingParams, stopAll);
		}
	}
	chg_nb_process(ifAscendingParams, new_nb_process_temp);

    cleanFile(&hFile, &ov, file, fileContent, json, *sort_info_path);

    return 0;
}

void chooseWithResRules(IfAscendingParams* ifAscendingParams) {
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

    int placeInd;
    int choiceInd;
    if ((*result)[*depth].result.result == -1) {
        placeInd = (*reservationRules)[*depth].resList;
        choiceInd = 0;
    }
    else {
        placeInd = (*result)[*depth].result.reservationListInd;
        choiceInd = (*result)[*depth].result.result + 1;
    }
    (*result)[*depth].result.result = -1;
    int found = 0;
    do {
        for (int i = choiceInd; i < (*reservationLists)[placeInd].data.size; i++) {
            (*result)[*depth].nodeId = ((int*)(*reservationLists)[placeInd].data.data)[i];
            if (busyNodes[(*result)[*depth].nodeId] == 0 && check_conditions(&ifAscendingParams, &nodes[i])) {
                (*result)[*depth].result.reservationListInd = placeInd;
                (*result)[*depth].result.result = i;
                found = 1;
                break;
            }
        }
        if (!found) {
            if (!(*reservationRules)[*depth].all) {
                break;
            }
            choiceInd = 0;
            placeInd = (*reservationLists)[placeInd].nextListInd;
        }
    } while (!found && placeInd != -1);
    if (!found) {
        *ascending = 0;
    }
}

void updateErrors(IfAscendingParams* ifAscendingParams, Node* node) {
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

    for (int condInd = 0; condInd < node->nb_errorFunctions; condInd++) {
        (*errors)[*depth] += 1;
    }
    for (int i = 0; i < node->nb_groups; i++) {
        updateErrors(ifAscendingParams, node->groups[i]);
    }
}

void chooseNextOpt(IfAscendingParams* ifAscendingParams) {
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

    // if the tree is shared with another thread at this depth
    if (*depth == *lastSharedDepth) {
        WaitForSingleObject(semaphore, INFINITE); // Acquire the semaphore
        (*result)[*depth].result = (*sharers)[*depth]->result;
    }

    if ((*reservationRules)[*depth].resList == -1) {
        do {
            (*result)[*depth].result.result++;
            if ((*result)[*depth].result.result == (*rests)[*depth].size) {
                *ascending = 0;
                break;
            }
            (*result)[*depth].nodeId = ((int*)(*rests)[*depth].data)[(*result)[*depth].result.result];
			if (check_conditions(ifAscendingParams, &nodes[(*result)[*depth].nodeId])) {
                break;
            }
        } while (1);
    }
    else {
		chooseWithResRules(ifAscendingParams);
    }

	updateErrors(ifAscendingParams, &nodes[(*result)[*depth].nodeId]);

    // if the tree is shared with another thread at this depth
    if (*depth == *lastSharedDepth) {
        if (*ascending) {
            (*sharers)[*depth]->result = (*result)[*depth].result;
        }
        else {
            (*lastSharedDepth)--;
        }
        ReleaseSemaphore(semaphore, 1, NULL); // Release the semaphore
    }
}

int updateBefUpd(IfAscendingParams* ifAscendingParams) {
    const int incremBefUpdErr = 50;
    const int initBefUpdErrMax = 10000;
    const int initBefUpdErrMin = 50;
    const int leastBefUpdErrToUpd = 30;

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

    if (*errorThrd != *error || *nb_awaitingThrds > 0) {
        if (initBefUpdErr - incremBefUpdErr >= initBefUpdErrMin) {
            initBefUpdErr -= incremBefUpdErr;
        }
	}
	else if(!*befUpdErr < leastBefUpdErrToUpd && *initBefUpdErr + incremBefUpdErr <= initBefUpdErrMax) {
        *initBefUpdErr += incremBefUpdErr;
	}
	*befUpdErr = *initBefUpdErr;
    if ((*errors)[*n - 1] < *error) {
        *error = (*errors)[*n - 1];
    }
    *errorThrd = *error;
}

int updateThread(IfAscendingParams* ifAscendingParams) {
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

    WaitForSingleObject(semaphore, INFINITE); // Acquire the semaphore
    int stopAll;
    modifyFile(ifAscendingParams, &stopAll);
    if (stopAll) {
        if (stopAll == 2) {
            ReleaseSemaphore(semaphore, 1, NULL); // Release the semaphore
            return 1;
        }
        *depth = 0;
    }
    if (*nb_awaitingThrds > 0 && *lastSharedDepth < n - 2) {
		giveJob(ifAscendingParams);
    }
    else {
        ReleaseSemaphore(semaphore, 1, NULL);
    }
	return 0;
}

int reallocVar(IfAscendingParams* ifAscendingParams) {
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

	*depth = 0;
    *busyNodes = malloc(*n * sizeof(int));
    *rests = malloc(*n * sizeof(GenericList));
    *reservationRules = malloc((*n + 1) * sizeof(ReservationRule));
    *reservationLists = malloc((*n - 1) * sizeof(ReservationList));
    *startExcl = malloc(*n * sizeof(GenericList));
    *endExcl = malloc(*n * sizeof(GenericList));
    *errors = malloc(*n * sizeof(int));
    (*errors)[0] = 0;
    for (int i = 0; i <= *n; i++) {
        initList(&(*rests)[i], sizeof(int), 1);
        (*busyNodes)[i] = (*nodes)[i].nbPost;
        (*reservationRules)[i].resList = -1;
    }
    *result = malloc(*n * sizeof(sharing));
    for (int i = 0; i < *n - 1; i++) {
        initList(&((*reservationLists)[i].data), sizeof(int), 1);
    }
    for (int i = 0; i < *n; i++) {
        (*result)[i].result.result = -1;
        Node* node = &(*nodes)[i];
        for (int j = 0; j < node->nb_allowedPlaces; i++) {
            if (node->allowedPlaces[j].start != 0) {
                addElement(&(*startExcl)[node->allowedPlaces[j].start], i);
            }
            else if (j == 0) {
                (*busyNodes)[i]++;
            }
            addElement(&(*endExcl)[node->allowedPlaces[j].end], i);
        }
        if ((*busyNodes)[i] == 0 && check_conditions(ifAscendingParams, node)) {
            addElement(&((*rests)[0]), &i);
        }
    }
    for (int i = 0; i < *n - 1; i++) {
        notChosenAnymore(ifAscendingParams, i);
    }
}

void waitForJob(IfAscendingParams* ifAscendingParams) {
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

    (*nb_awaitingThrds)++;
    *awaitingThrds = realloc(*awaitingThrds, *nb_awaitingThrds * sizeof(int));
    (*awaitingThrds)[*nb_awaitingThrds] = rank;
    ReleaseSemaphore(semaphore, 1, NULL);
    WaitForSingleObject((*threadEvents)[rank], INFINITE);
    *lastSharedDepth = *fstSharedDepth;
	WaitForSingleObject(semaphore, INFINITE);
}

void chg_nb_process(IfAscendingParams* ifAscendingParams, int new_nb_process) {
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

    *activeThrd = realloc(*activeThrd, new_nb_process * sizeof(int));
    for (int i = *nb_process; i < new_nb_process; i++) {
        (*activeThrd)[i] = 0;
    }
    *allParams = realloc(*allParams, new_nb_process * sizeof(ThreadParams));
    *threadEvents = realloc(*threadEvents, new_nb_process * sizeof(HANDLE));
	*nb_process = new_nb_process;
    for (int i = 0; i < new_nb_process; i++) {
        if (!(*activeThrd)[i]) {
            (*activeThrd)[i] = 1;
            (*threadEvents)[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
            ThreadParamsRank threadParamsRank = {
                .rank = i,
                .meetPoint = meetPoint,
            };
			CloseHandle(CreateThread(NULL, 0, threadSort, &threadParamsRank, 0, NULL));
        }
    }
}

int giveJob(IfAscendingParams* ifAscendingParams) {
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

    (*nb_awaitingThrds)--;
	int otherRank = (*awaitingThrds)[*nb_awaitingThrds];
    *awaitingThrds = realloc(*awaitingThrds, *nb_awaitingThrds * sizeof(int));
    ThreadParams* otherParams = &(*allParams)[otherRank];
    otherParams->fstSharedDepth = ++(*lastSharedDepth);
    if (*lastSharedDepth > -1) {
		otherParams->result = malloc(*lastSharedDepth * sizeof(sharing));
        sharingRef sharers = {
            .result = (*result)[*lastSharedDepth].result,
            .sharers = malloc(2 * sizeof(sharingRef)),
        };
		sharers.sharers[0] = rank;
		sharers.sharers[1] = otherRank;
    }
    ReleaseSemaphore(semaphore, 1, NULL);
    SetEvent(threadEvents[otherRank]);
}

int main(int argc, char* argv[]) {
    const char* sort_info_path = "C:/Users/abarb/Documents/health/news_underground/mediaSorter/programs/data/sort_info.txt";

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
    int nb_process = 0;
    int stopAll;
    int* activeThrd = NULL;
    int nb_allProblems = 0;

	ThreadParamsRank _threadParamsRank = {
		.meetPoint = &meetPoint,
	};
    IfAscendingParams ifAscendingParams = {
		.threadParamsRank = &_threadParamsRank,
    };
	modifyFile(&ifAscendingParams, &stopAll);

	WaitForSingleObject(meetPoint.mainSemaphore, INFINITE);

    return 0;
}
