#ifndef PACKAGE_H
#define PACKAGE_H

template<typename T> // Исправлено: добавлен <typename T>
class Package
{
public:
    Package(T *pBuf, int sz);
    Package() = default;
    ~Package();

    int size = 0;
    T *packageBuf = nullptr;

    Package(const Package &) = delete;
    Package &operator=(const Package &) = delete;
};

template<typename T>
Package<T>::Package(T *pBuf, int sz)
{
    packageBuf = pBuf;
    size = sz;
}

template<typename T>
Package<T>::~Package()
{
    if (packageBuf != nullptr) {
        delete[] packageBuf;
        packageBuf = nullptr;
        size = 0;
    }
}

#endif
