#include "nomov.h"

NoMov::NoMov()
{
    mpAllocMem  = nullptr;
    mpAllocMem  = new uint8_t[ALLOC_SIZE];

    mSelfAddr   = reinterpret_cast<uintptr_t>(this);
    mObjID      = mSelfAddr & 0x3ff;    // Generates a 10-bit ID for this object

    qDebug("NoMov %d ctor: Obj addr = %p; Allocation addr = %p", mObjID, this, mpAllocMem);
}

NoMov::~NoMov()
{
    delete [] mpAllocMem;
    mpAllocMem = nullptr;

    qDebug("NoMov %d dtor: Obj addr = 0x%llx", mObjID, mSelfAddr);
}

