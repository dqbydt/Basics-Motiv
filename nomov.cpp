#include "nomov.h"

void NoMov::init()
{
    mSelfAddr = reinterpret_cast<uintptr_t>(this);
    mObjID = mSelfAddr & 0x3ff;    // Generates a 10-bit ID for this object
    mObjIDString = QString("%1-%2").arg(AddrClassifier::classify(mSelfAddr)).arg(mObjID, 3, 10, QLatin1Char('0'));

    mpAllocMem = new uint32_t[ALLOC_SIZE];    // Allocate mem
    mAllocAddr = reinterpret_cast<uintptr_t>(mpAllocMem);
}

NoMov::NoMov()
{
    init();
    std::fill_n(mpAllocMem, ALLOC_SIZE, mObjID);    // Fill array with our id

    qDebug("NoMov %s CTOR: Obj addr = %s; Alloc addr = %s, contents %d",
           qPrintable(mObjIDString),
           qPrintable(AddrClassifier::classifyFull(mSelfAddr)),
           qPrintable(AddrClassifier::classifyFull(mAllocAddr)),
           mpAllocMem[0]);
}

NoMov::~NoMov()
{
    qDebug("NoMov %s DTOR: Obj addr = %s; Alloc contents %d",
           qPrintable(mObjIDString),
           qPrintable(AddrClassifier::classifyFull(mSelfAddr)),
           mpAllocMem[0]);

    delete [] mpAllocMem;
}

// Copy Ctor
NoMov::NoMov(const NoMov &o)
{
    init();
    std::copy_n(o.mpAllocMem, ALLOC_SIZE, this->mpAllocMem);    // Copy from other (will get o's id!)

    qDebug("NoMov %s CC  : Obj addr = %s; Alloc addr = %s, contents %d",
           qPrintable(mObjIDString),
           qPrintable(AddrClassifier::classifyFull(mSelfAddr)),
           qPrintable(AddrClassifier::classifyFull(mAllocAddr)),
           mpAllocMem[0]);
}

// Copy Assignment Operator
NoMov &NoMov::operator=(const NoMov &that)
{
    if (this == &that) {
        qDebug("NoMov %s CAO : Obj addr = %s; Alloc addr = %s, contents %d",
               qPrintable(mObjIDString),
               qPrintable(AddrClassifier::classifyFull(mSelfAddr)),
               qPrintable(AddrClassifier::classifyFull(mAllocAddr)),
               mpAllocMem[0]);

        return *this;   // Assigning to self
    }

    uint32_t* newAlloc = new uint32_t[ALLOC_SIZE];      // Allocate new mem
    std::copy_n(that.mpAllocMem, ALLOC_SIZE, newAlloc); // Copy from that (will get that's id!)
    delete [] mpAllocMem;   // Release original mem
    mpAllocMem = newAlloc;  // Reassign to new
    qDebug("NoMov %s CAO : Obj addr = %s; Alloc addr = %s, contents %d",
           qPrintable(mObjIDString),
           qPrintable(AddrClassifier::classifyFull(mSelfAddr)),
           qPrintable(AddrClassifier::classifyFull(mAllocAddr)),
           mpAllocMem[0]);

    return *this;
}

