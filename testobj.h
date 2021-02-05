#ifndef TESTOBJ_H
#define TESTOBJ_H

#include <QDebug>

#include "mov.h"
#include "nomov.h"

class TestObjM {
private:
    Mov m1;
    Mov m2;
public:
    // When constructed as TestObjM tom{Mov(), Mov()} -- causes
    // 1. CTORs for temp objs
    // 2. CCs   for m1, m2
    // 3. DTORs for temp objs
    // Adding a std::move on the args does nothing, since they have been declared const
    // and thus cannot be moved from.
    //TestObjM(const Mov& first, const Mov& second) : m1{first}, m2{second} {
    //TestObjM(const Mov& first, const Mov& second) : m1{std::move(first)},  m2{std::move(second)} {
    //    qDebug("TestObjM cref ctor");
    //}

    // Non-const lvalue ref args: cannot be initialized with temp objects, cannot be
    // constructed as TestObjM tom{Mov(), Mov()}
    //TestObjM(Mov& first, Mov& second) : m1{std::move(first)},  m2{std::move(second)} {
    //    qDebug("TestObjM lref ctor");
    //}

    // If constructed as TestObjM tom{Mov(), Mov()} -- same as cref construction
    // If constructed as TestObjM tom{std::move(Mov()), std::move(Mov())} -- same as cref construction
    //  AND note that std::move on a temp object is redundant, Mov() is already an rvalue, so casting it
    //  further to an rvalue (which is what std::move does) is pointless.
    //TestObjM(Mov&& first, Mov&& second) : m1{first}, m2{second} {
    //TestObjM(Mov&& first, Mov&& second) : m1{std::move(first)}, m2{std::move(second)} {
    //    qDebug("TestObjM move ctor");
    //}

    TestObjM(Mov first, Mov second) : m1{std::move(first)}, m2{std::move(second)} {
        qDebug("TestObjM by-val ctor");
    }
};

#endif // TESTOBJ_H
