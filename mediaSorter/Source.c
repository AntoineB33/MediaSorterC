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
} Node;

// Define a structure for the highests array
typedef struct {
    int integer; // The integer that must be placed at this index
    int placed;  // Flag indicating if the integer has been placed
} Highest;

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
void try_sort(Node* nodes, int n, int* sorted, int depth, int* result, Highest* highests) {
    // If all integers are sorted, print the current result
    if (depth == n) {
        for (int i = 0; i < n; i++) {
            printf("%d ", result[i]);
        }
        printf("\n");
        return;
    }

    // Check if there is an integer that must be placed at the current index according to `highests`
    if (highests[depth].integer != -1 && !highests[depth].placed) {
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

    int* sorted = (int*)calloc(n, sizeof(int));  // Keep track of sorted integers
    int* result = (int*)malloc(n * sizeof(int)); // Store the current result

    // Initialize the highests array
    Highest* highests = (Highest*)malloc(n * sizeof(Highest));
    for (int i = 0; i < n; i++) {
        highests[i].integer = -1;  // Default to no specific integer required
        highests[i].placed = 0;
    }

    // Populate the highests array based on the `highest` attribute of each node
    for (int i = 0; i < n; i++) {
        if (nodes[i].highest < n - 1) {
            highests[nodes[i].highest].integer = i;
        }
    }

    // Try all possible ways to sort the integers
    printf("All possible valid orderings:\n");
    try_sort(nodes, n, sorted, 0, result, highests);

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
