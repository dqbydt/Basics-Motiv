#ifndef NOMOV_H
#define NOMOV_H

#include <QDebug>

class NoMov
{
    uint8_t*    mpAllocMem;

    uintptr_t   mSelfAddr;    // Address of this object
    uint32_t    mObjID;       // ID of this object, derived from its addresss

    static constexpr size_t ALLOC_SIZE = 1024;

public:
    NoMov();
    ~NoMov();
};

#endif // NOMOV_H
