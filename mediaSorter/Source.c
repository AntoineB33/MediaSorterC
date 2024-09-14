#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"  // Include the cJSON header

// Define a structure to represent the graph node
typedef struct {
    int* ulteriors;     // List of integers that must come after
    int ulteriors_size; // Size of the 'ulteriors' list
    int nbPost;         // Count of integers that must be placed before this
    int highest;        // Highest placement allowed for this integer
	int lowest;		 // Lowest placement allowed for this integer
} Node;

typedef struct {
    int resListInd;
    int startInd;
} ReservationRule;

typedef struct {
    void* data;       // Pointer to the array (generic list)
    size_t size;      // Number of elements in the list
    size_t capacity;  // Capacity of the list
    size_t elemSize;  // Size of each element
	int lastElment;   // Index of the last element
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
	list->lastElment = 0;
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

// Recursive function to try all possible ways to sort the integers
void try_sort(Node* nodes, int n, int* sorted, int depth, int* result, Rule** rules, int* ruleSizes, int* ruleCapacities, Reservation** reservations, int* reservationSize, int* reservationsCapacity, int* reservationSizes, int* reservationsCapacities) {
    // If all integers are sorted, print the current result
    if (depth == n) {
        for (int i = 0; i < n; i++) {
            printf("%d ", result[i]);
        }
        printf("\n");
        return;
    }

    for (int j = 0; j < ruleSizes[depth]; j++) {
        if (rules[depth][j].type == 0) {
			int pt = rules[depth][j].pt;
            for (int m = 0; m < reservationSizes[pt]; m++) {
				if (!reservations[pt][m].placed) {
					int i = reservations[pt][m].integer;
					if (!sorted[i]) {
						// Place this integer
						sorted[i] = 1;
						result[depth] = i;
						reservations[pt][m].placed = 1;

						// Update the nbPost of all integers that come after the current one
						for (int k = 0; k < nodes[i].ulteriors_size; k++) {
							nodes[nodes[i].ulteriors[k]].nbPost--;
						}

						// Recur to place the next integer
						try_sort(nodes, n, sorted, depth + 1, result, rules, ruleSizes, ruleCapacities, reservations, reservationSize, reservationsCapacity, reservationSizes, reservationsCapacities);

						// Backtrack: unplace this integer
						for (int k = 0; k < nodes[i].ulteriors_size; k++) {
							nodes[nodes[i].ulteriors[k]].nbPost++;
						}
						sorted[i] = 0;
						reservations[pt][m].placed = 0;
					}
				}
            }
        }
    }
    if (!highests[depth].placed) {
        int i = highests[depth].integer;
        if (!sorted[i] && nodes[i].nbPost == 0) {
            // Place this integer
            sorted[i] = 1;
            result[depth] = i;

            // Update the nbPost of all integers that come after the current one
            for (int j = 0; j < nodes[i].ulteriors_size; j++) {
                nodes[nodes[i].ulteriors[j]].nbPost--;
            }

            // Recur to place the next integer
            try_sort(nodes, n, sorted, depth + 1, result, highests);

            // Backtrack: unplace this integer
            for (int j = 0; j < nodes[i].ulteriors_size; j++) {
                nodes[nodes[i].ulteriors[j]].nbPost++;
            }
            sorted[i] = 0;
        }
    }
    else {
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
}

void update(Node* nodes, int n, int i, ReservationRule* reservationRules, GenericList* reservations) {
    if (nodes[i].highest < n - 1) {
        int highest = nodes[i].highest;
        int ind = 0;
        int resGroupInd = reservationRules[highest].resGroupInd;
        if (resGroupInd == -1) {
            reservationRules[highest].resGroupInd = reservations->size;
			reservationRules[highest].resListInd = 0;
            reservationRules[highest].startInd = 0;
            GenericList reservationList;
			initList(&reservationList, sizeof(int), 1);
			addElement(&reservationList, &i);
            GenericList reservationGroup;
			initList(&reservationGroup, sizeof(GenericList), 1);
			addElement(&reservationGroup, &reservationList);
			addElement(reservations, &reservationGroup);
        }
        else {
            int newresGroupInd = resGroupInd;
            GenericList* reservationGroup = (GenericList*)reservations->data;
            GenericList* reservationList = &reservationGroup[newresGroupInd];
            int* integers = (int*)reservationList->data;
			int resListIndToSet = -1;
            if (highest != nodes[integers[0]].highest) {
                GenericList reservationList;
                initList(&reservationList, sizeof(int), 1);
                addElement(&reservationList, &i);
                GenericList* reservationGroup = (GenericList*)reservations->data;
                if (reservationRules[highest].resListInd == -1) {
                    reservationGroup->lastElment = reservationGroup->size;
                    resListIndToSet = -1;
                    newresGroupInd = reservationRules[highest - 1].resGroupInd;
                }
                addElement(&reservationGroup, &reservationList);
            }
            while (newresGroupInd != -1) {
                reservationGroup = (GenericList*)reservations->data;
                reservationList = &reservationGroup[reservationGroup->lastElment];
                integers = (int*)reservationList->data;
                highest = nodes[integers[0]].highest - reservationList->size + 1;
				resGroupInd = newresGroupInd;
                while ((newresGroupInd = reservationRules[highest - 1].resGroupInd) == resGroupInd) {
                    highest--;
                }
            }





            reservationSizes[pt]++;
            increaseCapacity(reservations[pt], reservationSizes[pt], reservationsCapacities[pt], sizeof(Reservation*));
            int m = highest - 1;
            while (1) {
                found = 0;
                for (int n = 0; n < ruleSizes[m]; n++) {
                    if (rules[m][n].type == 0) {
                        int pt2 = rules[m][n].pt;
                        if (pt2 != pt) {
                            for (int q = highest; q >= m; q--) {
                                for (int r = 0; r < ruleSizes[q]; r++) {
                                    if (rules[q][r].type == 0) {
                                        rules[q][r].pt = pt2;
                                    }
                                }
                            }
                            increaseCapacity(reservations[pt2], reservationSizes[pt] + 1, reservationsCapacities[pt2], sizeof(Reservation));
                            reservationInd[i] = reservationSizes[pt2] - 1;
                            reservationSizes[pt2] += reservationSizes[pt] + 1;
                            // Copying elements of list2 into list1
                            memcpy(reservations[pt2] + reservationSizes[pt2], reservations[pt], reservationSizes[pt] * sizeof(int));
                            newPt = pt2;
                            m -= reservationSizes[pt];
                            reservationSizes[pt] = 0;
                        }
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    highest = m;
                    break;
                }
                m--;
            }
            found = 1;
            break;
        }
        ruleSizes[i]++;
        increaseCapacity(rules[highest], ruleSizes[highest], ruleCapacities[highest], sizeof(Rule));
        rules[highest][ruleSizes[highest] - 1].type = 0;
        rules[highest][ruleSizes[highest] - 1].pt = reservationSize;
        if (!found) {
            reservationSize++;
            if (reservationSize == reservationsCapacity) {
                reservationSizes = realloc(reservationSizes, reservationSize * sizeof(Reservation*));
            }
            increaseCapacity(reservations, reservationSize, reservationsCapacity, sizeof(Reservation*));
        }
        reservations[newPt][reservationSizes[newPt] - 1].integer = i;
        reservations[newPt][reservationSizes[newPt] - 1].placed = 0;
        reservations[newPt][reservationSizes[newPt] - 1].lowest = nodes[i].lowest;
        reservations[newPt][reservationSizes[newPt] - 1].highest = nodes[i].highest;
    }
}

int main() {
    int n = 4; // Define the number of integers to sort

    // Dynamically allocate memory for the array of nodes
    Node* nodes = (Node*)malloc(n * sizeof(Node));

    // Setup the constraints for each integer
    // Integer 0
    nodes[0].ulteriors_size = 2;
    nodes[0].ulteriors = (int*)malloc(nodes[0].ulteriors_size * sizeof(int));
    nodes[0].ulteriors[0] = 1;  // 0 must come before 1
    nodes[0].ulteriors[1] = 2;  // 0 must come before 2
    nodes[0].nbPost = 0;
    nodes[0].highest = 1; // 0 must be placed at index 1 or earlier

    // Integer 1
    nodes[1].ulteriors_size = 1;
    nodes[1].ulteriors = (int*)malloc(nodes[1].ulteriors_size * sizeof(int));
    nodes[1].ulteriors[0] = 3;  // 1 must come before 3
    nodes[1].nbPost = 1; // 1 must come after 0
    nodes[1].highest = 3; // 1 must be placed at index 2 or earlier

    // Integer 2
    nodes[2].ulteriors_size = 0;
    nodes[2].ulteriors = NULL; // 2 has no restrictions
    nodes[2].nbPost = 1; // 2 must come after 0
    nodes[2].highest = 3; // 2 must be placed at index 3 or earlier

    // Integer 3
    nodes[3].ulteriors_size = 0;
    nodes[3].ulteriors = NULL; // 3 has no restrictions
    nodes[3].nbPost = 1; // 3 must come after 1
    nodes[3].highest = 2; // 3 must be placed at index 3 or earlier

    int* sorted = calloc(n, sizeof(int));  // Keep track of sorted integers
    int* result = malloc(n * sizeof(int)); // Store the current result

    GenericList reservations;
    reservations.size = 0;
    reservations.capacity = 0;
	reservations.elemSize = sizeof(GenericList);
	ReservationRule* reservationRules = malloc((n - 1) * sizeof(ReservationRule));



    // Populate the highests array based on the `highest` attribute of each node
    for (int i = 0; i < n; i++) {
        update
    }

    // Try all possible ways to sort the integers
    printf("All possible valid orderings:\n");
    try_sort(nodes, n, sorted, 0, result, rules, ruleSizes, ruleCapacities, reservations, reservationSize, reservationsCapacity, reservationSizes, reservationsCapacities);

    // Free allocated memory
    for (int i = 0; i < n; i++) {
        free(nodes[i].ulteriors);
    }
    free(nodes);
    free(sorted);
    free(result);

    return 0;
}
