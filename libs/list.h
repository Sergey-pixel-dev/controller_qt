#ifndef LIST_H
#define LIST_H

#include <memory>
#include "package.h"


class List // двусвязный FIFO, поток-небезопасен
{
public:
    List();
    ~List();

    void add(std::unique_ptr<Package> pack);
    std::unique_ptr<Package> pop(); // nullptr если пусто
    void free();                    // очистить весь список

    int size() const { return sz; }

    List(const List &) = delete;
    List &operator=(const List &) = delete;

private:
    struct Node
    {
        std::unique_ptr<Package> package;
        Node *prev = nullptr;
        Node *next = nullptr;
    };
    Node *head = nullptr;
    Node *tail = nullptr;
    int sz = 0;
};

#endif
