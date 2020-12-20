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
           objStr(), selfStr(), allocStr(), mpAllocMem[0]);
}

NoMov::~NoMov()
{
    qDebug("NoMov %s DTOR: Obj addr = %s; Alloc contents %d",
           objStr(), selfStr(), mpAllocMem[0]);

    delete [] mpAllocMem;
}

// Copy Ctor
NoMov::NoMov(const NoMov &o)
{
    init();
    // Deep copy from o (will get o's id!)
    std::copy_n(o.mpAllocMem, ALLOC_SIZE, this->mpAllocMem);

    qDebug("NoMov %s CC  : Obj addr = %s; Alloc addr = %s, contents %d",
           objStr(), selfStr(), allocStr(), mpAllocMem[0]);
}

// Copy Assignment Operator
NoMov &NoMov::operator=(const NoMov &that)
{
    if (this == &that) {
        qDebug("NoMov %s CAO : Obj addr = %s; Alloc addr = %s, contents %d",
               objStr(), selfStr(), allocStr(), mpAllocMem[0]);

        return *this;   // Assigning to self
    }

    uint32_t* newAlloc = new uint32_t[ALLOC_SIZE];      // Allocate new mem
    std::copy_n(that.mpAllocMem, ALLOC_SIZE, newAlloc); // Copy from that (will get that's id!)
    delete [] mpAllocMem;   // Release original mem
    mpAllocMem = newAlloc;  // Reassign to new
    qDebug("NoMov %s CAO : Obj addr = %s; Alloc addr = %s, contents %d",
           objStr(), selfStr(), allocStr(), mpAllocMem[0]);

    return *this;
}

// Retval is const to prevent it from being used as an lvalue. e.g.
// cannot say (nm1 + nm2) = nm3.
// https://www3.ntu.edu.sg/home/ehchua/programming/cpp/cp7_OperatorOverloading.html
const NoMov NoMov::operator+(const NoMov &rhs) const
{
    qDebug("In NoMov::operator+; will add objID %d", rhs.mObjID);
    NoMov ret;
    // Add contents of rhs allocation --
    // Note, iterator to last must be one-past-the-end
    // Equivalent to the following:
    //  for (; first != last; ++first) {
    //      f(*first);
    //  }
    // In the lambda, must capture vars by ref in order to access
    // the rhs object.
    std::for_each(ret.mpAllocMem, ret.mpAllocMem+ALLOC_SIZE,
                  [&](uint32_t& n){ n += rhs.mObjID; }
    );

    qDebug("NoMov::operator+ ret obj %d now has contents %d", ret.mObjID, ret.mpAllocMem[0]);
    return ret;
}

