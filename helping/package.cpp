#include "package.h"
Package::Package(uint8_t *pBuf, int sz)
{
    packageBuf = pBuf;
    size = sz;
}
Package::~Package()
{
    if (packageBuf != nullptr)
        delete[] packageBuf;
    packageBuf = nullptr;
    size = 0;
}