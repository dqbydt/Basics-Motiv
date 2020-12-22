#ifndef RO5_H
#define RO5_H

#include <QDebug>
#include <QString>

#include <cstring>

#include "AddrClassifier.h"

#define objStr()    qPrintable(QString("%1-%2").\
                                        arg(AddrClassifier::classify(reinterpret_cast<uintptr_t>(this))).\
                                        arg(reinterpret_cast<uintptr_t>(this) & 0x3ff, 3, 10, QLatin1Char('0')))
#define selfStr()   qPrintable(AddrClassifier::classifyFull(reinterpret_cast<uintptr_t>(this)))
#define allocStr()  qPrintable(AddrClassifier::classifyFull(reinterpret_cast<uintptr_t>(cstring)))

class Ro5
{
    char* cstring; // raw pointer used as a handle to a dynamically-allocated memory block
 public:
    Ro5(const char* s = "")
    : cstring(nullptr)  {
        if (s) {
            std::size_t n = std::strlen(s) + 1;
            cstring = new char[n];      // allocate
            std::memcpy(cstring, s, n); // populate

            qDebug("Ro5 %s CTOR: Obj addr = %s; Alloc addr = %s",
                   objStr(), selfStr(), allocStr());
        }
    }

    ~Ro5()  {
        qDebug("Ro5 %s DTOR: Obj addr = %s", objStr(), selfStr());
        delete[] cstring;  // deallocate
    }

    Ro5(const Ro5& other) // copy constructor
    : Ro5(other.cstring)    {
        qDebug("Ro5 %s CC  : Obj addr = %s; Alloc addr = %s",
               objStr(), selfStr(), allocStr());
    }

    Ro5(Ro5&& other) noexcept // move constructor
    : cstring(std::exchange(other.cstring, nullptr))    {
        qDebug("Ro5 %s MC  : Obj addr = %s; Alloc addr = %s",
               objStr(), selfStr(), allocStr());
    }

    Ro5& operator=(const Ro5& other)    {   // copy assignment
        qDebug("Ro5 %s CAO : Obj addr = %s; Alloc addr = %s",
               objStr(), selfStr(), allocStr());
        qDebug("CAO calling MAO");

        return *this = Ro5(other);
    }

    Ro5& operator=(Ro5&& other) noexcept  { // move assignment
        std::swap(cstring, other.cstring);
        qDebug("Ro5 %s MAO : Obj addr = %s; Alloc addr = %s",
               objStr(), selfStr(), allocStr());
        return *this;
    }

// alternatively, replace both assignment operators with
//  Ro5& operator=(Ro5 other) noexcept
//  {
//      std::swap(cstring, other.cstring);
//      return *this;
//  }
};

#endif // RO5_H
