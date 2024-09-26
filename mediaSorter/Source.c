#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"  // Include the cJSON header


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

void notChosenUlt(Node* nodes, int i) {
    for (int j = 0; j < nodes[i].ulteriors.size; j++) {
        int ulterior = ((int*)nodes[i].ulteriors.data)[j];
        nodes[ulterior].nbPost++;
    }
}

void notChosenAnymore(Node* nodes, int n, int i, ReservationRule* reservationRules, ReservationList* reservationLists) {
    if (nodes[i].highest < n - 1) {
        int highest = nodes[i].highest;
        int resListInd = reservationRules[highest].resList;
        if (resListInd == -1) {
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
        rests[rank][depth + 1][restsSizes[rank][depth + 1]] = i;
        restsSizes[rank][depth + 1]++;
    }
}

void chosen(Node* nodes, int n, int rank, int depth, int** result, int*** rests, int** restsSizes, int i, ReservationRule* reservationRules, ReservationList* reservationLists) {
	if (nodes[i].highest < n - 1) {
        int highest = nodes[i].highest;
        int placeInd = highest;
        int placeIndPrev;
        do {
            placeIndPrev = placeInd;
            placeInd = reservationLists[placeIndPrev].prevListInd;
        } while (reservationLists[placeIndPrev].out == 0);
        placeInd++;
        reservationRules[placeInd].resList = -1;
        reservationLists[placeIndPrev].prevListInd++;
        if (reservationRules[placeInd].all == 0) {
            do {
                placeIndPrev = placeInd;
                placeInd = reservationLists[placeInd].nextListInd;
            } while (placeInd != highest && reservationLists[placeInd].data.size >= placeInd - reservationLists[placeInd].prevListInd);
            reservationLists[placeIndPrev].nextListInd = -1;
            reservationLists[placeInd].out = 1;
        }
        removeElement(&(reservationLists[highest].data), i);
    }
    for (int j = 0; j < nodes[i].ulteriors.size; j++) {
		chosenUlt(nodes, rank, depth, rests, restsSizes, ((int*)nodes[i].ulteriors.data)[j]);
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

// Recursive function to try all possible ways to sort the integer
int try_sort(int nb_process, int nbItBefUpdErr, int nbLeavesMin, int nbLeavesMax, float repartTol, Node* nodes, GenericList* lowests, int** result, int* errors, int** reservationListsResult, int** reservationListsElemResult, int*** rests, int** restsSizes, ReservationRule** reservationRules, ReservationList** reservationLists, int n, int reste, int* error, int mpiManagement, int* end, int rank, int* startsSize) {
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
    while (1) {
        if (ascending == 1) {
            depth++;
			if(depth != 0) {
                if (depth == endI) {
                    if (mpiManagement == 0) {
                        if (startInd == endStarts) {
                            return 0;
                        }
                        if (startInd == startStarts) {
                            endI = n;
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
                if (ascending == 1) {
                    for (int i = 0; i < result[rank][depth - 1]; i++) {
                        rests[rank][depth][i] = rests[rank][depth - 1][i];
                    }
                    for (int i = result[rank][depth - 1] + 1; i < restsSizes[rank][depth - 1]; i++) {
                        rests[rank][depth][i - 1] = rests[rank][depth - 1][i];
                    }
                    result[rank][depth] = restsSizes[rank][depth];
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
                if (nbLeaves != 0) {
                    if (mpiManagement == 0) {
                        return 0;
                    }
                    else {
                        int restLeaves2 = nbLeaves % nb_process;
                        if (restLeaves2 <= restLeaves) {
                            if (restLeaves2 == 0 || nbLeaves > nbLeavesMin && restLeaves2 <= repartTol * nb_process) {
                                return 0;
                            }
                            *startsSize = nbLeaves;
                            *end = endI;
                            restLeaves = restLeaves2;
                            nbLeaves = 0;
                        }
                        endI++;
                    }
                }
                result[rank][0] = restsSizes[rank][0];
            } 
            else {
                depth--;
                int nodId = rests[rank][depth][result[rank][depth]];
                notChosenAnymore(nodes, n, nodId, reservationRules[rank], reservationLists[rank]);
                notChosenUlt(nodes, nodId);
            }
        }
        for (int i = 0; i < lowests[depth].size; i++) {
			chosenUlt(nodes, rank, depth, rests, restsSizes, ((int*)lowests[depth].data)[i]);
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
                    chosen(nodes, n, rank, depth, result, rests, restsSizes, currentElement, reservationRules[rank], reservationLists[rank]);
                    break;
                }
            } while (1);
        }
        else {
            found = 0;
            choiceInd = 0;
            placeInd = reservationRules[rank][depth].resList;
            do {
                for (int i = 0; i < reservationLists[rank][placeInd].data.size; i++) {
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
                        chosen(nodes, n, rank, depth, result, rests, restsSizes, currentElement, reservationRules[rank], reservationLists[rank]);
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
                placeInd = reservationLists[rank][placeInd].nextListInd;
            } while (placeInd != -1);
            if (!found) {
                ascending = 0;
            }
        }
    }
}

int main() {

	// Define the parameters
	int nbItBefUpdErr = 10000; // Define the number of iterations before updating the error
	int nbLeavesMin = 100; // Define the minimum time to sort
	int nbLeavesMax = 1000; // Define the maximum time to sort
	float repartTol = 0.1; // Define the repartition tolerance


    // example of nodes, lowests and attributeMngs

    int n = 4; // Define the number of integers to sort
    int nb_att = 0; // Define the number of attributes

    // Dynamically allocate memory for the array of nodes
    Node* nodes = malloc(n * sizeof(Node));
	GenericList* lowests = malloc(n * sizeof(GenericList));
	AttributeMng* attributeMngs = malloc(nb_att * sizeof(AttributeMng));

    nodes[0].highest = 0;
    initList(&(nodes[0].ulteriors), sizeof(int), 1);
    nodes[0].conditions_size = 0;
    nodes[0].conditions = malloc(0 * sizeof(char*));

    nodes[1].highest = 1;
    initList(&(nodes[1].ulteriors), sizeof(int), 1);
    int value = 0;
	addElement(&(nodes[1].ulteriors), &value);
    nodes[1].conditions_size = 0;
    nodes[1].conditions = malloc(0 * sizeof(char*));

    nodes[2].highest = 2;
    initList(&(nodes[2].ulteriors), sizeof(int), 1);
    value = 0;
    addElement(&(nodes[2].ulteriors), &value);
    nodes[2].conditions_size = 0;
    nodes[2].conditions = malloc(0 * sizeof(char*));

    nodes[3].highest = 3;
    initList(&(nodes[3].ulteriors), sizeof(int), 1);
	value = 1;
	addElement(&(nodes[3].ulteriors), &value);
	value = 2;
	addElement(&(nodes[3].ulteriors), &value);
    nodes[3].conditions_size = 0;
    nodes[3].conditions = malloc(0 * sizeof(char*));

    for (int i = 0; i < n; i++) {
        nodes[i].nbPost = 0;
		initList(&(lowests[i]), sizeof(int), 1);
        notChosenUlt(nodes, i);
    }




    int nb_process = 10;
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
		result[p] = malloc(n * sizeof(int));
		rests[p] = malloc(n * sizeof(int*));
		restsSizes[p] = malloc(n * sizeof(int));
		reservationRules[p] = malloc((n - 1) * sizeof(ReservationRule));
		reservationLists[p] = malloc((n - 1) * sizeof(ReservationList));
		reservationListsResult[p] = malloc(n * sizeof(int));
		reservationListsElemResult[p] = malloc(n * sizeof(int));
		errors[p] = malloc(n * sizeof(int));
        for (int i = 0; i < n - 1; i++) {
            reservationRules[p][i].resList = -1;
            if (reservationLists[p] != NULL) {
                initList(&(reservationLists[p][i].data), sizeof(int), 1);
            }
            else {
                printf("Error: reservationLists[%d] is NULL\n", p);
                exit(1);
            }
            notChosenAnymore(nodes, n, i, reservationRules[p], reservationLists[p]);
        }
        for (int i = 0; i < n; i++) {
            rests[p][i] = malloc((n - i) * sizeof(int));
            restsSizes[p][i] = 0;
            if (nodes[i].nbPost == 0) {
                rests[p][0][restsSizes[p][0]] = i;
                restsSizes[p][0]++;
            }
        }
    }
	int error = 0;
    int starts[] = { 0 }; // Define the starts array
	int end = 1;
	int startsSize = -1;
    try_sort(nb_process, nbItBefUpdErr, nbLeavesMin, nbLeavesMax, repartTol, nodes, lowests, result, errors, reservationListsResult, reservationListsElemResult, rests, restsSizes, reservationRules, reservationLists, n, 0, &error, 1, &end, 0, &startsSize);
    if (startsSize == -1) {
		printf("Error: startsSize is -1\n");
		return 1;
    }
    int reste = startsSize % nb_process;
	for (int rank = 0; rank < nb_process; rank++) {
        try_sort(nb_process, nbItBefUpdErr, nbLeavesMin, nbLeavesMax, repartTol, nodes, lowests, result, errors, reservationListsResult, reservationListsElemResult, rests, restsSizes, reservationRules, reservationLists, n, reste, &error, 0, &end, rank, &startsSize);
	}
    // Free allocated memory
    for (int i = 0; i < n; i++) {
		freeList(&(nodes[i].ulteriors));
    }
    free(nodes);
    free(result);

    return 0;
}
