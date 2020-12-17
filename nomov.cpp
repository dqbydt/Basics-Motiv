#include "nomov.h"

NoMov::NoMov()
{
    mpAllocMem  = nullptr;
    mpAllocMem  = new uint8_t[ALLOC_SIZE];

    mSelfAddr   = reinterpret_cast<uintptr_t>(this);
    uintptr_t id = mSelfAddr & 0x3ff;    // Generates a 10-bit ID for this object

    mObjID = QString("%1-%2").arg(AddrClassifier::classify(mSelfAddr)).arg(id);

    int i = 10;
    qDebug("NoMov %s ctor: Obj addr = %p; Allocation addr = %p; stack var i addr = %p",
           qPrintable(mObjID), this, mpAllocMem, &i);
}

NoMov::~NoMov()
{
    delete [] mpAllocMem;
    mpAllocMem = nullptr;

    qDebug("NoMov %s dtor: Obj addr = 0x%llx", qPrintable(mObjID), mSelfAddr);
}

