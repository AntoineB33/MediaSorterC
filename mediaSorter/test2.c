#include "all_func.h"

void test2(HANDLE* hFile, OVERLAPPED* ov) {
    UnlockFileEx(*hFile, 0, MAXDWORD, MAXDWORD, ov);
    CloseHandle(*hFile);
}

void unlockFile(HANDLE* hFile, OVERLAPPED* ov) {
    UnlockFileEx(*hFile, 0, MAXDWORD, MAXDWORD, ov);
    CloseHandle(*hFile);
}
int lockFile(HANDLE* hFile, OVERLAPPED* ov, const char* filePath) {
    *hFile = CreateFileA(
        filePath,
        GENERIC_READ | GENERIC_WRITE,
        0,              // No sharing
        NULL,           // Default security
        OPEN_EXISTING,  // Open an existing file
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Failed to open file. Error: %ld\n", GetLastError());
        return -1;
    }

    ZeroMemory(ov, sizeof(OVERLAPPED));

    // Keep trying to lock until successful
    while (!LockFileEx(hFile, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, ov)) {
        Sleep(1000); // Wait for 1 second before retrying
    }

    return 0;
}


void removeInd(void** list, int* listSize, int ind, size_t elemSize) {
    listSize--;  // Reduce the size after removing the element
    // Shift the elements after the index to the left
    for (int i = ind; i < *listSize; i++) {
        void* src = (char*)*list + (i + 1) * elemSize;
        void* dst = (char*)*list + i * elemSize;
        memcpy(dst, src, elemSize);
    }
    *list = realloc(*list, *listSize * elemSize);
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

//get last element
void* getLastElement(GenericList* list) {
    return (char*)list->data + (list->size - 1) * list->elemSize;
}

// Function to add an element to the list
void addElement(GenericList* list, void* element) {
    // Check if the list is full and resize if needed
    if (list->size == list->capacity) {
        list->capacity++;
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

// Function to free the list
void freeList(GenericList* list) {
    free(list->data);
    list->size = 0;
    list->capacity = 0;
}