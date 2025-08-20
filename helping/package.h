#ifndef PACKAGE_H
#define PACKAGE_H
#include <cstdint>

struct Package // простая структура-контейнер
{
    Package(uint8_t *pBuf, int sz);
    Package() = default;
    ~Package();

    int size = 0;
    uint8_t *packageBuf = nullptr;

    Package(const Package &) = delete;
    Package &operator=(const Package &) = delete;
};
#endif