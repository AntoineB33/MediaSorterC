#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"  // Include the cJSON header
#include <curl/curl.h>
#include <stdarg.h>
//#include <oleauto.h>
//#include <comutil.h>
////#include <time.h>
#include <Python.h>
#include <windows.h>
#include <stdbool.h>
#include <limits.h>


#define max(a, b) ((a) > (b) ? (a) : (b))


typedef struct {
    void* data;       // Pointer to the array (generic list)
    size_t size;      // Number of elements in the list
    size_t capacity;  // Capacity of the list
    size_t elemSize;  // Size of each element
} GenericList;

typedef struct {
    int start;
	int end;
} allowedPlaceSt;

// Define a structure to represent the graph node
typedef struct {
	int* ulteriors;	 // List of integers that must come after
    int nb_ulteriors;
    char** conditions;	   // List of conditions that must be satisfied
    int nb_conditions;
	Node** groups;
	int nb_groups;
    allowedPlaceSt* allowedPlaces;
    int nb_allowedPlaces;
    char** errorFunctions;
	int nb_errorFunctions;
	int nbPost;            // Number of nodes that must come before
} Node;

typedef struct {
	int all;            // 1 if at this depth any one can be chosen in the consecutive resLists, 0 otherwise
    int resList;
} ReservationRule;

typedef struct {
	GenericList data;	// List of the nodes that are in the reservation list
	int out;            // index of the first element that is not in the list
    int prevListInd;
    int nextListInd;
} ReservationList;      // List of the nodes that need to be placed at a certain depth

typedef struct {
    int lastOcc;
} AttributeMng;

typedef struct {
    int result;
    int reservationListInd;
} resultSt;

typedef struct {
    resultSt result;
    int nodeId;
} sharing;

typedef struct {
	int sharer;
    resultSt result;
} sharerResult;

typedef struct {
    int sep;
	int lastPlace;
} attributeSt;

typedef struct {
    char* sheetID;
    Node* nodes;
    int n;
    attributeSt* attributes;
    int nb_att;
    int error;
    int nb_process_on_it;
} problemSt;

typedef struct {
	problemSt* problem;
	int fstSharedDepth;
    sharing* result;
    sharerResult* sharers;
} ThreadParams;

typedef struct {
	HANDLE hThread;
    HANDLE eventHandlesThd;
} ThreadEvent;

typedef struct {
    const char* sort_info_path;
    problemSt** allProblems;
	int nb_allProblems;
	HANDLE* mainSemaphore;
    HANDLE* semaphore;
    ThreadEvent* threadEvents;
	int nb_process;
    ThreadParams* allParams;
    int nb_params;
    int* awaitingThrds;
    int nb_awaitingThrds;
	int* activeThrd;
} MeetPoint;

typedef struct {
    MeetPoint* meetPoint;
	int rank;
} ThreadParamsRank;

// Define the structure of an element
typedef struct {
    char stringField[256];
    int intField;
    int* listOfInts;
    int listOfIntsSize;
    int** listOfListOfInts;
    int listOfListOfIntsSize;
    int* listSizes; // Stores sizes of each sub-list in listOfListOfInts
} Element;


// Function to initialize the list
void initList(GenericList* list, size_t elemSize, size_t initialCapacity) {
    list->data = malloc(initialCapacity * elemSize); // Allocate memory for the generic array
    if (list->data == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }
    list->size = 0;
    list->capacity = initialCapacity;
    list->elemSize = elemSize;
}

//get last element
void* getLastElement(GenericList* list) {
	return (char*)list->data + (list->size - 1) * list->elemSize;
}

// Function to add an element to the list
void addElement(GenericList* list, void* element) {
    // Check if the list is full and resize if needed
    if (list->size == list->capacity) {
        list->capacity++;
        void* newData = realloc(list->data, list->capacity * list->elemSize);
        if (newData == NULL) {
            printf("Memory reallocation failed\n");
            exit(1);
        }
        list->data = newData;
    }
    // Copy the new element into the list
    void* destination = (char*)list->data + (list->size * list->elemSize);
    memcpy(destination, element, list->elemSize);
    list->size++;
}

// Function to remove an integer from the list
void removeElement(GenericList* list, int element) {
    // Iterate through the list to find the element
    for (size_t i = 0; i < list->size; i++) {
        int* currentElement = (int*)((char*)list->data + i * list->elemSize);
        if (*currentElement == element) {
            // Element found, now shift the remaining elements
            for (size_t j = i; j < list->size - 1; j++) {
                void* src = (char*)list->data + (j + 1) * list->elemSize;
                void* dst = (char*)list->data + j * list->elemSize;
                memcpy(dst, src, list->elemSize);
            }
            list->size--;  // Reduce the size after removing the element
            return;        // Exit after removing the element
        }
    }

    // If the element is not found, print a message
    printf("Element not found in the list\n");
}

void removeIndGL(GenericList* list, int ind) {
	// Shift the elements after the index to the left
	for (int i = ind; i < list->size - 1; i++) {
		void* src = (char*)list->data + (i + 1) * list->elemSize;
		void* dst = (char*)list->data + i * list->elemSize;
		memcpy(dst, src, list->elemSize);
	}
	list->size--;  // Reduce the size after removing the element
}

void removeInd(void* list, int listSize, int ind, size_t elemSize) {
    // Shift the elements after the index to the left
    for (int i = ind; i < listSize - 1; i++) {
        void* src = (char*)list + (i + 1) * elemSize;
        void* dst = (char*)list + i * elemSize;
        memcpy(dst, src, elemSize);
    }
}

// Function to free the list
void freeList(GenericList* list) {
    free(list->data);
    list->size = 0;
    list->capacity = 0;
}

void freeNode(Node* node) {
    freeList(&node->ulteriors);
    freeList(&node->conditions);
	freeList(&node->allowedPlaces);
}

void freeNodes(Node* nodes, size_t nodeCount) {
    for (size_t i = 0; i < nodeCount; i++) {
        freeNode(&nodes[i]);
    }
    free(nodes);
}

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

inline int lockFile(HANDLE* hFile, OVERLAPPED* ov, const char* filePath) {
    *hFile = CreateFileA(
        filePath,
        GENERIC_READ | GENERIC_WRITE,
        0,              // No sharing
        NULL,           // Default security
        OPEN_EXISTING,  // Open an existing file
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Failed to open file. Error: %ld\n", GetLastError());
        return -1;
    }

    ZeroMemory(ov, sizeof(OVERLAPPED));

    // Keep trying to lock until successful
    while (!LockFileEx(hFile, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &ov)) {
        Sleep(1000); // Wait for 1 second before retrying
    }

    return 0;
}

inline void unlockFile(HANDLE* hFile, OVERLAPPED* ov) {
    UnlockFileEx(*hFile, 0, MAXDWORD, MAXDWORD, ov);
	CloseHandle(*hFile);
}

inline int getNodes(char* sheetPath, int* n, Node* nodes, attributeSt* attributes) {
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
	attributes = malloc(cJSON_GetArraySize(attributesArray) * sizeof(attributeSt));
	if (attributesArray && cJSON_IsArray(attributesArray)) {
		int i = 0;
		cJSON* attribute;
		cJSON_ArrayForEach(attribute, attributesArray) {
			attributes[i].lastPlace = -1;
			attributes[i].sep = cJSON_GetObjectItem(attribute, "sep")->valueint;
			i++;
		}
	}

	cleanFile(&hFile, &ov, file, fileContent, json);
	return 0;
}

inline void getPrevSort(char* sort_info_path, cJSON* sheetIDToBuffer, char* sheetID, ThreadParams* allParams) {
    HANDLE hFile;
    OVERLAPPED ov;
    FILE* file;
    char* fileContent;
    cJSON* json;
    if (openTxtFile(&hFile, &ov, file, fileContent, json, sort_info_path)) {
        return -1;
    }

    cJSON* bufferJS = cJSON_GetObjectItemCaseSensitive(sheetIDToBuffer, sheetID);
    cJSON* threadsArray = cJSON_GetObjectItem(bufferJS, "threads");
	*nb_params = max(cJSON_GetArraySize(threadsArray), *nb_process);
	allParams = malloc(*nb_params * sizeof(ThreadParams));
	int i = 0;
	cJSON* thread;
    cJSON_ArrayForEach(thread, threadsArray) {
        cJSON* resultJS = cJSON_GetObjectItem(thread, "result");
		allParams[i].result = malloc(cJSON_GetArraySize(resultJS) * sizeof(sharing));
        int k = 0;
        cJSON* cJSONEl;
        cJSON_ArrayForEach(cJSONEl, resultJS) {
            cJSON* resultAtt = cJSON_GetObjectItem(cJSONEl, "result");
            cJSON* reservationListInd = cJSON_GetObjectItem(cJSONEl, "reservationListInd");
			allParams[i].result[k].result.result = resultAtt->valueint;
			allParams[i].result[k].result.reservationListInd = reservationListInd->valueint;
            k++;
        }
        cJSON* sharersJS = cJSON_GetObjectItem(thread, "sharers");
		allParams[i].sharers = malloc(cJSON_GetArraySize(sharersJS) * sizeof(sharerResult));
        int k = 0;
        cJSON* cJSONEl;
        cJSON_ArrayForEach(cJSONEl, sharersJS) {
            cJSON* sharerAtt = cJSON_GetObjectItem(cJSONEl, "sharer");
            cJSON* resultAtt = cJSON_GetObjectItem(cJSONEl, "result");
			allParams[i].sharers[k].sharer = sharerAtt->valueint;
			allParams[i].sharers[k].result.result = resultAtt->valueint;
            k++;
        }
        i++;
    }
    
	cleanFile(&hFile, &ov, file, fileContent, json);
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

inline void cleanFile(HANDLE* hFile, OVERLAPPED* ov, FILE* file, char* fileContent, cJSON* json) {
    // Cleanup
    cJSON_Delete(json);
    free(fileContent);

    fclose(file);

    unlockFile(&hFile, &ov);
}

inline void updateThrdAchiev(cJSON* sheetIDToBuffer, char* sheetID, char* sheetPath, int isCompleteSorting, int* error, cJSON* waitingSheets, cJSON* json, ThreadParams* allParams, int rank, int n, sharing* result, sharerResult* sharers, int* stopAll) {
    int stillRelvt = 1;
    cJSON* sheetPathToSheetID = cJSON_GetObjectItem(json, "sheetPathToSheetID");
    cJSON* sheetIDJS = cJSON_GetObjectItem(sheetPathToSheetID, sheetPath);
    if (strcmp(sheetIDJS->valuestring, sheetID) != 0) {
        stillRelvt = 0;
    }
    else if (*error == 0) {
        stillRelvt = isCompleteSorting;
    }
    int toUpdate = 0;

	if (stillRelvt) {
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
        if (*error) {
            cJSON* threadsArray = cJSON_GetObjectItem(bufferJS, "threads");
            int j = 0;
            cJSON* thread = cJSON_GetArrayItem(threadsArray, rank);
            cJSON* resultJS = cJSON_GetObjectItem(thread, "result");
            int k = 0;
            cJSON* resultElmnt;
            cJSON_ArrayForEach(resultElmnt, resultJS) {
                cJSON* resultAtt = cJSON_GetObjectItem(resultElmnt, "result");
                cJSON* reservationListInd = cJSON_GetObjectItem(resultElmnt, "reservationListInd");
                if (resultAtt && cJSON_IsNumber(resultAtt) && reservationListInd && cJSON_IsNumber(reservationListInd)) {
                    cJSON_ReplaceItemInObject(resultElmnt, "result", cJSON_CreateNumber(result[k].result.result));
                    cJSON_ReplaceItemInObject(resultElmnt, "reservationListInd", cJSON_CreateNumber(result[k].result.reservationListInd));
                }
                k++;
            }
            cJSON* sharersJS = cJSON_GetObjectItem(thread, "sharers");
            int k = 0;
            cJSON* sharer;
            cJSON_ArrayForEach(sharer, sharersJS) {
                cJSON* sharerAtt = cJSON_GetObjectItem(sharer, "sharer");
                cJSON* resultAtt = cJSON_GetObjectItem(sharer, "result");
                if (sharerAtt && cJSON_IsNumber(sharerAtt) && resultAtt && cJSON_IsNumber(resultAtt)) {
                    cJSON_ReplaceItemInObject(sharer, "sharer", cJSON_CreateNumber(sharers[k].sharer));
                    cJSON_ReplaceItemInObject(sharer, "result", cJSON_CreateNumber(sharers[k].result.result));
                }
                k++;
            }
        }
	}
}

inline void chkIfSameSheetID(int* stopAll, cJSON* waitingSheets, char* sheetID, cJSON* sheetIDToBuffer, int prbInd, problemSt** allProblems, int* nb_allProblems, Node* nodes, attributeSt* attributes, int* n, int* error, int* nb_att) {
    const int fstNbItBefUpdErr = 50;


    if (cJSON_GetArraySize(waitingSheets)) {
        cJSON* waitingSheet = cJSON_GetArrayItem(waitingSheets, 0);
        if (!sheetID) {
            *stopAll = 1;
        }
        else {
            *stopAll = strcmp(waitingSheet->valuestring, sheetID);
        }
        if (*stopAll) {
            allProblems[prbInd]->nb_process_on_it--;
            if (!allProblems[prbInd]->nb_process_on_it) {
                (*nb_allProblems)--;
                for (int j = prbInd; j < *nb_allProblems; j++) {
                    allProblems[j] = allProblems[j + 1];
                }
                allProblems = realloc(allProblems, *nb_allProblems * sizeof(problemSt*));
            }
			sheetID = waitingSheet->valuestring;
            int found = 0;
            for (int i = 0; i < *nb_allProblems; i++) {
                if (strcmp(allProblems[i]->sheetID, sheetID) == 0) {
                    nodes = allProblems[i]->nodes;
                    attributes = allProblems[i]->attributes;
                    *n = allProblems[i]->n;
                    *error = allProblems[i]->error;
                    *nb_att = allProblems[i]->nb_att;
					allProblems[i]->nb_process_on_it++;
                    found = 1;
                    break;
                }
            }
            if(!found) {
                (*nb_allProblems)++;
				allProblems = realloc(allProblems, *nb_allProblems * sizeof(problemSt*));
                cJSON* bufferJS = cJSON_GetObjectItemCaseSensitive(sheetIDToBuffer, sheetID);
                getNodes();
                getPrevSort();
            }
            initBefUpdErr = fstNbItBefUpdErr;
        }
    }
    else {
        *stopAll = 2;
        *sheetID = NULL;
    }
}

// Function to search and modify the content in the file
inline int modifyFile(char* sort_info_path, char* sheetID, int isCompleteSorting, HANDLE semaphore, int* befUpdErr, int* errorThrd, int* depth, int* lastSharedDepth, int nbItBefUpdErr, ReservationList* reservationLists, ReservationRule* reservationRules, int* busyNodes, int* ascending, int* error, int* nb_awaitingThrds, int n, int* awaitingThrds, ThreadParams* allParams, sharing* result, int rank, sharerResult* sharers, int* nb_process, ThreadEvent* threadEvents, GenericList* const rests, Node* nodes, int* stopAll, int* activeThrd) {
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
    if (*sheetID) {
		updateThrdAchiev(sheetIDToBuffer, sheetID, isCompleteSorting, error, waitingSheets, json, allParams, rank, n);
    }


	cJSON* nb_processJS = cJSON_GetObjectItem(json, "nb_process");
	int new_nb_process = nb_processJS->valueint;
    *stopAll = 0;
    if (new_nb_process < nb_process) {
        if (rank >= new_nb_process) {
            activeThrd[rank] = 0;
            if (rank == new_nb_process - 1) {
                for (int i = nb_process - 2; i >= new_nb_process; i--) {
                    if (activeThrd[i] != -1) {
                        chg_nb_process(new_nb_process);
                        break;
                    }
                }
            }
            *stopAll = 2;
        }
	}
	else if (new_nb_process > nb_process) {
        chg_nb_process(new_nb_process);
	}

    if (*stopAll == 0) {
        chkIfSameSheetID(stopAll, waitingSheets, sheetID, sheetIDToBuffer);
    }

	cleanFile(&hFile, &ov, file, fileContent, json);

    return 0;
}

inline void chooseWithResRules(HANDLE semaphore, int* befUpdErr, int* errorThrd, int* depth, int* lastSharedDepth, int nbItBefUpdErr, ReservationList* reservationLists, ReservationRule* reservationRules, int* busyNodes, int* ascending, int* error, int* nb_awaitingThrds, int n, int* awaitingThrds, ThreadParams* allParams, sharing* result, int rank, sharerResult* sharers, int* nb_process, ThreadEvent* threadEvents, GenericList* const rests, Node* nodes, const char* const sort_info_path) {
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
        ascending = 0;
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

inline void chooseNextOpt(HANDLE semaphore, int* befUpdErr, int* errorThrd, int* depth, int* lastSharedDepth, int nbItBefUpdErr, ReservationList* reservationLists, ReservationRule* reservationRules, int* busyNodes, int* ascending, int* error, int* nb_awaitingThrds, int n, int* awaitingThrds, ThreadParams* allParams, sharing* result, int rank, sharerResult* sharers, int* nb_process, ThreadEvent* threadEvents, GenericList* const rests, Node* nodes, const char* const sort_info_path) {


    // if the tree is shared with another thread at this depth
    if (*depth == *lastSharedDepth) {
        WaitForSingleObject(semaphore, INFINITE); // Acquire the semaphore
        result[*depth].result = sharers[*depth].result;
    }

    if (reservationRules[*depth].resList == -1) {
        do {
            result[*depth].result.result++;
            if (result[*depth].result.result >= rests[*depth].size) {
                ascending = 0;
                break;
            }
            result[*depth].nodeId = ((int*)rests[*depth].data)[result[*depth].result.result];
			if (check_conditions(result, &nodes[result[*depth].nodeId], *depth)) {
                break;
            }
        } while (1);
    }
    else {
		chooseWithResRules();
    }

	updateErrors(error);

    // if the tree is shared with another thread at this depth
    if (*depth == *lastSharedDepth) {
        if (result[*depth].result.result > rests[*depth].size) {
            (*lastSharedDepth)--;
        }
        else {
            allParams[sharers[*depth].sharer].sharers[*depth].result = result[*depth].result;
            if (result[*depth].result.result == rests[*depth].size) {
                (*lastSharedDepth)--;
            }
        }
        ReleaseSemaphore(semaphore, 1, NULL); // Release the semaphore
    }
}

inline int updateBefUpd() {
    const int incremBefUpdErr = 50;


    if (errorThrd != *error) {
        initBefUpdErr += incremBefUpdErr;
	}
	else {
        initBefUpdErr -= incremBefUpdErr;
	}
    if (errors[n - 1] < *error) {
        *error = errors[n - 1];
    }
    else {
        errors[n - 1] = *error;
        if (isCompleteSorting) {
            ReleaseSemaphore(semaphore, 1, NULL); // Release the semaphore
            return 0;
        }
    }
    if (!*befUpdErr) {
        *befUpdErr = initBefUpdErr;
    }
}

inline int updateThread(int* befUpdErr, HANDLE semaphore, int* errors, int* error, int* depth, int* nb_awaitingThrds, int* lastSharedDepth, int n, int* awaitingThrds, ThreadParams* allParams, sharing* result, int rank, sharerResult* sharers, ThreadEvent* threadEvents, int nbItBefUpdErr, int isCompleteSorting) {
    WaitForSingleObject(semaphore, INFINITE); // Acquire the semaphore
	updateBefUpd();
    int stopAll;
    modifyFile();
    if (stopAll) {
        if (stopAll == 2) {
            ReleaseSemaphore(semaphore, 1, NULL); // Release the semaphore
            return 1;
        }
        *depth = 0;
    }
    if (*nb_awaitingThrds > 0 && *lastSharedDepth < n - 2) {
        int otherRank = awaitingThrds[*nb_awaitingThrds - 1];
		(*nb_awaitingThrds)--;
		awaitingThrds = realloc(awaitingThrds, *nb_awaitingThrds * sizeof(int));
        ReleaseSemaphore(semaphore, 1, NULL); // Release the semaphore
        (*lastSharedDepth)++;
        ThreadParams* otherParams;
        otherParams = &allParams[otherRank];
        otherParams->fstSharedDepth = *lastSharedDepth;
        for (int i = 0; i <= *lastSharedDepth; i++) {
            otherParams->result[i].result = result[i].result;
        }
        otherParams->sharers[*lastSharedDepth].sharer = rank;
        otherParams->sharers[*lastSharedDepth].result = result[*lastSharedDepth].result;
        sharers[*lastSharedDepth].sharer = otherRank;
        sharers[*lastSharedDepth].result = result[*lastSharedDepth].result;
        SetEvent(threadEvents[rank].eventHandlesThd);
    }
    else {
        ReleaseSemaphore(semaphore, 1, NULL); // Release the semaphore
    }
	return 0;
}

inline int reallocVar() {
    problem->nb_process_on_it++;
    resultNodes = malloc(n * sizeof(int));
    const busyNodes = malloc(n * sizeof(int));
    rests = malloc(n * sizeof(GenericList));
    reservationRules = malloc((n + 1) * sizeof(ReservationRule));
    reservationLists = malloc((n - 1) * sizeof(ReservationList));
    startExcl = malloc(n * sizeof(GenericList));
    endExcl = malloc(n * sizeof(GenericList));
    errors = malloc(n * sizeof(int));
    errors[0] = 0;
    for (int i = 0; i < n + 1; i++) {
        busyNodes[i] = nodes[i].nbPost;
        initList(&rests[i], sizeof(int), 1);
        reservationRules[i].resList = -1;
    }
    for (int i = 0; i < n; i++) {
		result[i].result = -1;
        Node* node = &nodes[i];
        for (int j = 0; j < node->nb_allowedPlaces; i++) {
            if (node->allowedPlaces[j].start != 0) {
                addElement(&startExcl[node->allowedPlaces[j].start], i);
            }
            else if (j == 0) {
                busyNodes[i]++;
            }
            addElement(&endExcl[node->allowedPlaces[j].end], i);
        }
        if (busyNodes[i] == 0 && check_conditions(nodes, node, i)) {
            addElement(&(rests[0]), &i);
        }
    }
    for (int i = 0; i < n - 1; i++) {
        initList(&(reservationLists[i].data), sizeof(int), 1);
        notChosenAnymore(&nodes[i], n, i, reservationRules, reservationLists, startExcl, endExcl);
    }
}

inline int waitOrSearchForJob(HANDLE* semaphore, int* nb_params, int nb_process_on_it, int nb_awaitingThrds, int nb_process, HANDLE* mainSemaphore) {
    WaitForSingleObject(semaphore, INFINITE);
    if (*nb_params > nb_process_on_it) {
        (*nb_params)--;
        *currentParams = allParams[nb_params];
        allParams = realloc(allParams, nb_params * sizeof(ThreadParams));
        fstSharedDepth = currentParams->fstSharedDepth;
        result = currentParams->result;
        sharers = currentParams->sharers;
        ReleaseSemaphore(semaphore, 1, NULL);
    }
    else if (nb_awaitingThrds == nb_process) {
        ReleaseSemaphore(mainSemaphore, 1, NULL);
        ReleaseSemaphore(semaphore, 1, NULL);
        return;
    }
    else {
        nb_awaitingThrds++;
        awaitingThrds = realloc(awaitingThrds, nb_awaitingThrds * sizeof(int));
        awaitingThrds[nb_awaitingThrds] = rank;
        ReleaseSemaphore(semaphore, 1, NULL);
        WaitForSingleObject(threadEvent[rank].eventHandlesThd, INFINITE);
        lastSharedDepth = fstSharedDepth;
    }
    reallocVar();
    depth = 0;
}

// Recursive function to try all possible ways to sort the integer
DWORD WINAPI threadSort(LPVOID lpParam) {
    // Cast lpParam to the appropriate structure type
    ThreadParamsRank* threadParamsRank = (ThreadParamsRank*)lpParam;
    MeetPoint* meetPoint = threadParamsRank->meetPoint;
    char* sort_info_path = meetPoint->sort_info_path;
    HANDLE* semaphore = meetPoint->semaphore;
    ThreadEvent* threadEvent = meetPoint->threadEvents;
    int nb_process = meetPoint->nb_process;
    char* sheetID = meetPoint->sheetID;
    Node* nodes = nodes;
    int n = meetPoint->n;
    attributeSt* attributes = meetPoint->attributes;
    int nb_att = meetPoint->nb_att;
    int* error = meetPoint->error;
    ThreadParams* allParams = meetPoint->allParams;
    int nb_params = meetPoint->nb_params;
    int* awaitingThrds = meetPoint->awaitingThrds;
    int nb_awaitingThrds = meetPoint->nb_awaitingThrds;
	int* activeThrd = meetPoint->activeThrd;

    int rank = threadParamsRank->rank;

    ThreadParams* currentParams = &allParams[rank];
    int fstSharedDepth = currentParams->fstSharedDepth;
    sharing* result = currentParams->result;
    sharerResult* sharers = currentParams->sharers;


	int lastSharedDepth = currentParams->fstSharedDepth;
	int ascending = 1;
    int found = 0;
    int currentElement;
	int befUpdErr;
    int startInd = 0;
    int depth = 0;
    int nodId;

	activeThrd[rank] = 1;

    int* resultNodes = NULL;
    int* const busyNodes = NULL;
    GenericList* const rests = NULL;
    ReservationRule* const reservationRules = NULL;
    ReservationList* const reservationLists = NULL;
    GenericList* const startExcl = NULL;
    GenericList* const endExcl = NULL;
    int* const errors = NULL;


	if (updateThread()) {
		return;
    }
    // main loop
    while (1) {
		chooseNextOpt(threadParamsRank, depth, lastSharedDepth, semaphore, reservationRules, reservationLists, rests, &ascending, busyNodes);

        if (ascending == 1) {
            depth++;
            if (depth == n) {
				if (updateThread()) {
					return;
				}
				ascending = 0;
            }
            else {
                nodId = result[depth - 1].nodeId;

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
        if (ascending == 0) {
			ascending = 1;
            result[depth].result.result = -1;
			depth--;
            if (depth == fstSharedDepth) {
                waitOrSearchForJob();
            }
            else {
                befUpdErr--;
                if (!befUpdErr && updateThread()) {
                    return;
                }
                nodes[result[depth].nodeId].nbPost--;
                notChosenAnymore();
                for (int j = 0; j < nodes[result[depth].nodeId].nb_ulteriors; j++) {
                    int ulterior = ((int*)nodes[result[depth].nodeId].nb_ulteriors)[j];
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
        }
    }
}

inline void chg_nb_process(int* nb_process, ThreadParams* allParams, ThreadEvent* threadEvents, int new_nb_process, int* activeThrd) {
	for (int i = nb_process; i < new_nb_process; i++) {
		allParams = realloc(allParams, (i + 1) * sizeof(ThreadParams));
        threadEvents = realloc(threadEvents, (i + 1) * sizeof(ThreadEvent));
        ThreadEvent threadEvent = {
			.eventHandlesThd = CreateEvent(NULL, FALSE, FALSE, NULL),
			.hThread = CreateThread(NULL, 0, threadSort, &allParams[i], 0, NULL)
		};
        threadEvents[i] = threadEvent;
	}
	for (int i = new_nb_process; i < nb_process; i++) {
		CloseHandle(threadEvents[i].hThread);
		CloseHandle(threadEvents[i].eventHandlesThd);
        allParams = realloc(allParams, (i + 1) * sizeof(ThreadParams));
        threadEvents = realloc(threadEvents, (i + 1) * sizeof(ThreadEvent));
	}
	nb_process = new_nb_process;
}

inline int giveJob(HANDLE* semaphore, int* awaitingThrds, int* nb_awaitingThrds, int* lastSharedDepth, ThreadParams* allParams, sharing* result, int rank, char* sheetID, problemSt* problem) {
    WaitForSingleObject(semaphore, INFINITE);
    int otherRank = awaitingThrds[*nb_awaitingThrds - 1];
    (*nb_awaitingThrds)--;
    awaitingThrds = realloc(awaitingThrds, *nb_awaitingThrds * sizeof(int));
    ReleaseSemaphore(semaphore, 1, NULL); // Release the semaphore
    (*lastSharedDepth)++;
    ThreadParams* otherParams = &allParams[otherRank];
    if (strcmp(otherParams->problem->sheetID, sheetID)) {
		otherParams->problem = problem;
		reallocVar();
    }
    otherParams->fstSharedDepth = *lastSharedDepth;
    for (int i = 0; i <= *lastSharedDepth; i++) {
        otherParams->result[i].result = result[i].result;
    }
    otherParams->sharers[*lastSharedDepth].sharer = rank;
    otherParams->sharers[*lastSharedDepth].result = result[*lastSharedDepth].result;
    sharers[*lastSharedDepth].sharer = otherRank;
    sharers[*lastSharedDepth].result = result[*lastSharedDepth].result;
    SetEvent(threadEvents[rank].eventHandlesThd);
    ReleaseSemaphore(meetPoint.semaphore, 1, NULL);
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
		.activeThrd = NULL
    };
    ThreadParamsRank allParamsRank = {
        .meetPoint = &meetPoint,
        .rank = 0
    };

	// Create a new thread
    modifyFile();

	int lastSharedDepth = -2;
    giveJob();

	WaitForSingleObject(meetPoint.mainSemaphore, INFINITE);

	chg_nb_process();

    return 0;
}
