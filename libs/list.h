#ifndef LIST_H
#define LIST_H

#include <memory>

template<typename T>
class List // двусвязный FIFO, поток-небезопасен
{
public:
    List();
    ~List();
    void add(std::unique_ptr<T> pack);
    std::unique_ptr<T> pop(); // nullptr если пусто
    void free();              // очистить весь список
    int size() const { return sz; }

    List(const List &) = delete;
    List &operator=(const List &) = delete;

private:
    struct Node
    {
        std::unique_ptr<T> package;
        Node *prev = nullptr;
        Node *next = nullptr;
    };

    Node *head = nullptr;
    Node *tail = nullptr;
    int sz = 0;
};

// Реализация методов

template<typename T>
List<T>::List() = default;

template<typename T>
List<T>::~List()
{
    free();
}

template<typename T>
void List<T>::add(std::unique_ptr<T> pack)
{
    if (!pack)
        return;

    Node *node = new Node();
    node->package = std::move(pack);
    node->next = head;

    if (sz)
        head->prev = node;

    head = node;

    if (sz == 0)
        tail = head;

    ++sz;
}

template<typename T>
std::unique_ptr<T> List<T>::pop()
{
    if (sz == 0)
        return nullptr;

    std::unique_ptr<T> ret = std::move(tail->package);
    Node *killer = tail;

    if (sz == 1) {
        head = tail = nullptr;
    } else {
        tail = tail->prev;
        tail->next = nullptr;
    }

    delete killer;
    --sz;
    return ret;
}

template<typename T>
void List<T>::free()
{
    while (tail) {
        Node *n = tail;
        tail = tail->prev;
        delete n; // unique_ptr внутри Node всё очистит
    }

    head = nullptr;
    sz = 0;
}

#endif
