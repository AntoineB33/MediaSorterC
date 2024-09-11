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
    int type;
    int pt;
} Rule;

typedef struct {
    int integer;
    int placed;
    int lowest;
    int highest;
} Reservation;

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

void increaseCapacity(void* list, int listSize, int* listCapacity, size_t elementSize) {
    if (listSize == *listCapacity) {
        *listCapacity = listSize;
        list = realloc(list, listSize * elementSize);
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

	Rule** rules = malloc(sizeof(Rule*));
	int* ruleSizes = malloc(sizeof(int));
	int* ruleCapacities = malloc(sizeof(int));
    Reservation** reservations = NULL;
    int reservationSize = 0;
    int reservationsCapacity = 0;
    int* reservationSizes = NULL;
    int* reservationsCapacities = NULL;


    // Populate the highests array based on the `highest` attribute of each node
    for (int i = 0; i < n; i++) {
        if (nodes[i].highest < n - 1) {
            int highest = nodes[i].highest;
            int newPt = reservationSize - 1;
            int found = 0;
            for (int j = 0; j < ruleSizes[highest]; j++) {
                if (rules[highest][j].type == 0) {
					int pt = rules[highest][j].pt;
                    reservationSizes[pt]++;
					increaseCapacity(reservations[pt], reservationSizes[pt], reservationsCapacities[pt], sizeof(Reservation));
                    int m = highest - 1;
                    while(1) {
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
                                    reservationSizes[pt2] += reservationSizes[pt] + 1;
                                    increaseCapacity(reservations[pt2], reservationSizes[pt2], reservationsCapacities[pt2], sizeof(Reservation));

                                    // Allocate memory for the new array
                                    int* newList = malloc((reservationSizes[pt2]) * sizeof(int));

                                    // Copy the first list to the new list
                                    memcpy(newList, reservations[pt2], reservationSizes[pt] * sizeof(int));

                                    // Copy the second list to the new list (after the first list)
                                    memcpy(newList + reservationSizes[pt2], reservations[pt], reservationSizes[pt] * sizeof(int));

									reservations[pt2] = newList;
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
    free(highests);

    return 0;
}
