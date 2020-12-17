#ifndef NOMOV_H
#define NOMOV_H

#include <QDebug>

#include "AddrClassifier.h"

class NoMov
{
    uint8_t*    mpAllocMem;

    uintptr_t   mSelfAddr;    // Address of this object
    QString     mObjID;       // H/S classifier + ID of this object, derived from its addresss

    static constexpr size_t ALLOC_SIZE = 1024;

public:
    NoMov();
    ~NoMov();
};

#endif // NOMOV_H
