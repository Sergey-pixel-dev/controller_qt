#include "list.h"
#include <cstring>

List::List() = default;
List::~List() { free(); }

void List::add(std::unique_ptr<Package> pack)
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

std::unique_ptr<Package> List::pop()
{
    if (sz == 0)
        return nullptr;

    std::unique_ptr<Package> ret = std::move(tail->package);

    Node *killer = tail;
    if (sz == 1)
    {
        head = tail = nullptr;
    }
    else
    {
        tail = tail->prev;
        tail->next = nullptr;
    }
    delete killer;
    --sz;
    return ret;
}

void List::free()
{
    while (tail)
    {
        Node *n = tail;
        tail = tail->prev;
        delete n; // unique_ptr внутри Node всё очистит
    }
    head = nullptr;
    sz = 0;
}
