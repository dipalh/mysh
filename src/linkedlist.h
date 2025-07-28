#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Node {
    char key[129];
    char value[129];
    int status;
    int idx;
    struct Node* next;
} Node;

Node* createNode(char* key, char* value, int idx);
int insert(Node** head, char* key, char* value);
Node* searchNode(Node* head, char* key);
void freeList(Node* head);
void deleteNode(Node** head, char* key);


#endif
