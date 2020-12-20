#ifndef ADDRCLASSIFIER_H
#define ADDRCLASSIFIER_H

#include <stdint.h>
#include <QString>

// For generic addresses. Call as acStr(&var)
#define acStr(a)    qPrintable(AddrClassifier::classifyFull(reinterpret_cast<uintptr_t>(a)))

// https://stackoverflow.com/a/9255906/3367247
// Had used this to attempt to compile with C++03 (because std::clamp exists only
// in C++17 and up, but there is no way to get that to work :(
template <typename T>
bool withinRange(const T& value, const T& low, const T& high) {
    return !(value < low) && (value < high);
}

struct AddrClassifier {

    static uintptr_t stackTop, stackBot;

    // Because there seems to be no way of deterministically determining heap address ranges
    // on either Windows or Linux, we just classify an address as H if it is not on the stack.

    // std::clamp: if val < lo, returns lo
    //             if hi < val, returns hi
    //             else returns val
    // So: if val has been returned then lo <= val <= hi
    static QString classify(uintptr_t a) {
        // if (withinRange(a, stackBot, stackTop))     // Pre c++1z
        if (a == std::clamp(a, stackBot, stackTop)) // For c++1z
            return "S";
        else
            return "H";
    }

    // Prints "S-0x006efcf0" for instance
    static QString classifyFull(uintptr_t a) {
        // https://stackoverflow.com/a/16568641/3367247
        // asprintf() NRND per https://doc.qt.io/qt-5/qstring.html#asprintf,
        // but going to let that pass here.
        return classify(a) + QString::asprintf("-0x%08llx", a);
    }

};

#endif // ADDRCLASSIFIER_H
