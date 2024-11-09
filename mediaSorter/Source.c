#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"  // Include the cJSON header
#include <curl/curl.h>
#include <stdarg.h>

//#include <windows.h>
//#include <oleauto.h>
//#include <comutil.h>
////#include <time.h>

#include <Python.h>


#define TYPE_END 0
#define TYPE_STRING 1
#define TYPE_LIST_INT 2


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
    int nbPost;         // Count of integers that must be placed before this
    int highest;        // Highest placement allowed for this integer
} Node;

typedef struct {
    int all;
    int resList;
} ReservationRule;

typedef struct {
    GenericList data;
    int out;
    int prevListInd;
    int nextListInd;
} ReservationList;

typedef struct {
    int lastOcc;
} AttributeMng;


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

void chosenUlt(Node* nodes, int rank, int depth, int*** rests, int** restsSizes, int i) {
    nodes[i].nbPost--;
    if (nodes[i].nbPost == 0) {
        rests[rank][depth][restsSizes[rank][depth]] = i;
        restsSizes[rank][depth]++;
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

int check_conditions(Node* nodes, int* result, int n, int i) {
	//for (int condInd = 0; condInd < nodes[i].conditions_size; condInd++) {
	//	// Check if the condition is satisfied
	//}
	return 1;
}

void removeFromRest(Node* nodes, int rank, int depth, int element, int*** rests, int** restsSizes) {
    nodes[element].nbPost++;
    if (nodes[element].nbPost == 1) {
        int k = restsSizes[rank][depth] - 1;
        while (1) {
            if (rests[rank][depth][k] == element) {
                restsSizes[rank][depth]--;
                for (int m = k; m < restsSizes[rank][depth]; m++) {
                    rests[rank][depth][m] = rests[rank][depth][m + 1];
                }
                break;
            }
            k--;
        }
    }
}

// Function that takes two strings and a list of integers and calls a Python function
void call_python_function(const char* str1, const char* str2, int* int_list, size_t list_size) {
    // Initialize the Python interpreter
    Py_Initialize();

    // Add this before importing the module
    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append('C:/Users/abarb/Documents/health/news_underground/mediaSorter/programs/excel_prog/mediaSorter')");  // Update this path

    // Prepare the arguments for the Python function
    PyObject* pName, * pModule, * pFunc, * pArgs, * pList;

    // Convert C strings to Python strings
    PyObject* pyStr1 = PyUnicode_FromString(str1);
    PyObject* pyStr2 = PyUnicode_FromString(str2);

    // Create a Python list for the integers
    pList = PyList_New(list_size);
    for (size_t i = 0; i < list_size; ++i) {
        PyObject* pyInt = PyLong_FromLong(int_list[i]);
        PyList_SetItem(pList, i, pyInt);  // Note: PyList_SetItem steals reference
    }

    // Load the Python module
    pName = PyUnicode_FromString("callVBA");  // Replace with your module name
    pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (pModule != NULL) {
        // Get the reference to the Python function
        pFunc = PyObject_GetAttrString(pModule, "call_vba_function");  // Replace with your function name

        if (pFunc && PyCallable_Check(pFunc)) {
            // Create a tuple to hold the arguments
            pArgs = PyTuple_Pack(3, pyStr1, pyStr2, pList);
            // Call the Python function without caring for the return value
            PyObject_CallObject(pFunc, pArgs);
            Py_DECREF(pArgs);
        }
        else {
            if (PyErr_Occurred())
                PyErr_Print();
            fprintf(stderr, "Cannot find function \"%s\"\n", "call_vba_function");
        }

        Py_XDECREF(pFunc);
        Py_DECREF(pModule);
    }
    else {
        PyErr_Print();
        fprintf(stderr, "Failed to load \"%s\"\n", "callVBA");
    }

    // Clean up
    Py_DECREF(pyStr1);
    Py_DECREF(pyStr2);
    Py_DECREF(pList);

    // Finalize the Python interpreter
    Py_Finalize();
}

// Recursive function to try all possible ways to sort the integer
int try_sort(char* sheetCodeName,int nb_process, int nbItBefUpdErr, int nbLeavesMin, int nbLeavesMax, float repartTol, Node* nodes, GenericList* startExcl, GenericList* endExcl, int** result, int* errors, int** reservationListsResult, int** reservationListsElemResult, int*** rests, int** restsSizes, ReservationRule** reservationRules, ReservationList** reservationLists, int n, int reste, int* error, int mpiManagement, int* end, int rank, int* startsSize) {
    int placeInd;
    int choiceInd;
	int ascending = 0;
    int found = 0;
    int currentElement;
	int befUpdErr = nbItBefUpdErr;
	int errorCopy = *error;
    int nbLeaves = 0;
	int startStarts = *startsSize * rank / nb_process + min(reste, rank);
    int endStarts = *startsSize * (rank + 1) / nb_process + min(reste, rank + 1);
    int startInd = 0;
    int depth = 0;
    int endI = *end;
    int restLeaves = nb_process;
    int nodId;
    while (1) {
        if (ascending == 1) {
            depth++;
            restsSizes[rank][depth] = restsSizes[rank][depth - 1] - 1;
            for (int i = 0; i < result[rank][depth - 1]; i++) {
                rests[rank][depth][i] = rests[rank][depth - 1][i];
            }
            for (int i = result[rank][depth - 1] + 1; i < restsSizes[rank][depth - 1]; i++) {
                rests[rank][depth][i - 1] = rests[rank][depth - 1][i];
            }
            nodId = rests[rank][depth - 1][result[rank][depth - 1]];
            nodes[nodId].nbPost++;

            // what choosing nodes[i] implies
            if (nodes[nodId].highest != -1) {
                int highest = nodes[nodId].highest;
                int placeInd = highest;
                int placeIndPrev;
                do {
                    placeIndPrev = placeInd;
                    placeInd = reservationLists[rank][placeIndPrev].prevListInd;
                } while (reservationLists[rank][placeIndPrev].out == 0 && (placeIndPrev != highest || placeIndPrev - placeInd >= reservationLists[rank][placeIndPrev].data.size));
				placeInd++;
                if(placeIndPrev == highest) {
                    if (reservationRules[rank][placeInd].all == 1) {
                        int placeInd2 = placeIndPrev - reservationLists[rank][placeIndPrev].data.size + 1;
                        reservationRules[rank][placeInd2].all = 1;
                    }
                    do {
                        placeIndPrev = placeInd;
                        placeInd = reservationLists[rank][placeIndPrev].prevListInd;
                    } while (reservationLists[rank][placeIndPrev].out == 0);
                }
                placeInd++;
                reservationRules[rank][placeInd].resList = -1;
                reservationLists[rank][placeIndPrev].prevListInd++;
                placeInd++;
                if (placeInd<= highest && reservationRules[rank][placeInd].all == 0) {
                    do {
                        placeIndPrev = placeInd;
                        placeInd = reservationLists[rank][placeInd].nextListInd;
                    } while (placeInd != highest && reservationLists[rank][placeInd].data.size == placeInd - reservationLists[rank][placeInd].prevListInd);
                    reservationLists[rank][placeIndPrev].nextListInd = -1;
                    reservationLists[rank][placeInd].out = 1;
                }
                removeElement(&(reservationLists[rank][highest].data), nodId);
            }
            for (int j = 0; j < nodes[nodId].ulteriors.size; j++) {
                chosenUlt(nodes, rank, depth, rests, restsSizes, ((int*)nodes[nodId].ulteriors.data)[j]);
            }

            // what being at this depth implies
            for (int i = 0; i < startExcl[depth].size; i++) {
                int element = ((int*)endExcl[depth].data)[i];
                removeFromRest(nodes, rank, depth, element, rests, restsSizes);
            }
            for (int i = 0; i < endExcl[depth].size; i++) {
                chosenUlt(nodes, rank, depth, rests, restsSizes, ((int*)startExcl[depth].data)[i]);
            }

            result[rank][depth] = restsSizes[rank][depth];

            if (depth == n) {

                // Open the file in write mode
                FILE* file = fopen("C:/Users/abarb/Documents/health/news_underground/mediaSorter/programs/data/overwatch_order.txt", "w");

				int* nodIds = malloc(n * sizeof(int));
                for (int i = 0; i < n; i++) {
                    if (reservationListsResult[rank][i] == -1) {
                        nodId = rests[rank][i][result[rank][i]];
                    }
                    else {
                        nodId = ((int*)reservationLists[rank][reservationListsResult[rank][i]].data.data)[reservationListsElemResult[rank][i]];
                    }
					nodIds[i] = nodId;
                    fprintf(file, "%d\n", nodId);
                    printf("%d ", nodId);
                }
                printf("\n\n\n");
                call_python_function("SwapRows", sheetCodeName, nodIds, n);
            }
            if (depth == endI) {
                if (mpiManagement == 0) {
                    if (startInd == endStarts) {
                        return 0;
                    }
                    if (startInd < startStarts) {
                        ascending = 0;
                    }
                    startInd++;
                }
                else {
                    if (nbLeaves == nbLeavesMax) {
                        return 0;
                    }
                    nbLeaves++;
                    ascending = 0;
                }
            }
        }
        if (ascending == 0) {
            ascending = 1;
			befUpdErr--;
			if (befUpdErr == 0) {
                errorCopy = min(*error, errorCopy);
				befUpdErr = nbItBefUpdErr;
			}
            if (depth == 0) {
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
                result[rank][0] = restsSizes[rank][0];
            } 
            else {
				if (reservationListsResult[rank][depth - 1] == -1) {
                    nodId = rests[rank][depth - 1][result[rank][depth - 1]];
				}
				else {
                    nodId = ((int*)reservationLists[rank][reservationListsResult[rank][depth - 1]].data.data)[reservationListsElemResult[rank][depth - 1]];
				}
                nodes[nodId].nbPost--;
                notChosenAnymore(nodes, n, nodId, reservationRules[rank], reservationLists[rank]);
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




        if (reservationRules[rank][depth].resList == -1) {
            do {
                if (result[rank][depth] == 0) {
                    ascending = 0;
                    break;
                }
                result[rank][depth]--;
                currentElement = rests[rank][depth][result[rank][depth]];
                if (check_conditions(nodes, result[rank], n, currentElement) == 1) {
                    break;
                }
            } while (1);
        }
        else {
            found = 0;
			if (reservationListsResult[rank][depth] == -1) {
				placeInd = reservationRules[rank][depth].resList;
                choiceInd = 0;
			}
			else {
				placeInd = reservationListsResult[rank][depth];
				choiceInd = reservationListsElemResult[rank][depth] + 1;
			}
            reservationListsResult[rank][depth] = -1;
            do {
                for (int i = choiceInd; i < reservationLists[rank][placeInd].data.size; i++) {
                    currentElement = ((int*)reservationLists[rank][placeInd].data.data)[i];
                    if (nodes[currentElement].nbPost == 0 && check_conditions(nodes, result[rank], n, currentElement) == 1) {
                        reservationListsResult[rank][depth] = placeInd;
                        reservationListsElemResult[rank][depth] = i;
                        for (int j = 0; j < restsSizes[rank][depth]; j++) {
                            if (rests[rank][depth][j] == currentElement) {
                                result[rank][depth] = j;
                                break;
                            }
                        }
                        found = 1;
                        break;
                    }
                }
                if (found) {
                    break;
                }
                if (reservationRules[rank][depth].all != 0) {
                    break;
                }
                choiceInd = 0;
                placeInd = reservationLists[rank][placeInd].nextListInd;
            } while (placeInd != -1);
            if (!found) {
                ascending = 0;
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

void add_to_python_path(const char* path) {
    // Import the sys module
    PyObject* pSys = PyImport_ImportModule("sys");
    if (!pSys) {
        PyErr_Print();
        fprintf(stderr, "Failed to import sys\n");
        return;
    }

    // Get the path list from sys.path
    PyObject* pPath = PyObject_GetAttrString(pSys, "path");
    if (!pPath) {
        PyErr_Print();
        fprintf(stderr, "Failed to get sys.path\n");
        Py_DECREF(pSys);
        return;
    }

    // Create a new Python string for the path to add
    PyObject* pPathStr = PyUnicode_FromString(path);
    if (!pPathStr) {
        PyErr_Print();
        fprintf(stderr, "Failed to create Python string for path\n");
        Py_DECREF(pPath);
        Py_DECREF(pSys);
        return;
    }

    // Append the path to sys.path
    PyList_Append(pPath, pPathStr);

    // Clean up references
    Py_DECREF(pPathStr);
    Py_DECREF(pPath);
    Py_DECREF(pSys);
}

int main(int argc, char* argv[]) {
	// TODO : Check if sortings of similar systems already exist

    // Define the parameters
    int nbItBefUpdErr = 10000; // Define the number of iterations before updating the error
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
	if (nb_process < 1 || nb_process > 127) {
		printf("The number of processes must be between 1 and 127\n");
		return 1;
	}
    int** result = malloc(nb_process * sizeof(int*)); // Store the current result
    int*** rests = malloc(nb_process * sizeof(int**));
	int** restsSizes = malloc(nb_process * sizeof(int*));
    ReservationRule** reservationRules = malloc(nb_process * sizeof(ReservationRule*));
    ReservationList** reservationLists = malloc(nb_process * sizeof(ReservationList*));
    int** reservationListsResult = malloc(nb_process * sizeof(int*));
	int** reservationListsElemResult = malloc(nb_process * sizeof(int*));
	int** errors = malloc(nb_process * sizeof(int*)); // Store the current error
    
	int depth = 0; // Current depth of the recursion
    for (int p = 0; p < nb_process; p++) {
        result[p] = malloc((n + 1) * sizeof(int));
		rests[p] = malloc((n + 1) * sizeof(int*));
		restsSizes[p] = malloc((n + 1) * sizeof(int));
		reservationRules[p] = malloc((n + 1) * sizeof(ReservationRule));
		reservationLists[p] = malloc((n - 1) * sizeof(ReservationList));
		reservationListsResult[p] = malloc((n + 1) * sizeof(int));
		reservationListsElemResult[p] = malloc(n * sizeof(int));
		errors[p] = malloc(n * sizeof(int));
        for (int i = 0; i < n + 1; i++) {
            rests[p][i] = malloc((n - i) * sizeof(int));
            restsSizes[p][i] = 0;
            reservationRules[p][i].resList = -1;
            reservationListsResult[p][i] = -1;
        }
        for (int i = 0; i < n; i++) {
            if (nodes[i].nbPost == 0) {
                rests[p][0][restsSizes[p][0]] = i;
                restsSizes[p][0]++;
            }
        }
        for (int i = 0; i < n - 1; i++) {
            if (reservationLists[p] != NULL) {
                initList(&(reservationLists[p][i].data), sizeof(int), 1);
            }
            else {
                printf("Error: reservationLists[%d] is NULL\n", p);
                exit(1);
            }
            notChosenAnymore(nodes, n, i, reservationRules[p], reservationLists[p]);
        }
    }
	int error = 0;
    int starts[] = { 0 }; // Define the starts array
	int end = 1;
	int startsSize = -1;
    try_sort(argv[1], nb_process, nbItBefUpdErr, nbLeavesMin, nbLeavesMax, repartTol, nodes, startExcl, endExcl, result, errors, reservationListsResult, reservationListsElemResult, rests, restsSizes, reservationRules, reservationLists, n, 0, &error, 1, &end, 0, &startsSize);
    if (startsSize == -1) {
		printf("Error: startsSize is -1\n");
		return 1;
    }
    int reste = startsSize % nb_process;
	for (int rank = 0; rank < nb_process; rank++) {
        try_sort(argv[1], nb_process, nbItBefUpdErr, nbLeavesMin, nbLeavesMax, repartTol, nodes, startExcl, endExcl, result, errors, reservationListsResult, reservationListsElemResult, rests, restsSizes, reservationRules, reservationLists, n, reste, &error, 0, &end, rank, &startsSize);
	}
    free(result);
    freeNodes(nodes, n);

    call_python_function("switchSort", argv[1], NULL, 0);

    return 0;
}
