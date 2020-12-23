#include "mov.h"

// Inits ID and mem allocation for this obj
void Mov::init()
{
    initLite();
    mpAllocMem = new uint32_t[ALLOC_SIZE];    // Allocate mem
    mAllocAddr = reinterpret_cast<uintptr_t>(mpAllocMem);
}

// Used in mov functions - only inits the ID; allocation will be
// stolen from victim src object.
void Mov::initLite()
{
    mSelfAddr = reinterpret_cast<uintptr_t>(this);
    mObjID = mSelfAddr & 0x3ff;    // Generates a 10-bit ID for this object
    // Creates id string as S-031 for instance.
    // The 2nd arg below formats number as fieldwidth 3, decimal, padded with 0.
    mObjIDString = QString("%1-%2").arg(AddrClassifier::classify(mSelfAddr)).arg(mObjID, 3, 10, QLatin1Char('0'));
}

Mov::Mov()
{
    init();

    // https://en.cppreference.com/w/cpp/algorithm/fill_n
    std::fill_n(mpAllocMem, ALLOC_SIZE, mObjID);    // Fill array with our id

    qDebug("Mov %s CTOR: Obj addr = %s; Alloc addr = %s, contents %d",
           objStr(), selfStr(), allocStr(), mpAllocMem[0]);
}

Mov::~Mov()
{
    // C++ guarantees that delete p will do nothing if p is null!
    // https://isocpp.org/wiki/faq/freestore-mgmt#delete-handles-null
    // This means that even if the value has been moved away, we don't
    // need to check for nullptr on the allocation before calling delete
    // here.
    // Will print -1 if obj was moved from
    qDebug("Mov %s DTOR: Obj addr = %s; Alloc contents %d",
           objStr(), selfStr(), (mpAllocMem)? mpAllocMem[0] : -1);

    delete [] mpAllocMem;
}

// Copy Ctor
Mov::Mov(const Mov &o)
{
    init();
    // Deep copy from o (will get o's id!)
    // https://en.cppreference.com/w/cpp/algorithm/copy_n
    std::copy_n(o.mpAllocMem, ALLOC_SIZE, this->mpAllocMem);

    qDebug("Mov %s CC  : Obj addr = %s; Alloc addr = %s, contents %d",
           objStr(), selfStr(), allocStr(), mpAllocMem[0]);

}

// Mov Ctor
// std::exchange specifically intended for use in move operations:
// https://en.cppreference.com/w/cpp/utility/exchange
// See GDoc "Modern C++ Move Semantics"; search for Jason Turner to
// understand diff betn exchange and swap.
// Only exchange can move a constant value (nullptr here) into the other
// variable, so that's what we need for the Mov Ctor. In
// y = std::exchange(x,k);
// y <- x, x <- k
Mov::Mov(Mov &&o) : mpAllocMem{std::exchange(o.mpAllocMem, nullptr)},
                    mAllocAddr{std::exchange(o.mAllocAddr, 0x0)}
{
    initLite();

    // allocAddr and contents will be from the erstwhile o obj
    qDebug("Mov %s MC  : Obj addr = %s; Alloc addr = %s, contents %d",
           objStr(), selfStr(), allocStr(), mpAllocMem[0]);

}

// CAO
Mov &Mov::operator=(const Mov &that)
{

    qDebug("Mov %s CAO : Obj addr = %s; Alloc addr = %s, contents %d",
           objStr(), selfStr(), allocStr(), mpAllocMem[0]);
    qDebug("CAO calling MAO");
    // RHS below is automatically an rvalue since it is unnamed.
    // So a std::move around it is unnecessary. In the statement below:
    // 1. CC is called with arg that to construct the RHS object
    // 2. MAO is called to reassign *this
    // 3. DTOR is called to destroy the temp RHS object
    return *this = Mov(that);
}

// MAO
Mov& Mov::operator=(Mov&& that)
{
    // https://en.cppreference.com/w/cpp/algorithm/swap
    // Our mem is transferred into that. When that gets destroyed eventually,
    // that mem will be released.
    std::swap(mpAllocMem, that.mpAllocMem);
    qDebug("Mov %s MAO : Obj addr = %s; Alloc addr = %s, contents %d",
           objStr(), selfStr(), allocStr(), mpAllocMem[0]);
    return *this;
}

// Retval is const to prevent it from being used as an lvalue. e.g.
// cannot say (nm1 + nm2) = nm3.
// https://www3.ntu.edu.sg/home/ehchua/programming/cpp/cp7_OperatorOverloading.html
const Mov Mov::operator+(const Mov &rhs) const
{
    qDebug("In Mov::operator+; will add objID %d", rhs.mObjID);
    Mov ret;
    // Add contents of rhs allocation --
    // Note, iterator to last must be one-past-the-end in for_each
    // Equivalent to the following:
    //  for (; first != last; ++first) {
    //      f(*first);
    //  }
    // In the lambda, must capture vars by ref in order to access
    // the rhs object.
    std::for_each(ret.mpAllocMem, ret.mpAllocMem+ALLOC_SIZE,
                  [&](uint32_t& n){ n += rhs.mObjID; }
    );

    qDebug("Mov::operator+ ret obj %d now has contents %d", ret.mObjID, ret.mpAllocMem[0]);
    return ret;
}

