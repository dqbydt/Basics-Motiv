#ifndef NOMOV_H
#define NOMOV_H

#include <QDebug>

#include "AddrClassifier.h"

// Various strings for debug printout of ID/addresses.
// NOTE: Had initially made these as private members, but that does not work!
// qPrintable produces a char*, and you cannot return a char* from a function
// unless it has been allocated on the heap (and must therefore be freed by the
// caller)!
#define objStr()    qPrintable(mObjIDString)
#define selfStr()   qPrintable(AddrClassifier::classifyFull(mSelfAddr))
#define allocStr()  qPrintable(AddrClassifier::classifyFull(mAllocAddr))

class NoMov
{
private:
    uint32_t*   mpAllocMem;
    uint32_t    mObjID;         // ID of this object
    QString     mObjIDString;   // H/S classifier + ID of this object, derived from its addresss
    uintptr_t   mSelfAddr;
    uintptr_t   mAllocAddr;

    static constexpr size_t ALLOC_SIZE = 64;

    void init();   // Get addr of this object

public:
    NoMov();

    // Non-movable type that manages a resource: Rule of Three applies
    // https://en.cppreference.com/w/cpp/language/rule_of_three
    ~NoMov();               // I.   Dtor
    NoMov(const NoMov& o);  // II.  Copy ctor
    NoMov& operator=(const NoMov& that);    // III. Copy Assignment Operator

    // Retval is const to prevent it from being used as an lvalue. e.g.
    // cannot say (nm1 + nm2) = nm3.
    const NoMov operator+(const NoMov& rhs) const;
};

#endif // NOMOV_H
