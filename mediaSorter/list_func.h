#pragma once
#ifndef LIST_FUNC_H
#define LIST_FUNC_H

#include "all_func.h"


void initList(GenericList* list, size_t elemSize, size_t initialCapacity);
void* getLastElement(GenericList* list);
void addElement(GenericList* list, void* element);
void removeElement(GenericList* list, int element);
void removeIndGL(GenericList* list, int ind);
void removeInd(void** list, int* listSize, int ind, size_t elemSize);
void freeList(GenericList* list);
void freeNode(Node* node);
void freeNodes(Node* nodes, size_t nodeCount);


#endif