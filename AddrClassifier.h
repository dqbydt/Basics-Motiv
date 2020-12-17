#ifndef ADDRCLASSIFIER_H
#define ADDRCLASSIFIER_H

#include <stdint.h>
#include <QString>

struct AddrClassifier {
    static uintptr_t stackTop, stackBot;

    // Because there seems to be no way of deterministically determining heap addresses
    // on either Windows or Linux, we just classify an address as H if it is not within
    // stackTop and Bot.

    // std::clamp: if val < lo, returns lo
    //             if hi < val, returns hi
    //             else returns val
    // So: if val has been returned then lo <= val <= hi
    static QString classify(uintptr_t a) {
        if (a == std::clamp(a, stackBot, stackTop))
            return "S";
        else
            return "H";
    }

};

#endif // ADDRCLASSIFIER_H
