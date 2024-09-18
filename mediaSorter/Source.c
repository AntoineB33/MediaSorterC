#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"  // Include the cJSON header


// Define a structure to represent the graph node
typedef struct {
	GenericList ulteriors;	 // List of integers that must come after
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
    void* data;       // Pointer to the array (generic list)
    size_t size;      // Number of elements in the list
    size_t capacity;  // Capacity of the list
    size_t elemSize;  // Size of each element
} GenericList;


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
        list->data = realloc(list->data, list->capacity * list->elemSize);
        if (list->data == NULL) {
            printf("Memory reallocation failed\n");
            exit(1);
        }
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

// Utility function to check if all integers have been sorted
int all_sorted(int* sorted, int n) {
    for (int i = 0; i < n; i++) {
        if (!sorted[i]) {
            return 0;
        }
    }
    return 1;
}

// Recursive function to try all possible ways to sort the integer
void try_sort(Node* nodes, int n, int depth, int* result, GenericList* lowests, ReservationRule* reservationRules, ReservationList* reservationLists) {
    // If all integers are sorted, print the current result
    if (depth == n) {
        for (int i = 0; i < n; i++) {
            printf("%d ", result[i]);
        }
        printf("\n");
        return;
    }

    int placeInd;
	if (reservationRules[depth].resList != -1) {
        if (reservationRules[depth].all == 0) {
            do {
				for (int i = 0; i < reservationLists[placeInd].data.size; i++) {
					int* currentElement = reservationLists[placeInd].data.data + i * reservationLists[placeInd].data.elemSize;
					if (nodes[*currentElement].nbPost == 0) {
						// Place this integer
						result[depth] = *currentElement;
						chosen(nodes, n, *currentElement, reservationRules, reservationLists);
						// Recur to place the next integer
						try_sort(nodes, n, depth + 1, result, lowests, reservationRules, reservationLists);
						// Backtrack: unplace this integer
						notChosenAnymore(nodes, n, *currentElement, reservationRules, reservationLists);
					}
				}
				placeInd = reservationLists[placeInd].nextListInd;
			} while (1);
        }
    }



    // List the integers that can be placed (those with nbPost == 0)
    for (int i = 0; i < n; i++) {
        if (!sorted[i] && nodes[i].nbPost == 0) {
            // Place this integer
            sorted[i] = 1;
            result[depth] = i;

            if (nodes[i].highest < n - 1) {
                highests[nodes[i].highest].placed = 1;
            }

            // Update the nbPost of all integers that come after the current one
            for (int j = 0; j < nodes[i].ulteriors_size; j++) {
                nodes[nodes[i].ulteriors[j]].nbPost--;
            }

            // Recur to place the next integer
            try_sort(nodes, n, sorted, depth + 1, result, highests);

            if (nodes[i].highest < n - 1) {
                highests[nodes[i].highest].placed = 0;
            }

            // Backtrack: unplace this integer
            for (int j = 0; j < nodes[i].ulteriors_size; j++) {
                nodes[nodes[i].ulteriors[j]].nbPost++;
            }
            sorted[i] = 0;
        }
    }
}

void notChosenAnymore(Node* nodes, int n, int i, ReservationRule* reservationRules, ReservationList* reservationLists) {
    if (nodes[i].highest < n - 1) {
        int highest = nodes[i].highest;
        int resListInd = reservationRules[highest].resList;
        if (resListInd == -1) {
            reservationLists[highest].out = 1;
            reservationLists[highest].prevListInd = highest - 1;
			reservationLists[highest].nextListInd = -1;
            reservationLists[highest].all = 0;
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
            do{
                do{
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
	for (int j = 0; j < nodes[i].ulteriors.size; j++) {
		int ulterior = nodes[i].ulteriors[j];
		nodes[ulterior].nbPost--;
	}
}

void chosen(Node* nodes, int n, int i, ReservationRule* reservationRules, ReservationList* reservationLists) {
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
		int ulterior = nodes[i].ulteriors[j];
		nodes[ulterior].nbPost++;
	}
}

int main() {
    int n = 4; // Define the number of integers to sort

    // Dynamically allocate memory for the array of nodes
    Node* nodes = (Node*)malloc(n * sizeof(Node));
	GenericList* lowests = malloc((n - 1) * sizeof(GenericList));

    int* result = malloc(n * sizeof(int)); // Store the current result

    ReservationRule* reservationRules = malloc((n - 1) * sizeof(ReservationRule));
    ReservationList* reservationLists = malloc((n - 1) * sizeof(ReservationList));
	for (int i = 0; i < n - 1; i++) {
		reservationRules[i].resList = -1;
		initList(&(reservationLists[i].data), sizeof(int), 1);
        reservationLists[i].nextListInd = -1;
        reservationLists[i].out = 0;
        notChosenAnymore(nodes, n, i, reservationRules, reservationLists);
	}

    // Try all possible ways to sort the integers
    printf("All possible valid orderings:\n");
    try_sort(nodes, n, 0, result, lowests, reservationRules, reservationLists);

    // Free allocated memory
    for (int i = 0; i < n; i++) {
		freeList(&(nodes[i].ulteriors));
    }
    free(nodes);
    free(result);

    return 0;
}
