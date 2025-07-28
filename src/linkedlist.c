#include "linkedlist.h"
#include "io_helpers.h"


Node* createNode(char* key, char* value, int idx) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        display_error("Memory allocation failed!", "");
        return NULL;
    }
    newNode->idx = idx;
    strncpy(newNode->key, key, 129);
    newNode->key[128] = '\0';
    strncpy(newNode->value, value, 129);
    newNode->value[128] = '\0';
    newNode->next = NULL;
    newNode->status = 0;
    return newNode;
}

int insert(Node** head, char* key, char* value) {
    if (*head == NULL) {
	    Node* newNode = createNode(key, value, 0);
        if (newNode == NULL) {
            return -1;
        }
        *head = newNode;
        return 0;
    }
    int idx = 0;
    Node* temp = *head;
    Node* last = *head;
    int keylen = strnlen(key, 128);
    while (temp != NULL) {
        if (strncmp(temp->key, key, keylen) == 0){
            strncpy(temp->value, value, 129);
            return 0;
        }
        idx++;
        last = temp;
        temp = temp->next;
    }
    Node* newNode = createNode(key, value, idx);
    if (newNode == NULL) {
        return -1;
    }
    last->next = newNode;
    return 0;
}

Node* searchNode(Node* head, char* key) {
    Node* temp = head;
    while (temp != NULL) {
        if (strncmp(temp->key, key, 129) == 0) {
            return temp;
        }
        temp = temp->next;
    }
    return NULL;
}

void freeList(Node* head) {
    Node* temp = head;
    while (temp != NULL) {
        Node* next = temp->next;
        free(temp);
        temp = next;
    }
}

void deleteNode(Node** head, char* key) {
    Node* temp = *head;
    Node* prev = NULL;
    while (temp != NULL) {
        if (strncmp(temp->key, key, 129) == 0) {
            if (prev == NULL) {
                *head = temp->next;
            } else {
                prev->next = temp->next;
            }
            free(temp);
            return;
        }
        prev = temp;
        temp = temp->next;
    }
}
