#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"  // Include the cJSON header

// Define a structure to represent the graph node
typedef struct {
    int* ulteriors;     // List of integers that must come ulteriors
    int ulteriors_size; // Size of the 'ulteriors' list
    int nbPost; // Count of integers that must be placed before this
	int highest; // Highest placement
} Node;

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
void try_sort(Node* nodes, int n, int* sorted, int depth, int* result) {
    // If all integers are sorted, print the current result
    if (depth == n) {
        for (int i = 0; i < n; i++) {
            printf("%d ", result[i]);
        }
        printf("\n");
        return;
    }

    // List the integers that can be placed (those with nbPost == 0)
    for (int i = 0; i < n; i++) {
        if (!sorted[i] && nodes[i].nbPost == 0) {
            // Place this integer
            sorted[i] = 1;
            result[depth] = i;

            // Update the nbPost of all integers that come ulteriors the current one
            for (int j = 0; j < nodes[i].ulteriors_size; j++) {
                nodes[nodes[i].ulteriors[j]].nbPost--;
            }

            // Recur to place the next integer
            try_sort(nodes, n, sorted, depth + 1, result);

            // Backtrack: unplace this integer
            for (int j = 0; j < nodes[i].ulteriors_size; j++) {
                nodes[nodes[i].ulteriors[j]].nbPost++;
            }
            sorted[i] = 0;
        }
    }
}

// Function to parse the JSON input and fill the Node structure
Node* parse_json(const char* json_string, int* n) {
    cJSON* json = cJSON_Parse(json_string);
    if (json == NULL) {
        printf("Error parsing JSON!\n");
        return NULL;
    }

    cJSON* nodes_json = cJSON_GetObjectItem(json, "nodes");
    *n = cJSON_GetArraySize(nodes_json);

    Node* nodes = (Node*)malloc((*n) * sizeof(Node));

    for (int i = 0; i < *n; i++) {
        cJSON* node_json = cJSON_GetArrayItem(nodes_json, i);
        cJSON* ulteriors_json = cJSON_GetObjectItem(node_json, "ulteriors");
        cJSON* nbPost_json = cJSON_GetObjectItem(node_json, "nbPost");

        nodes[i].ulteriors_size = cJSON_GetArraySize(ulteriors_json);
        nodes[i].ulteriors = (int*)malloc(nodes[i].ulteriors_size * sizeof(int));
        for (int j = 0; j < nodes[i].ulteriors_size; j++) {
            nodes[i].ulteriors[j] = cJSON_GetArrayItem(ulteriors_json, j)->valueint;
        }
        nodes[i].nbPost = nbPost_json->valueint;
    }

    cJSON_Delete(json);
    return nodes;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <json_data>\n", argv[0]);
        return 1;
    }

    const char* json_data = argv[1];
    int n;

    // Parse the JSON input to create the graph nodes
    Node* nodes = parse_json(json_data, &n);
    if (nodes == NULL) {
        return 1;
    }

    int* sorted = (int*)calloc(n, sizeof(int));  // Keep track of sorted integers
    int* result = (int*)malloc(n * sizeof(int)); // Store the current result

    // Try all possible ways to sort the integers
    printf("All possible valid orderings:\n");
    try_sort(nodes, n, sorted, 0, result);

    // Free allocated memory
    for (int i = 0; i < n; i++) {
        free(nodes[i].ulteriors);
    }
    free(nodes);
    free(sorted);
    free(result);

    return 0;
}
