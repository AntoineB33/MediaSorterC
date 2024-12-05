﻿#include <stdio.h>
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
    HANDLE* semaphore;
    ThreadEvent* threadEvents;
	int nb_process;
    char* sheetID;
    Node* nodes;
    int n;
    attributeSt* attributes;
    int nb_att;
    int error;
    ThreadParams* allParams;
    int* awaitingThrds;
    int nb_awaitingThrds;
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

inline void notChosenAnymore(HANDLE semaphore, int* befUpdErr, int* errorThrd, int* depth, int* lastSharedDepth, int nbItBefUpdErr, ReservationList* reservationLists, ReservationRule* reservationRules, int* busyNodes, int* ascending, int* error, int* nb_awaitingThrds, int n, int* awaitingThrds, ThreadParams* allParams, sharing* result, int rank, sharerResult* sharers, int* nb_process, ThreadEvent* threadEvents, GenericList* const rests, Node* nodes, const char* const sort_info_path, Node* node, int nodeId) {
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

inline int getNodes(HANDLE semaphore, int* befUpdErr, int* errorThrd, int* depth, int* lastSharedDepth, int nbItBefUpdErr, ReservationList* reservationLists, ReservationRule* reservationRules, int* busyNodes, int* ascending, int* error, int* nb_awaitingThrds, int n, int* awaitingThrds, ThreadParams* allParams, sharing* result, int rank, sharerResult* sharers, int* nb_process, ThreadEvent* threadEvents, GenericList* const rests, Node* nodes, const char* const sort_info_path) {
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
    if (nodesArray && cJSON_IsArray(nodesArray)) {
        int i = 0;
        cJSON* node;
        cJSON_ArrayForEach(node, nodesArray) {

			cJSON* ulteriorsArray = cJSON_GetObjectItem(node, "ulteriors");
			int ulteriorsSize = cJSON_GetArraySize(ulteriorsArray);
			initList(&(nodes[i].ulteriors), sizeof(int), ulteriorsSize);
            for (int j = 0; j < ulteriorsSize; j++) {
                int ulterior = cJSON_GetArrayItem(ulteriorsArray, j)->valueint;
                addElement(&(nodes[i].ulteriors), &ulterior);
            }

			cJSON* conditionsArray = cJSON_GetObjectItem(node, "conditions");
			int conditionsSize = cJSON_GetArraySize(conditionsArray);
			initList(&(nodes[i].conditions), sizeof(char*), conditionsSize);
			for (int j = 0; j < conditionsSize; j++) {
				const char* condition = cJSON_GetArrayItem(conditionsArray, j)->valuestring;
				addElement(&(nodes[i].conditions), &condition);
			}

			cJSON* allowedPlacesArray = cJSON_GetObjectItem(node, "allowedPlaces");
			int allowedPlacesSize = cJSON_GetArraySize(allowedPlacesArray);
			initList(&(nodes[i].allowedPlaces), sizeof(allowedPlaceSt), allowedPlacesSize);
			for (int j = 0; j < allowedPlacesSize; j++) {
				cJSON* allowedPlace = cJSON_GetArrayItem(allowedPlacesArray, j);
				allowedPlaceSt allowedPlaceSt;
				allowedPlaceSt.start = cJSON_GetObjectItem(allowedPlace, "start")->valueint;
				allowedPlaceSt.end = cJSON_GetObjectItem(allowedPlace, "end")->valueint;
				addElement(&(nodes[i].allowedPlaces), &allowedPlaceSt);
			}

			nodes[i].nbPost = cJSON_GetObjectItem(node, "nbPost")->valueint;
			i++;
        }
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

    // Cleanup
    cJSON_Delete(json);
    free(fileContent);

    fclose(file);

    unlockFile(&hFile, &ov);
}

inline void getPrevSort(HANDLE semaphore, int* befUpdErr, int* errorThrd, int* depth, int* lastSharedDepth, int nbItBefUpdErr, ReservationList* reservationLists, ReservationRule* reservationRules, int* busyNodes, int* ascending, int* error, int* nb_awaitingThrds, int n, int* awaitingThrds, ThreadParams* allParams, sharing* result, int rank, sharerResult* sharers, int* nb_process, ThreadEvent* threadEvents, GenericList* const rests, Node* nodes, const char* const sort_info_path) {
    HANDLE hFile;
    OVERLAPPED ov;

    lockFile(&hFile, &ov, sort_info_path);

    // Open the file for reading
    FILE* file = fopen(sort_info_path, "r");
    if (!file) {
        printf("Failed to open file for reading: %s\n", sort_info_path);
        unlockFile(&hFile, &ov);
        free(sort_info_path);
        return -1;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    // Read file content into a buffer
    char* fileContent = (char*)malloc(fileSize + 1);
    if (!fileContent) {
        printf("Failed to allocate memory for file content.\n");
        fclose(file);
        unlockFile(&hFile, &ov);
        free(sort_info_path);
        return -1; // Memory allocation error
    }
    fread(fileContent, 1, fileSize, file);
    fileContent[fileSize] = '\0'; // Null-terminate the JSON string
    fclose(file);


    // Parse JSON content
    cJSON* json = cJSON_Parse(fileContent);
    if (!json) {
        printf("Failed to parse JSON: %s\n", cJSON_GetErrorPtr());
        free(fileContent);
        free(sort_info_path);
        return -1; // JSON parsing error
    }

    HANDLE* eventHandlesThd = malloc(*nb_process * sizeof(HANDLE));
	for (int i = 0; i < *nb_process; i++) {
		eventHandlesThd[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
	}
    HANDLE semaphore;
    int error = INT_MAX;
    int nbAwaitingThrd = 0;
    GenericList* awaitingThrds;
    initList(awaitingThrds, sizeof(int), 1);
	cJSON* sheetsArray = cJSON_GetObjectItem(json, "sheets");
    if (sheetsArray && cJSON_IsArray(sheetsArray)) {
        int i = 0;
        cJSON* sheet;
        cJSON_ArrayForEach(sheet, sheetsArray) {
            cJSON* sheetName = cJSON_GetObjectItem(sheet, "sheetName");
            if (sheetName && cJSON_IsString(sheetName) && strcmp(sheetName->valuestring, sheetPath) != 0) {
                continue;
            }
            cJSON* threadsArray = cJSON_GetObjectItem(sheet, "threads");
            if (threadsArray && cJSON_IsArray(threadsArray)) {
				int threadsArraySize = cJSON_GetArraySize(threadsArray);
				initList(allParams, sizeof(ThreadParams), max(*nb_process, threadsArraySize));
                int j = 0;
                cJSON* thread;
                cJSON_ArrayForEach(thread, threadsArray) {
                    int* reservationListsResult = malloc(n * sizeof(int));
                    int* reservationListsElemResult = malloc(n * sizeof(int));
                    sharing* result = malloc(n * sizeof(sharing));
                    sharerResult* sharers = malloc(n * sizeof(int));
					cJSON* resultJS = cJSON_GetObjectItem(thread, "result");
                    int k = 0;
                    cJSON* resultElmnt;
                    if (cJSON_IsArray(resultJS)) {
                        cJSON_ArrayForEach(resultElmnt, resultJS) {
                            cJSON* resultAtt = cJSON_GetObjectItem(thread, "result");
                            cJSON* reservationListInd = cJSON_GetObjectItem(thread, "reservationListInd");
                            if (resultAtt && cJSON_IsNumber(resultAtt) && reservationListInd && cJSON_IsNumber(reservationListInd)) {
								result[k].result.result = resultAtt->valueint;
								result[k].result.reservationListInd = reservationListInd->valueint;
                            }
                            k++;
                        }
                    }
                    for (int m = k; m < n; m++) {
                        result[m].result.result = -1;
                    }
					cJSON* sharersJS = cJSON_GetObjectItem(thread, "sharers");
					k = 0;
					cJSON* sharer;
					if (cJSON_IsArray(sharersJS)) {
						cJSON_ArrayForEach(sharer, sharersJS) {
							cJSON* sharerAtt = cJSON_GetObjectItem(thread, "sharer");
							cJSON* resultAtt = cJSON_GetObjectItem(thread, "result");
							if (sharerAtt && cJSON_IsNumber(sharerAtt) && resultAtt && cJSON_IsNumber(resultAtt)) {
								sharers[k].sharer = sharerAtt->valueint;
								sharers[k].result.result = resultAtt->valueint;
							}
							k++;
						}
					}
                    ThreadParams params = {
                        .sort_info_path = sort_info_path,
                        .eventHandlesThd = eventHandlesThd,
                        .semaphore = &semaphore,
                        .sheetPath = sheetPath,
                        .nodes = nodes,
                        .attributes = attributes,
                        .n = n,
                        .fstSharedDepth = 0,
                        .currentLeaf = 0,
                        .error = &error,
                        .errorThrd = error,
                        .awaitingThrds = awaitingThrds,
                        .reservationListsResult = reservationListsResult,
                        .reservationListsElemResult = reservationListsElemResult,
                        .result = result,
                        .sharers = sharers,
                        .restoreUpTo = 0
                    };
					addElement(&allParams[j], &params);
                    j++;
                }
                for (int t = threadsArraySize; t < *nb_process; t++) {
                    int* reservationListsResult = malloc(n * sizeof(int));
                    int* reservationListsElemResult = malloc(n * sizeof(int));
                    sharing* result = malloc(n * sizeof(sharing));
                    sharerResult* sharers = malloc(n * sizeof(int));
                    for (int k = 0; k < n; k++) {
                        result[k].result.result = -1;
                    }
                    ThreadParams params = {
                        .sort_info_path = sort_info_path,
                        .eventHandlesThd = eventHandlesThd,
                        .semaphore = &semaphore,
                        .sheetPath = sheetPath,
                        .nodes = nodes,
                        .attributes = attributes,
                        .n = n,
                        .fstSharedDepth = 0,
                        .currentLeaf = 0,
                        .error = &error,
                        .errorThrd = error,
                        .awaitingThrds = awaitingThrds,
                        .reservationListsResult = reservationListsResult,
                        .reservationListsElemResult = reservationListsElemResult,
                        .result = result,
                        .sharers = sharers,
                        .restoreUpTo = 0
                    };
                    addElement(&allParams[t], &params);
                }
            }
		    break;
        }
    }
    unlockFile(&hFile, &ov);


    // Cleanup
    cJSON_Delete(json);
    free(fileContent);
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

inline int updateThrdAchiev(cJSON* sheetIDToBuffer, char* sheetID, int isCompleteSorting, int* error, cJSON* waitingSheets, cJSON* json, ThreadParams* allParams, int rank, int n) {
    int toUpdate = 0;
    cJSON* bufferJS = cJSON_GetObjectItemCaseSensitive(sheetIDToBuffer, sheetID);
    if (isCompleteSorting) {
        if (*error == 0) {
            cJSON_DeleteItemFromObject(bufferJS, "threads");
            cJSON_DeleteItemFromObject(bufferJS, "toUpdate");
            cJSON_DeleteItemFromObject(bufferJS, "updated");
            int i = 0;
            cJSON* waitingSheet;
            cJSON_ArrayForEach(waitingSheet, waitingSheets) {
                if (strcmp(waitingSheet->valuestring, sheetID) == 0) {
                    cJSON_DeleteItemFromArray(waitingSheets, i);
                    break;
                }
                i++;
            }
        }
        cJSON* toUpdateJS = cJSON_GetObjectItem(bufferJS, "toUpdate");
        toUpdate = toUpdateJS->valueint;
        if (toUpdate) {
            cJSON* sheetPathJS = cJSON_GetObjectItem(bufferJS, "sheetPath");
            char* sheetPath = sheetPathJS->valuestring;
            cJSON* sheetPathToSheetID = cJSON_GetObjectItem(json, "sheetPathToSheetID");
            cJSON* sheetIDJS = cJSON_GetObjectItem(sheetPathToSheetID, sheetPath);
            if (strcmp(sheetIDJS->valuestring, sheetID) != 0) {
                toUpdate = -1;
            }
        }
        if (toUpdate != -1) {
            cJSON* errorJS = cJSON_GetObjectItem(bufferJS, "error");
            cJSON_ReplaceItemInObject(bufferJS, "error", cJSON_CreateNumber(*error));
            cJSON* resultNodesArray = cJSON_GetObjectItem(bufferJS, "resultNodes");
            for (int j = 0; j < n; j++) {
                cJSON_ReplaceItemInArray(resultNodesArray, j, cJSON_CreateNumber(allParams[rank].result[j].nodeId));
            }
            if (toUpdate) {
                // call VBA
            }
        }
    }
    if (toUpdate != -1) {
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
                cJSON_ReplaceItemInObject(resultElmnt, "result", cJSON_CreateNumber(allParams[rank].result[k].result.result));
                cJSON_ReplaceItemInObject(resultElmnt, "reservationListInd", cJSON_CreateNumber(allParams[rank].result[k].result.reservationListInd));
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
                cJSON_ReplaceItemInObject(sharer, "sharer", cJSON_CreateNumber(allParams[rank].sharers[k].sharer));
                cJSON_ReplaceItemInObject(sharer, "result", cJSON_CreateNumber(allParams[rank].sharers[k].result.result));
            }
            k++;
        }
    }
}

inline void chkIfSameSheetID(int* stopAll, cJSON* waitingSheets, char* sheetID, cJSON* sheetIDToBuffer) {
    if (cJSON_GetArraySize(waitingSheets) == 0) {
        cJSON* waitingSheet = cJSON_GetArrayItem(waitingSheets, 0);
        *stopAll = strcmp(waitingSheet->valuestring, sheetID);
        if (*stopAll != 0) {
            cJSON* bufferJS = cJSON_GetObjectItemCaseSensitive(sheetIDToBuffer, sheetID);
            getNodes(sort_info_path, nodes, attributes, sheetPath, &n, &nb_att);
            getPrevSort(sort_info_path, allParams, nodes, attributes, nb_process, n, nb_att, sheetPath);
        }
    }
    else {
        *stopAll = 2;
        *sheetID = NULL;
    }
}

// Function to search and modify the content in the file
inline int modifyFile(char* sort_info_path, char* sheetID, int isCompleteSorting, HANDLE semaphore, int* befUpdErr, int* errorThrd, int* depth, int* lastSharedDepth, int nbItBefUpdErr, ReservationList* reservationLists, ReservationRule* reservationRules, int* busyNodes, int* ascending, int* error, int* nb_awaitingThrds, int n, int* awaitingThrds, ThreadParams* allParams, sharing* result, int rank, sharerResult* sharers, int* nb_process, ThreadEvent* threadEvents, GenericList* const rests, Node* nodes, int* stopAll) {

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

	*stopAll = 0;

	cJSON* nb_processJS = cJSON_GetObjectItem(json, "nb_process");
	int nb_process = nb_processJS->valueint;
    if (nb_process < nb_process) {
        if (rank >= nb_process) {
            allParams[rank].fstSharedDepth = -1;
            if (rank == nb_process - 1) {
                for (int i = nb_process - 2; i >= nb_process; i--) {
                    if (allParams[i].fstSharedDepth != -1) {
                        new_nb_process(i);
                        break;
                    }
                }
            }
            *stopAll = 2;
        }
	}
	else if (nb_process > nb_process) {
		new_nb_process(nb_process);
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
    if (*depth == lastSharedDepth) {
        WaitForSingleObject(semaphore, INFINITE); // Acquire the semaphore
        result[*depth].result = sharers[*depth].result;
    }

    if (reservationRules[*depth].resList == -1) {
        do {
            if (result[*depth].result.result == rests[*depth].size) {
                ascending = 0;
                break;
            }
            result[*depth].result.result++;
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
    if (*depth == lastSharedDepth) {
        allParams[sharers[*depth].sharer].sharers[*depth].result = result[*depth].result;
        ReleaseSemaphore(semaphore, 1, NULL); // Release the semaphore
    }
}

inline int updateThread(int* befUpdErr, HANDLE semaphore, int* errorThrd, int* error, int* depth, int* nb_awaitingThrds, int* lastSharedDepth, int n, int* awaitingThrds, ThreadParams* allParams, sharing* result, int rank, sharerResult* sharers, ThreadEvent* threadEvents, int nbItBefUpdErr) {
    (*befUpdErr)--;
    if (*befUpdErr == 0) {
        WaitForSingleObject(semaphore, INFINITE); // Acquire the semaphore
        error[n - 1] = *error;
        int stopAll;
        modifyFile();
        if (stopAll) {
            if (stopAll == 2) {
                ReleaseSemaphore(semaphore, 1, NULL); // Release the semaphore
                return 0;
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
        *befUpdErr = nbItBefUpdErr;
    }
	return 0;
}

// Recursive function to try all possible ways to sort the integer
DWORD WINAPI threadSort(LPVOID lpParam) {
    const int fstNbItBefUpdErr = 50;
    const int nbItBefUpdErr = 10000;


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
    int* awaitingThrds = meetPoint->awaitingThrds;
    int nb_awaitingThrds = meetPoint->nb_awaitingThrds;

    int rank = threadParamsRank->rank;

    ThreadParams* currentParams = &allParams[rank];
    int fstSharedDepth = currentParams->fstSharedDepth;
    sharing* result = currentParams->result;
    sharerResult* sharers = currentParams->sharers;


	int lastSharedDepth = currentParams->fstSharedDepth;
	int ascending = 1;
    int found = 0;
    int currentElement;
	int befUpdErr = fstNbItBefUpdErr;
    int startInd = 0;
    int depth = 0;
    int nodId;
	int* resultNodes = malloc(n * sizeof(int));
	int* const busyNodes = malloc(n * sizeof(int));
	GenericList* const rests = malloc(n * sizeof(GenericList));
    ReservationRule* const reservationRules = malloc((n + 1) * sizeof(ReservationRule));
    ReservationList* const reservationLists = malloc((n - 1) * sizeof(ReservationList));
	GenericList* const startExcl = malloc(n * sizeof(GenericList));
	GenericList* const endExcl = malloc(n * sizeof(GenericList));
    int* const errors = malloc(n * sizeof(int));
	errors[0] = 0;
    for (int i = 0; i < n + 1; i++) {
        busyNodes[i] = nodes[i].nbPost;
        initList(&rests[i], sizeof(int), 1);
        reservationRules[i].resList = -1;
    }
    for (int i = 0; i < n; i++) {
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
	int searchingRoot = 0;
	int* neighborRanks = malloc(n * sizeof(int));

	// all the threads except the first one get in a state of waiting for the first to share.
    if (rank != 0) {
        if (reservationRules[0].resList == -1) {
            result[0].result.result = rests[depth].size;
        }
		else {
			int placeInd = reservationRules[depth].resList;
            while (placeInd != -1) {
                placeInd = reservationLists[placeInd].nextListInd;
            }
            result[depth].result.result = reservationLists[placeInd].data.size;
            result[depth].result.reservationListInd = placeInd;
		}
    }

	if (updateThread(semaphore, &befUpdErr, &errorThrd, &depth, &lastSharedDepth, nbItBefUpdErr, reservationLists, reservationRules, busyNodes, &ascending, error, &nb_awaitingThrds, n, awaitingThrds, allParams, result, rank, sharers, eventHandlesThd)) {
		return;
    }
    // main loop
    while (1) {
		chooseNextOpt(threadParamsRank, depth, lastSharedDepth, semaphore, reservationRules, reservationLists, rests, &ascending, busyNodes);

        if (ascending == 1) {
            depth++;
            if (depth == n) {
                WaitForSingleObject(semaphore, INFINITE); // Acquire the semaphore
                if (error[n - 1] < *error) {
                    *error = error[n - 1];
                }
                else {
                    error[n - 1] = *error;
                }
                int stopAll;
                modifyFile();
				if (stopAll) {
					if (stopAll == 2) {
						ReleaseSemaphore(semaphore, 1, NULL); // Release the semaphore
                        return;
					}
                    depth = 0;
				}
                ReleaseSemaphore(semaphore, 1, NULL); // Release the semaphore
				befUpdErr = nbItBefUpdErr;
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
                WaitForSingleObject(semaphore, INFINITE); // Acquire the semaphore
                error[n - 1] = *error;
                addElement(awaitingThrds, &rank);
                ReleaseSemaphore(semaphore, 1, NULL); // Release the semaphore
                WaitForSingleObject(threadEvent[rank].eventHandlesThd, INFINITE);
                ResetEvent(threadEvent[rank].eventHandlesThd);
            }
			if (updateThread()) {
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

inline void chg_nb_process(int* nb_process, ThreadParams* allParams, ThreadEvent* threadEvents, int new_nb_process) {
	for (int i = nb_process; i < new_nb_process; i++) {
		allParams = realloc(allParams, (i + 1) * sizeof(ThreadParams));
		allParams[i].fstSharedDepth = 0;
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
        threadEvents = realloc(threadEvents, (i + 1) * sizeof(ThreadEvent));
	}
	nb_process = new_nb_process;
}

int main(int argc, char* argv[]) {
    const char* sort_info_path = "C:/Users/abarb/Documents/health/news_underground/mediaSorter/programs/data/sort_info.txt";


    MeetPoint meetPoint = {
		.sort_info_path = sort_info_path,
		.semaphore = CreateSemaphore(NULL, 1, 1, NULL),
		.threadEvents = NULL,
		.nb_process = 0,
		.sheetID = NULL,
		.nodes = NULL,
		.n = 0,
		.attributes = NULL,
		.nb_att = 0,
		.error = 0,
		.allParams = NULL,
		.awaitingThrds = NULL,
		.nb_awaitingThrds = 0
    };
    ThreadParamsRank allParamsRank = {
        .meetPoint = &meetPoint,
        .rank = 0
    };

	// Create a new thread
	chg_nb_process(&meetPoint.nb_process, meetPoint.allParams, meetPoint.threadEvents, 1);

	// Wait for all threads to finish
    int p;
    while (meetPoint.nb_process) {
        WaitForSingleObject(meetPoint.semaphore, INFINITE);
		for (int i = 0; i < meetPoint.nb_process; i++) {
			if (meetPoint.allParams[i].fstSharedDepth != -1) {
				p = i;
				break;
			}
		}
		ReleaseSemaphore(meetPoint.semaphore, 1, NULL);
        WaitForSingleObject(meetPoint.threadEvents[p].hThread, INFINITE);
    }

    return 0;
}
