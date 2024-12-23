// Our implementation of Linked List

#include "list.h"
#include <stdio.h>
#include <stdlib.h>


void append(struct list_item *first, int x) {
    // Allocate memory for a new node
    struct list_item* newNode = (struct list_item*)malloc(sizeof(struct list_item));
    newNode->value = x;
    newNode->next = NULL;

    // Traverse the list to find the last node
    struct list_item* current = first;
    while (current->next != NULL) {
        current = current->next;
    }

    // Append the new node to the end of the list
    current->next = newNode;
}


void prepend(struct list_item *first, int x) {
    // Allocate memory for a new node
    struct list_item* newNode = (struct list_item*)malloc(sizeof(struct list_item));
    struct list_item* firstNode = first->next;

    newNode->value = x;
    newNode->next = firstNode;

    // Prepend the new node to the begining of the list
    first->next = newNode;
}


void input_sorted(struct list_item *first, int x) {
    // Insert the new node in sorted order
    if (first->next != NULL) {
        while(first->next->value <= x) {
            first = first->next;
            if (first->next == NULL) {
                break;
            }
        }
    }

    // Call prepend to insert the new node
    prepend(first, x); 
}


void print(struct list_item *first){
    // Print the elements of the linked list
    struct list_item* currentNode = first->next;
    while (currentNode != 0) {
        printf("%d\n", currentNode->value);
        currentNode = currentNode->next;
    }

}


void clear(struct list_item *first){
    // Free the memory allocateed for each node in the list
    struct list_item *currentNode;
    currentNode = first;
    first = first->next;
    currentNode->next = NULL;

    while(first != NULL){
        currentNode = first;
        first = first->next;
        free(currentNode);
    }   
}
