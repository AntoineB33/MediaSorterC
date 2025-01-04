#pragma once
// my_structs.h
#ifndef MY_STRUCTS_H
#define MY_STRUCTS_H

#include "all_func.h"


typedef struct {
    void* data;       // Pointer to the array (generic list)
    size_t size;      // Number of elements in the list
    size_t capacity;  // Capacity of the list
    size_t elemSize;  // Size of each element
} GenericList;

typedef struct {
    int start;
    int end;
} allowedPlaceSt;

// Define a structure to represent the graph node
typedef struct {
    int* ulteriors;	 // List of integers that must come after
    int nb_ulteriors;
    char** conditions;	   // List of conditions that must be satisfied
    int nb_conditions;
    Node** groups;
    int nb_groups;
    allowedPlaceSt* allowedPlaces;
    int nb_allowedPlaces;
    char** errorFunctions;
    int nb_errorFunctions;
    int nbPost;            // Number of nodes that must come before
} Node;

typedef struct {
    int all;            // 1 if at this depth any one can be chosen in the consecutive resLists, 0 otherwise
    int resList;
} ReservationRule;

typedef struct {
    GenericList data;	// List of the nodes that are in the reservation list
    int out;            // index of the first element that is not in the list
    int prevListInd;
    int nextListInd;
} ReservationList;      // List of the nodes that need to be placed at a certain depth

typedef struct {
    int lastOcc;
} AttributeMng;

typedef struct {
    int result;
    int reservationListInd;
} resultSt;

typedef struct {
    resultSt result;
    int nodeId;
} sharing;

typedef struct {
    resultSt result;
    int* sharers;
} sharingRef;

typedef struct {
    int sep;
    int lastPlace;
} attributeSt;

typedef struct problemSt problemSt;

typedef struct {
    problemSt* problem;
    int fstSharedDepth;
	int lstSharedDepth;
    sharing* result;
    sharingRef** sharers;
} ThreadParams;

typedef struct {
    char* sheetPath;
    char* sheetID;
    Node* nodes;
    int n;
    attributeSt* attributes;
    int nb_att;
    int nb_process_on_it;
    ThreadParams* allParams;
    int nb_params;
    int prbInd;
} problemSt;

typedef struct {
    const char* sort_info_path;
    problemSt* allProblems;
    int nb_allProblems;
    HANDLE* mainSemaphore;
    HANDLE* semaphore;
    HANDLE* threadEvents;
    int nb_process;
    int* awaitingThrds;
    int nb_awaitingThrds;
    int* activeThrd;
    int error;
    ThreadParams* allParams;
    int nb_params;
} MeetPoint;

typedef struct {
    MeetPoint* meetPoint;
    int rank;
} ThreadParamsRank;

// Define the structure of an element
typedef struct {
    char stringField[256];
    int intField;
    int* listOfInts;
    int listOfIntsSize;
    int** listOfListOfInts;
    int listOfListOfIntsSize;
    int* listSizes; // Stores sizes of each sub-list in listOfListOfInts
} Element;

#endif // MY_STRUCTS_H