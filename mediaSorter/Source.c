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


#define LOCK_FILE_PATH "C:/Users/abarb/Documents/health/news_underground/mediaSorter/programs/data/data.lock" // Lock file path


typedef struct {
    void* data;       // Pointer to the array (generic list)
    size_t size;      // Number of elements in the list
    size_t capacity;  // Capacity of the list
    size_t elemSize;  // Size of each element
} GenericList;

// Define a structure to represent the graph node
typedef struct {
	GenericList ulteriors;	 // List of integers that must come after
	char** conditions;	   // List of conditions that must be satisfied
	int conditions_size;   // Number of conditions
	int nbPost;            // Number of nodes that must come before
    int highest;        // Highest placement allowed for this integer
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
    int reservationListsResult;
	int reservationListsElemResult;
    int nodeId;
    int shared;
} sharing;

typedef struct {
	HANDLE* eventHandlesThd;
	HANDLE* semaphore;
    char* sheetPath;
	Node* nodes;
	int n;
	int startingDepth;
	int currentLeaf;
    int* error;
	int errorThrd;
    GenericList* awaitingThrds;
	GenericList* startExcl;
    GenericList* endExcl;
    int* reservationListsResult;
	int* reservationListsElemResult;
    sharing* result;
	int* sharers;
    int restoreUpTo;
} ThreadParams;

typedef struct {
    ThreadParams* allParams;
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

// Function to add an element to the list
void addElement(GenericList* list, void* element) {
    // Check if the list is full and resize if needed
    if (list->size == list->capacity) {
        list->capacity *= 2;
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

void removeInd(GenericList* list, int ind) {
	// Shift the elements after the index to the left
	for (int i = ind; i < list->size - 1; i++) {
		void* src = (char*)list->data + (i + 1) * list->elemSize;
		void* dst = (char*)list->data + i * list->elemSize;
		memcpy(dst, src, list->elemSize);
	}
	list->size--;  // Reduce the size after removing the element
}

// Function to free the list
void freeList(GenericList* list) {
    free(list->data);
    list->size = 0;
    list->capacity = 0;
}

void freeNode(Node* node) {
    freeList(&node->ulteriors);
    for (int i = 0; i < node->conditions_size; i++) {
        free(node->conditions[i]);
		freeList(&node->ulteriors);
    }
    free(node->conditions);
}

void freeNodes(Node* nodes, size_t nodeCount) {
    for (size_t i = 0; i < nodeCount; i++) {
        freeNode(&nodes[i]);
    }
    free(nodes);
}

void parseJSONToNode(Node** nodes, int* n, const char* filename) {
    // Open the JSON file
    FILE* file = fopen("C:/Users/abarb/Documents/health/news_underground/mediaSorter/programs/data/data.json", "r");
    if (file == NULL) {
        perror("Unable to open file");
        return;
    }


    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Read the file into a string
    char* json_data = (char*)malloc(file_size + 1);
    fread(json_data, 1, file_size, file);
    fclose(file);
    json_data[file_size] = '\0';

    // Parse JSON data
    cJSON* json = cJSON_Parse(json_data);
    if (json == NULL) {
        printf("Error parsing JSON\n");
        free(json_data);
        return;
    }

    *n = cJSON_GetArraySize(json);
    *nodes = malloc(*n * sizeof(Node));
    for (size_t i = 0; i < *n; i++) {
        Node* node = &(*nodes)[i];
        cJSON* nodeJson = cJSON_GetArrayItem(json, i);

        // Parse ulteriors array
        cJSON* ulteriorsJson = cJSON_GetObjectItem(nodeJson, "ulteriors");
        int ulteriorsCount = (int)cJSON_GetArraySize(ulteriorsJson);
        initList(&node->ulteriors, sizeof(int), ulteriorsCount);
        for (int j = 0; j < ulteriorsCount; j++) {
            int ulterior = cJSON_GetArrayItem(ulteriorsJson, j)->valueint;
            addElement(&node->ulteriors, &ulterior);
        }

        // Parse conditions array
        cJSON* conditionsJson = cJSON_GetObjectItem(nodeJson, "conditions");
        node->conditions_size = cJSON_GetArraySize(conditionsJson);
        node->conditions = (char**)malloc(node->conditions_size * sizeof(char*));
        for (int j = 0; j < node->conditions_size; j++) {
            const char* condition = cJSON_GetArrayItem(conditionsJson, j)->valuestring;
            node->conditions[j] = _strdup(condition);
        }

        // Parse nbPost and highest
        node->nbPost = cJSON_GetObjectItem(nodeJson, "nbPost")->valueint;
        node->highest = cJSON_GetObjectItem(nodeJson, "highest")->valueint;
    }

    cJSON_Delete(json);
}

void write_to_file(const char* filename, const char* message) {
    // Open file for writing
    HANDLE file = CreateFile(filename, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Error opening file\n");
        return;
    }

    // Lock the file for exclusive access
    OVERLAPPED overlap = { 0 };
    if (!LockFileEx(file, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &overlap)) {
        fprintf(stderr, "Error locking file\n");
        CloseHandle(file);
        return;
    }

    // Move the file pointer to the end for appending
    SetFilePointer(file, 0, NULL, FILE_END);

    // Write the message to the file
    DWORD written;
    WriteFile(file, message, strlen(message), &written, NULL);
    WriteFile(file, "\n", 1, &written, NULL); // Add newline

    // Unlock and close the file
    UnlockFileEx(file, 0, MAXDWORD, MAXDWORD, &overlap);
    CloseHandle(file);
}

void trigger_python_script(const char* script_path, const char* arg) {
    // Construct the command to run the Python script with the argument
    char command[512];  // Adjust buffer size if needed
    snprintf(command, sizeof(command), "python3 %s %s", script_path, arg);

    // Use the system() function to execute the command
    int status = system(command);
    if (status == -1) {
        perror("Error executing Python script");
    }
    else {
        printf("Python script triggered successfully with argument: %s\n", arg);
    }
}

void notChosenAnymore(Node* nodes, int n, int i, ReservationRule* reservationRules, ReservationList* reservationLists) {
    if (nodes[i].highest != -1) {
        int highest = nodes[i].highest;
        int resListInd = reservationRules[highest].resList;
        if (resListInd == -1) {
            reservationRules[highest].resList = highest;
            reservationRules[highest].all = 0;
            reservationLists[highest].out = 1;
            reservationLists[highest].prevListInd = highest - 1;
            reservationLists[highest].nextListInd = -1;
            addElement(&(reservationLists[highest].data), &i);
        }
        else {
            int resThatWasThere = reservationRules[highest].resList;
            if (resThatWasThere != highest) {
                reservationLists[highest].prevListInd = reservationLists[resThatWasThere].prevListInd;
                reservationLists[resThatWasThere].prevListInd = highest;
                reservationLists[resThatWasThere].out = 0;
                reservationLists[highest].nextListInd = resThatWasThere;
            }
            addElement(&(reservationLists[highest].data), &i);
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

// Utility function to check if all integers have been sorted
int all_sorted(int* sorted, int n) {
    for (int i = 0; i < n; i++) {
        if (!sorted[i]) {
            return 0;
        }
    }
    return 1;
}

int check_conditions(Node* nodes, int n, sharing* result, int depth) {
	int is_true = 1;
	for (int condInd = 0; condInd < nodes[result[depth].nodeId].conditions_size; condInd++) {
        is_true = 0;
	}
	return is_true;
}

// Function to check if the file is locked
int isFileLocked() {
    FILE* lockFile = fopen(LOCK_FILE_PATH, "r");
    if (lockFile) {
        fclose(lockFile);
        return 1; // File is locked
    }
    return 0; // File is not locked
}

// Function to acquire the lock
void acquireLock() {
    if (isFileLocked()) {
        fprintf(stderr, "File is locked by another process.\n");
        exit(1);
    }
    FILE* lockFile = fopen(LOCK_FILE_PATH, "w");
    if (lockFile) {
        fprintf(lockFile, "LOCK");
        fclose(lockFile);
    }
}

// Function to release the lock
void releaseLock() {
    remove(LOCK_FILE_PATH);
}

// Function to search and modify the content in the file
void modifyFile(const char* const sheetPath, int* const error, int* const result, ReservationList* const reservationLists, int* const reservationListsResult, int* const reservationListsElemResult, int* const resultNodes) {
    const char* filePath = "C:/Users/abarb/Documents/health/news_underground/mediaSorter/programs/data/sort_info.txt";

    // Open the file in read mode
    FILE* file = fopen(filePath, "r");
    if (!file) {
        perror("Failed to open file");
        return;
    }

    // Get the file size and read the content
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* content = malloc(fileSize + 1);
    if (!content) {
        perror("Failed to allocate memory");
        fclose(file);
        return;
    }
    fread(content, 1, fileSize, file);
    content[fileSize] = '\0';  // Null-terminate the content
    fclose(file);

    // Parse the JSON data
    cJSON* json = cJSON_Parse(content);
    if (json == NULL) {
        fprintf(stderr, "Error parsing JSON: %s\n", cJSON_GetErrorPtr());
        free(content);
        return;
    }

    cJSON* boolean1 = cJSON_GetObjectItem(json, "complexList");
	if (boolean1 && cJSON_IsBool(boolean1)) {
        if (cJSON_IsTrue(boolean1)) {
			*error = 1;
        }
    }
    else {
		perror("boolean1 not found.\n");
        return;
    }

    // Get the "complexList" array
    cJSON* complexList = cJSON_GetObjectItem(json, "complexList");
    if (complexList && cJSON_IsArray(complexList)) {
        // Iterate through each item in the "complexList"
        cJSON* item = NULL;
        cJSON_ArrayForEach(item, complexList) {
            cJSON* str = cJSON_GetObjectItem(item, "str");
            if (str && cJSON_IsString(str) && strcmp(str->valuestring, "example4") == 0) {
                // Modify the listOfIntLists for this element
                cJSON* listOfIntLists = cJSON_GetObjectItem(item, "listOfIntLists");
                if (listOfIntLists && cJSON_IsArray(listOfIntLists)) {
                    // Modify the first sublist in listOfIntLists as an example
                    cJSON* firstSublist = cJSON_GetArrayItem(listOfIntLists, 0);
                    if (firstSublist && cJSON_IsArray(firstSublist)) {
                        cJSON* newValue = cJSON_CreateNumber(999);  // Modify a value (example)
                        cJSON_AddItemToArray(firstSublist, newValue);
                    }
                }
            }
        }
    }
    else {
        printf("complexList not found or is not an array.\n");
    }

    // Convert the modified JSON back to a string
    char* modifiedContent = cJSON_Print(json);
    if (!modifiedContent) {
        fprintf(stderr, "Error printing JSON\n");
        cJSON_Delete(json);
        free(content);
        return;
    }

    // Write the modified content back to the file
    file = fopen(filePath, "w");
    if (!file) {
        perror("Failed to open file for writing");
        cJSON_Delete(json);
        free(content);
        free(modifiedContent);
        return;
    }
    fputs(modifiedContent, file);
    fclose(file);

    // Cleanup
    cJSON_Delete(json);
    free(content);
    free(modifiedContent);
}

// Recursive function to try all possible ways to sort the integer
DWORD WINAPI threadSort(LPVOID lpParam) {
    const int nbItBefUpdErr = 10000;

    // Cast lpParam to the appropriate structure type
    ThreadParamsRank* threadParamsRank = (ThreadParamsRank*)lpParam;
	int rank = threadParamsRank->rank;
	ThreadParams* allParams = threadParamsRank->allParams;
	ThreadParams* params = &allParams[rank];
	HANDLE* eventHandlesThd = params->eventHandlesThd;
	HANDLE semaphore = *params->semaphore;
	const char* const sheetPath = params->sheetPath;
	const Node* const nodes = params->nodes;
	const int n = params->n;
	int startDepthThrd = params->startingDepth;
	int currentLeaf = params->currentLeaf;
	int* const error = params->error;
    int errorThrd = params->errorThrd;
    GenericList* const awaitingThrds = params->awaitingThrds;
    GenericList* const startExcl = params->startExcl;
    GenericList* const endExcl = params->endExcl;
	int* const reservationListsResult = params->reservationListsResult;
	int* const reservationListsElemResult = params->reservationListsElemResult;
	sharing* const result = params->result;
	int* const sharers = params->sharers;
	int restoreUpTo = params->restoreUpTo;

    int placeInd;
    int choiceInd;
	int ascending = 0;
    int found = 0;
    int currentElement;
	int befUpdErr = 1;
    int startInd = 0;
    int depth = 0;
    int nodId;
	int* resultNodes = malloc(n * sizeof(int));
	int* const busyNodes = malloc(n * sizeof(int));
	GenericList* const rests = malloc(n * sizeof(GenericList));
    ReservationRule* const reservationRules = malloc((n + 1) * sizeof(ReservationRule));
    ReservationList* const reservationLists = malloc((n - 1) * sizeof(ReservationList));
    int* const errors = malloc(n * sizeof(int));
	errors[0] = 0;
    for (int i = 0; i < n + 1; i++) {
        busyNodes[i] = nodes[i].nbPost;
        initList(&rests[i], sizeof(int), 1);
        reservationRules[i].resList = -1;
    }
    for (int i = 0; i < n; i++) {
        if (busyNodes[i] == 0 && check_conditions(nodes, n, resultNodes, 0, i)) {
			addElement(&(rests[0]), &i);
        }
    }
    for (int i = 0; i < n - 1; i++) {
        initList(&(reservationLists[i].data), sizeof(int), 1);
        notChosenAnymore(nodes, n, i, reservationRules, reservationLists);
    }
	int searchingRoot = 0;
	int* neighborRanks = malloc(n * sizeof(int));
    while (1) {

        // if the tree is shared with another thread at this depth
        if (result[depth].shared) {
            WaitForSingleObject(semaphore, INFINITE); // Acquire the semaphore
            result[depth].result = allParams[sharers[depth]].result[depth].result;
            allParams[sharers[depth]].result[depth].result++;
            ReleaseSemaphore(semaphore, 1, NULL); // Release the semaphore
        }

        if (reservationRules[depth].resList == -1) {
            do {
                if (result[depth].result == rests[depth].size) {
                    ascending = 0;
                    break;
                }
                result[depth].result++;
                result[depth].nodeId = ((int*)rests[depth].data)[result[depth].result];
                if (check_conditions(nodes, n, result, depth)) {
                    break;
                }
            } while (1);
        }
        else {
            found = 0;
            if (result[depth].result == -1) {
                placeInd = reservationRules[depth].resList;
                choiceInd = 0;
            }
            else {
                placeInd = result[depth].result;
                choiceInd = result[depth].reservationListsElemResult + 1;
            }
            result[depth].result = -1;
            do {
                for (int i = choiceInd; i < reservationLists[placeInd].data.size; i++) {
                    result[depth].nodeId = ((int*)reservationLists[placeInd].data.data)[i];
                    if (nodes[result[depth].nodeId].nbPost == 0 && check_conditions(nodes, n, resultNodes, depth) == 1) {
                        result[depth].result = placeInd;
                        result[depth].reservationListsElemResult = i;
                        found = 1;
                        break;
                    }
                }
                if (found) {
                    break;
                }
                if (reservationRules[depth].all != 0) {
                    break;
                }
                choiceInd = 0;
                placeInd = reservationLists[placeInd].nextListInd;
            } while (placeInd != -1);
            if (!found) {
                ascending = 0;
            }
        }
        if (ascending == 1) {
            if (depth == n) {
                WaitForSingleObject(semaphore, INFINITE); // Acquire the semaphore
                // Open the file in write mode
                FILE* file = fopen("C:/Users/abarb/Documents/health/news_underground/mediaSorter/programs/data/overwatch_order.txt", "w");

                int* nodIds = malloc(n * sizeof(int));
                for (int i = 0; i < n; i++) {
                    if (reservationListsResult[i] == -1) {
                        nodId = ((int*)rests[i].data)[result[i]];
                    }
                    else {
                        nodId = ((int*)reservationLists[reservationListsResult[i]].data.data)[reservationListsElemResult[i]];
                    }
                    nodIds[i] = nodId;
                    fprintf(file, "%d\n", nodId);
                    printf("%d ", nodId);
                }
                printf("\n\n\n");
				modifyFile(sheetPath, error, result, reservationLists, reservationListsResult, reservationListsElemResult, nodIds);
				errorThrd = *error;
                ReleaseSemaphore(semaphore, 1, NULL); // Release the semaphore
                befUpdErr = nbItBefUpdErr;
				ascending = 0;
            } else {
                // ********* update the remaining elements *********

                // copy the remaining indexes before the chosen one
                for (int i = 0; i < result[depth - 1]; i++) {
                    addElement(&(rests[depth]), &((int*)rests[depth - 1].data)[i]);
                }

                // copy the remaining indexes after the chosen one
                for (int i = result[depth - 1]; i < rests[depth - 1].size - 1; i++) {
					addElement(&(rests[depth]), &((int*)rests[depth - 1].data)[i + 1]);
                }


                // ********* consequences of the choice in previous depth *********

                nodId = ((int*)rests[depth - 1].data)[result[depth - 1]];
                busyNodes[nodId]++;

                // update the reservation rules
                if (nodes[nodId].highest != -1) {
                    int highest = nodes[nodId].highest;
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
                for (int j = 0; j < nodes[nodId].ulteriors.size; j++) {
                    int element = ((int*)nodes[nodId].ulteriors.data)[j];
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

                result[depth] = rests[depth].size;
            }
            depth++;
        }
        if (ascending == 0) {
            ascending = 1;
			befUpdErr--;
			if (befUpdErr == 0) {
                WaitForSingleObject(semaphore, INFINITE); // Acquire the semaphore
                errorThrd = *error;
				modifyFile(sheetPath, error, result, reservationLists, reservationListsResult, reservationListsElemResult, resultNodes);
                if (awaitingThrds->size > 0) {
                    removeInd(awaitingThrds, awaitingThrds->size - 1);
                    ReleaseSemaphore(semaphore, 1, NULL); // Release the semaphore
                    startDepthThrd++;
					int otherRank = ((int*)awaitingThrds->data)[awaitingThrds->size];
                    ThreadParams* otherParams;
					otherParams = &allParams[otherRank];
                    for (int i = 0; i < startDepthThrd; i++) {
                        otherParams->result[i].result = result[i].result;
                    }
					otherParams->startingDepth = startDepthThrd;
                    otherParams->result[startDepthThrd].shared = 1;
                    otherParams->sharers[startDepthThrd] = rank;
					SetEvent(eventHandlesThd[otherRank]);
                }
                else {
					ReleaseSemaphore(semaphore, 1, NULL); // Release the semaphore
                }
				befUpdErr = nbItBefUpdErr;
			}
            if (depth == startDepthThrd) {
                if (nbLeaves!=0) {
                    int restLeaves2 = nbLeaves % nb_process;
                    if (restLeaves2 <= restLeaves) {
                        if (nbLeaves > nbLeavesMin && restLeaves2 <= repartTol * nb_process) {
                            return 0;
                        }
                        *startsSize = nbLeaves;
                        *end = endI;
                        if (restLeaves2 == 0) {
                            return 0;
                        }
                        restLeaves = restLeaves2;
                    }
                    nbLeaves = 0;
                    if (endI == n) {
                        return 0;
                    }
                    endI++;
                }
                result[0] = restsSizes[0];
            } 
            else {
				if (reservationListsResult[depth - 1] == -1) {
                    nodId = rests[depth - 1][result[depth - 1]];
				}
				else {
                    nodId = ((int*)reservationLists[reservationListsResult[depth - 1]].data.data)[reservationListsElemResult[depth - 1]];
				}
                nodes[nodId].nbPost--;
                notChosenAnymore(nodes, n, nodId, reservationRules, reservationLists);
                for (int j = 0; j < nodes[nodId].ulteriors.size; j++) {
                    int ulterior = ((int*)nodes[nodId].ulteriors.data)[j];
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

char* concatenate(const char* str1, const char* str2) {
    // Calculate the length of the new string
    size_t len1 = strlen(str1);
    size_t len2 = strlen(str2);
    size_t total_length = len1 + len2 + 1; // +1 for the null terminator

    // Allocate memory for the new string
    char* result = (char*)malloc(total_length * sizeof(char));
    if (result == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1); // Exit if allocation fails
    }

    // Copy the first string into the result with buffer size `total_length`
    strcpy_s(result, total_length, str1);
    // Concatenate the second string into result with buffer size `total_length`
    strcat_s(result, total_length, str2);

    return result; // Return the concatenated string
}

void checkForStopSignal() {
    // Could be a named pipe, event handle, or a file status check
    HANDLE stopEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, "StopEvent");
    if (stopEvent && WaitForSingleObject(stopEvent, 0) == WAIT_OBJECT_0) {
        stopRequested = true;
        CloseHandle(stopEvent);
    }
}

int main(int argc, char* argv[]) {


	// TODO : Check if sortings of similar systems already exist

    // Define the parameters
    int nbLeavesMin = 100; // Define the minimum time to sort
    int nbLeavesMax = 1000; // Define the maximum time to sort
    float repartTol = 0.1f; // Define the repartition tolerance


    // example of nodes, startExcl, endExcl and attributeMngs

    int n; // Define the number of integers to sort
    int nb_att = 0; // Define the number of attributes

    // Dynamically allocate memory for the array of nodes
    Node* nodes;



    Node node;
    char* path = concatenate("C:/Users/abarb/Documents/health/news_underground/mediaSorter/programs/data/", argv[1]);
    path = concatenate(path, ".json");
    parseJSONToNode(&nodes, &n, path);


    for (size_t i = 0; i < n; i++) {
        Node* node = &nodes[i];
        printf("Node %zu:\n", i + 1);

        printf("  Ulteriors: ");
        for (size_t j = 0; j < node->ulteriors.size; j++) {
            int* value = (int*)((char*)node->ulteriors.data + j * sizeof(int));
            printf("%d ", *value);
        }
        printf("\n");

        printf("  Conditions: ");
        for (int j = 0; j < node->conditions_size; j++) {
            printf("%s ", node->conditions[j]);
        }
        printf("\n");

        printf("  nbPost: %d\n", node->nbPost);
        printf("  highest: %d\n", node->highest);
    }

    /*freeNodes(nodes, n);
    return 0;*/


    GenericList* startExcl = malloc((n + 1) * sizeof(GenericList));
    GenericList* endExcl = malloc((n + 1) * sizeof(GenericList));
    AttributeMng* attributeMngs = malloc(nb_att * sizeof(AttributeMng));

    /*nodes[0].highest = -1;
    initList(&(nodes[0].ulteriors), sizeof(int), 1);
    nodes[0].conditions_size = 0;
    nodes[0].conditions = malloc(0 * sizeof(char*));

    nodes[1].highest = -1;
    initList(&(nodes[1].ulteriors), sizeof(int), 1);
    int value = 0;
	addElement(&(nodes[1].ulteriors), &value);
    nodes[1].conditions_size = 0;
    nodes[1].conditions = malloc(0 * sizeof(char*));

    nodes[2].highest = -1;
    initList(&(nodes[2].ulteriors), sizeof(int), 1);
    value = 0;
    addElement(&(nodes[2].ulteriors), &value);
    nodes[2].conditions_size = 0;
    nodes[2].conditions = malloc(0 * sizeof(char*));

    nodes[3].highest = -1;
    initList(&(nodes[3].ulteriors), sizeof(int), 1);
	value = 1;
	addElement(&(nodes[3].ulteriors), &value);
	value = 2;
	addElement(&(nodes[3].ulteriors), &value);
    nodes[3].conditions_size = 0;
    nodes[3].conditions = malloc(0 * sizeof(char*));*/

    for (int i = 0; i < n + 1; i++) {
        initList(&(startExcl[i]), sizeof(int), 1);
        initList(&(endExcl[i]), sizeof(int), 1);
    }


    for (int i = 0; i < n; i++) {
        nodes[i].nbPost = 0;
        for (int j = 0; j < nodes[i].ulteriors.size; j++) {
            int ulterior = ((int*)nodes[i].ulteriors.data)[j];
            nodes[ulterior].nbPost++;
        }
    }


    int nb_process = 1;

	ThreadParams* allParams = malloc(nb_process * sizeof(ThreadParams));
    for (int i = 0; i < nb_process; i++) {
        HANDLE* eventHandlesThd = malloc(nb_process * sizeof(HANDLE));
        HANDLE semaphore;
        int error = INT_MAX;
        sharing* nextLeafToUseList = malloc(nb_process * sizeof(sharing));
        int nbAwaitingThrd = 0;
        GenericList* awaitingThrds;
		initList(awaitingThrds, sizeof(int), 1);
        GenericList* startExclThrd = malloc(nb_process * sizeof(GenericList));
        GenericList* endExclThrd = malloc(nb_process * sizeof(GenericList));
        for (int i = 0; i < nb_process; i++) {
            nextLeafToUseList[i].shared = 0;
            initList(&(startExclThrd[i]), sizeof(int), 1);
            initList(&(endExclThrd[i]), sizeof(int), 1);
        }
        int* reservationListsResult = malloc(n * sizeof(int));
        int* reservationListsElemResult = malloc(n * sizeof(int));
        sharing* result = malloc(n * sizeof(sharing));
		int* sharers = malloc(n * sizeof(int));
		for (int i = 0; i < n; i++) {
			result[i].shared = 0;
			result[i].result = 0;
		}
        ThreadParams params = {
            .eventHandlesThd = eventHandlesThd,
            .semaphore = &semaphore,
            .sheetPath = argv[1],
            .nodes = nodes,
            .n = n,
            .startingDepth = 0,
            .currentLeaf = 0,
            .error = &error,
            .errorThrd = error,
            .awaitingThrds = awaitingThrds,
            .startExcl = startExcl,
            .endExcl = endExcl,
            .reservationListsResult = reservationListsResult,
            .reservationListsElemResult = reservationListsElemResult,
            .result = result,
			.sharers = sharers,
            .restoreUpTo = 0
        };
        allParams[i] = params;
        ThreadParamsRank allParamsRank = {
            .allParams = allParams,
            .rank = i
        };
        // Create thread
        HANDLE hThread = CreateThread(NULL, 0, threadSort, &allParamsRank, 0, NULL);
        if (hThread == NULL) {
            printf("Error creating thread %d\n", 0);
            return 1;
        }
    }

    // Wait for threads to complete
	WaitForMultipleObjects(nb_process, allParams[0].eventHandlesThd, TRUE, INFINITE);




    
    freeNodes(nodes, n);

    call_python_function("switchSort", argv[1], NULL, 0);

    return 0;
}
