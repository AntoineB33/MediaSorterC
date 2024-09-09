#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"

void process_json(cJSON* json) {
    cJSON* p = cJSON_GetObjectItemCaseSensitive(json, "p");
    cJSON* v = cJSON_GetObjectItemCaseSensitive(json, "v");

    // Assuming `p` is an array and `v` is an integer
    if (cJSON_IsArray(p) && cJSON_IsNumber(v)) {
        printf("v: %d\n", v->valueint);

        // Print the contents of the array `p`
        printf("p array contents:\n");
        for (int i = 0; i < cJSON_GetArraySize(p); i++) {
            cJSON* element = cJSON_GetArrayItem(p, i);
            if (cJSON_IsNumber(element)) {
                printf("%d ", element->valueint);
            }
            else if (cJSON_IsArray(element)) {
                printf("[ ");
                for (int j = 0; j < cJSON_GetArraySize(element); j++) {
                    cJSON* sub_element = cJSON_GetArrayItem(element, j);
                    if (cJSON_IsNumber(sub_element)) {
                        printf("%d ", sub_element->valueint);
                    }
                }
                printf("] ");
            }
        }
        printf("\n");
    }
    else {
        printf("Invalid JSON format.\n");
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <json_string>\n", argv[0]);
        return 1;
    }

    // Parse the JSON string passed as an argument
    char* json_string = argv[1];
    cJSON* json = cJSON_Parse(json_string);

    if (json == NULL) {
        fprintf(stderr, "Error parsing JSON.\n");
        return 1;
    }

    // Process the parsed JSON
    process_json(json);

    // Free the cJSON object
    cJSON_Delete(json);

    return 0;
}
