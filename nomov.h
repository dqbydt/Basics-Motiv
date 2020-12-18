#ifndef NOMOV_H
#define NOMOV_H

#include <QDebug>

#include "AddrClassifier.h"

class NoMov
{
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
};

#endif // NOMOV_H
