#ifndef LIST_H
#define LIST_H

#include "macro.h"

#include <stddef.h>



class List {
public:
    struct Node {
        long val;
        Node* next;
    };

    List() : size(0), head(nullptr), current(nullptr) {}
    Node* getHead() const {
        return head;
    }
    int add(const long value) {
        Node* newNode = new Node{value, nullptr};
        if (head == nullptr) {
            head = newNode;
            current = newNode;
        } else {
            current->next = newNode;
            current = newNode;
        }
        size++;
        return 0;
    }

    int getSize() const {
        return size;
    }

    int copyTo(long array[]) const {
        Node* tempNode = head;
        for (int i = 0; i < size; ++i) {
            array[i] = tempNode->val;
            tempNode = tempNode->next;
        }
        return 0;
    }

    ~List() {
        while (head != nullptr) {
            Node* tempNode = head->next;
            delete head;
            head = tempNode;
        }
    }

private:
    int size;
    Node* head;
    Node* current;
};

class ListIterator {
public:

    ListIterator(const List& list) : pos(list.getHead()) {}
    int walk(long& value) {
        if (pos == nullptr) {
            return 1;
        }

        value = pos->val;
        pos = pos->next;
        return 0;
    }

    ~ListIterator() {}

private:
    List::Node* pos;
};



#endif /* LIST_H */
