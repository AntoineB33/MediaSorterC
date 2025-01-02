#include "all_func.h"

void getPrevSort(char* sort_info_path, cJSON* bufferJS, int* nb_params, ThreadParams* allParams, int n, problemSt* problem) {
    HANDLE hFile;
    OVERLAPPED ov;
    FILE* file;
    char* fileContent;
    cJSON* json;
    if (openTxtFile(&hFile, &ov, file, fileContent, json, sort_info_path)) {
        return -1;
    }

    cJSON* threadsArray = cJSON_GetObjectItem(bufferJS, "threads");
    *nb_params = max(cJSON_GetArraySize(threadsArray), 1);
    allParams = malloc(*nb_params * sizeof(ThreadParams));
    allParams[0].fstSharedDepth = 0;

    cJSON* sharerRefsJS = cJSON_GetObjectItem(bufferJS, "sharerRefs");
    for (int i = 0; i < cJSON_GetArraySize(sharerRefsJS); i++) {
        allParams[i].sharers = malloc(n * sizeof(int*));
        for (int j = 0; j < n; j++) {
            allParams[i].sharers[j] = NULL;
        }
    }
    int i = 0;
    cJSON* thread;
    cJSON_ArrayForEach(thread, threadsArray) {
        cJSON* fstSharedDepth = cJSON_GetObjectItem(thread, "fstSharedDepth");
        allParams[i].fstSharedDepth = fstSharedDepth->valueint;
        cJSON* resultJS = cJSON_GetObjectItem(thread, "result");
        allParams[i].result = malloc(n * sizeof(sharing));
        int k = 0;
        cJSON* cJSONEl;
        cJSON_ArrayForEach(cJSONEl, resultJS) {
            cJSON* resultAtt = cJSON_GetObjectItem(cJSONEl, "result");
            cJSON* reservationListInd = cJSON_GetObjectItem(cJSONEl, "reservationListInd");
            allParams[i].result[k].result.result = resultAtt->valueint;
            allParams[i].result[k].result.reservationListInd = reservationListInd->valueint;
            k++;
        }
        cJSON* sharersJS = cJSON_GetObjectItem(thread, "sharers");
        int array_size = cJSON_GetArraySize(sharersJS);
        allParams[i].lstSharedDepth = allParams[i].fstSharedDepth + array_size - 1;
        for (int k = 0; k < array_size; k++) {
            int p = k + allParams[i].fstSharedDepth;
            if (allParams[i].sharers[p] == NULL) {
                int m;
                allParams[i].sharers[p] = &m;
                cJSON* sharerAtt = cJSON_GetArrayItem(sharersJS, k);
                cJSON* sharePointed = cJSON_GetObjectItem(sharerRefsJS, sharerAtt->valuestring);
                cJSON* oneSharer;
                cJSON_ArrayForEach(oneSharer, sharePointed) {
                    int ind = oneSharer->valueint;
                    if (ind != i) {
                        allParams[ind].sharers[p] = &m;
                    }
                }
            }
        }
        i++;
    }
    cleanFile(&hFile, &ov, file, fileContent, json);
}