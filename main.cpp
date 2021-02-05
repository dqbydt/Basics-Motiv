#include <QCoreApplication>

#include <vector>
#include <utility>
#include <functional>

#include <malloc.h>
#include <intsafe.h>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "nomov.h"
#include "mov.h"
#include "ro5.h"
#include "testobj.h"
#include "AddrClassifier.h"

// Define storage for AddrClassifier members:
uintptr_t AddrClassifier::stackTop;
uintptr_t AddrClassifier::stackBot;

// For overloading the createAndInsert function with dummy arg differences
// (Remember that you cannot overload a function by ret type)
// https://stackoverflow.com/a/34095060/3367247
constexpr uint32_t  DUMMY_NM  = 'N';
constexpr int32_t   DUMMY_M   = 'M';

// Tries to determine stack and heap on various platforms
void queryStackHeap()
{
#ifdef Q_OS_WIN

    // Link to my SO post detailing the strange case of perverse allocation addresses
    // seen on Win10:
    // https://stackoverflow.com/q/65298802/3367247

    // https://docs.microsoft.com/en-us/windows/win32/memory/getting-process-heaps
    // 1.   Retrieve the number of active heaps for the current process
    //      so we can calculate the buffer size needed for the heap handles.
    DWORD NumberOfHeaps;
    NumberOfHeaps = GetProcessHeaps(0, NULL);
    Q_ASSERT(NumberOfHeaps > 0);

    // 2.   Calculate the buffer size needed to store heap handles.
    //      Size = sizeof(HANDLE) * numHeaps
    HRESULT Result;
    PHANDLE aHeaps;
    SIZE_T BytesToAllocate;
    Result = SizeTMult(NumberOfHeaps, sizeof(*aHeaps), &BytesToAllocate);   // Bytes <-- NumHeaps*sizeof(HANDLE)
    Q_ASSERT(Result == S_OK);

    // 3.   Get a handle to the default process heap.
    HANDLE hDefaultProcessHeap;
    hDefaultProcessHeap = GetProcessHeap();
    Q_ASSERT(hDefaultProcessHeap != NULL);

    // 4.   Allocate the buffer to store heap handles, from the default process heap.
    aHeaps = (PHANDLE)HeapAlloc(hDefaultProcessHeap, 0, BytesToAllocate);
    Q_ASSERT_X((aHeaps != NULL), "main", "HeapAlloc failed to allocate space for buffer");

    // 5.   Save the original number of heaps because we are going to compare it
    //      to the return value of the next GetProcessHeaps call.
    DWORD HeapsLength = NumberOfHeaps;

    // 6.   Retrieve handles to the process heaps and print.
    NumberOfHeaps = GetProcessHeaps(HeapsLength, aHeaps);
    Q_ASSERT(NumberOfHeaps > 0);

    qDebug("Num heaps = %lu", NumberOfHeaps);
    DWORD HeapsIndex;
    for (HeapsIndex = 0; HeapsIndex < HeapsLength; ++HeapsIndex) {

        // https://j00ru.vexillium.org/2016/07/disclosing-stack-data-from-the-default-heap-on-windows/
        MEMORY_BASIC_INFORMATION mbi = { };
        SIZE_T heapStart = (SIZE_T) aHeaps[HeapsIndex];
        assert(VirtualQuery(aHeaps[HeapsIndex], &mbi, sizeof(mbi)) != 0);
        SIZE_T heapEnd = heapStart + mbi.RegionSize;

        qDebug("Heap %lu at address: %p - 0x%llx, size %llu bytes", HeapsIndex, aHeaps[HeapsIndex], heapEnd, mbi.RegionSize);
    }

    // 7.   Release memory allocated for buffer from default process heap.
    if (HeapFree(hDefaultProcessHeap, 0, aHeaps) == FALSE) {
        qDebug("Failed to free allocation from default process heap.");
    }

#endif

#ifdef Q_OS_LINUX
    // Read from /proc/self/maps, parse out stack and heap


#endif

    size_t stackObject;
    size_t stackBot = (((size_t) &stackObject) & ~0xfff);  // For a 4kB stack frame
    size_t stackTop = stackBot + 0xfff;

    AddrClassifier::stackTop = stackTop;
    AddrClassifier::stackBot = stackBot;

    qDebug("Stack: 0x%llx - 0x%llx (Sample stack obj address: %p)\n", stackTop, stackBot, &stackObject);
}

// Unused in-param used merely to effect overload
std::vector<NoMov> createAndInsert(uint32_t dummy_arg_for_ovl)
{
    Q_UNUSED(dummy_arg_for_ovl)
    std::vector<NoMov> coll;    // Vector of NoMovs
    coll.reserve(3);            // Reserve mem for 3 elements (this is a heap alloc)
    qDebug("coll address in createAndInsert: %s", acStr(&coll));
    qDebug("&coll[0] = %s, &coll[1] = %s, &coll[2] = %s",
           acStr(&coll[0]), acStr(&coll[1]), acStr(&coll[2]));

    NoMov nm;                   // Create a NoMov

    // All containers in C++ have value semantics, which means they create copies of
    // values passed to them.
    coll.push_back(nm);         // Insert local obj
    coll.push_back(nm+nm);      // Insert anon temp obj -- CC for obj in vec; DTOR for temp
    coll.push_back(nm);         // Insert local named obj -- CC for obj in vec; DTOR for nm

    return coll;
}

// Unused in-param used merely to effect overload
std::vector<Mov> createAndInsert(int32_t dummy_arg_for_ovl)
{
    Q_UNUSED(dummy_arg_for_ovl)
    constexpr int ARRAY_SIZE = 8;

    std::vector<Mov> coll;      // Vector of NoMovs
    coll.reserve(ARRAY_SIZE);   // Reserve mem for vector (this is a heap alloc)

    qDebug("coll address in createAndInsert: %s", acStr(&coll));
    for (int i=0; i<ARRAY_SIZE; i++)  {
        qDebug("&coll[%d] = %s", i, acStr(&coll[i]));
    }

    Mov mv;                 // Create a NoMov

    // All containers in C++ have value semantics, which means they create copies of
    // values passed to them.
    coll.push_back(mv);             // Insert local obj -- CC for obj in vec
    coll.push_back(mv+mv);          // Insert anon temp obj -- op+ CTOR for temp; CC for obj in vec; DTOR for temp
    coll.emplace_back(mv+mv);       // This *also* causes the same -- op+ CTOR for temp; CC for obj in vec; DTOR for temp
    coll.push_back(Mov());          // S-ddd CTOR; MC for H-ddd obj; S-ddd DTOR
    // emplace_back() constructs object in-place in the container, instead of the
    // temp CTOR - MC - temp DTOR dance
    // https://abseil.io/tips/65
    // https://abseil.io/tips/112
    // NOTE: emplace_back only takes the direct ctor arguments, which it fwds to the
    // ctor of the object! Think make_unique. The following does NOT work: coll.emplace_back(Mov())
    // Rather - this results in the same S-CTOR; H-MC; S-DTOR sequence as with push_back.
    // push_back does NOT have this facility; must give it an actual obj - tmp or otherwise.
    coll.emplace_back();            // Just calls a H-ddd ctor!
    coll.push_back(std::move(mv));  // Insert local named obj -- MC for obj in vec

    return coll;
}

void fnParmByVal(Mov m) {
    qDebug("fnByVal: arg m is %s", acStr(&m));
}

void fnParmByRRef(Mov&& m) {
    qDebug("fnByRRef: arg m is %s", acStr(&m));
    // NOTE! Within this function, m is still an lvalue (even though its *type*
    // is rvalue ref) by virtue of the simple fact that it is the fn parameter
    // and thereby has a name.
    // Therefore, in order to invoke the *move* ctor for mloc below, it is
    // still necessary to std::move(m). If you omit that, the COPY ctor
    // gets invoked! See Josuttis Sec 3.2.2: MS is not passed through.
    Mov mloc{std::move(m)};
    // As per usual C++ semantics, MC invoked here, not MAO. This is because
    // a new obj is being created.
//    Mov mloc = std::move(m);
    qDebug("fnByRRef scavenged rvref argument. Now orig obj invalid!");
}


Mov fnRetByVal(int32_t dummy_arg_for_ovl) {
    Q_UNUSED(dummy_arg_for_ovl)   // Parm used only to enable fn overload
    Mov m;
    //return m;
    return std::move(m);  // BAD! Don't do this! QtCreator will also emit warning
}

NoMov fnRetByVal(uint32_t dummy_arg_for_ovl) {
    Q_UNUSED(dummy_arg_for_ovl)
    NoMov nm;
    return nm;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    queryStackHeap();

    qDebug("--------------------------------------------------------------");
    {
        // Non-Movable type
        qDebug("Without Move Semantics:");
        std::vector<NoMov> vnm;
        qDebug("vnm address in main: %s", acStr(&vnm));
        qDebug("sizeof(NoMov) = %llu (=0x%llx) bytes", sizeof(NoMov), sizeof(NoMov));
        vnm = createAndInsert(DUMMY_NM);

        qDebug("--------------------------------------------------------------");

        // Movable type
        qDebug("WITH Move Semantics:");
        std::vector<Mov> vm;
        qDebug("vm address in main: %s", acStr(&vm));
        qDebug("sizeof(NoMov) = %llu (=0x%llx) bytes", sizeof(Mov), sizeof(Mov));
        vm = createAndInsert(DUMMY_M);
    }
    qDebug("--------------------------------------------------------------");
    {
        Mov mbvfc;
        fnParmByVal(mbvfc);     // CC with placement new in the location of the arg on the stack
        fnParmByVal(std::move(mbvfc));  // MC with placement new

        Mov mbrrfc;
        fnParmByRRef(std::move(mbrrfc));
    }
    qDebug("--------------------------------------------------------------");
    {
        NoMov nmret;                        // CTOR for stack obj
        nmret = fnRetByVal(DUMMY_NM);       // CTOR, then CAO, then DTOR
        // NoMov nmret = fnRetByVal(DUMMY_NM);   // Results in single CTOR call!

        //Mov mret;                           // CTOR for stack obj
        //mret = fnRetByVal(DUMMY_M);         // CTOR, then MAO, then DTOR

        // If we std::move(retval) in the function, this results in CTOR+MC+DTOR!
        // Compared with a single CTOR call if we don't std::move.
        Mov mret = fnRetByVal(DUMMY_M);
    }
    qDebug("---------------------for Loop Test----------------------------");
    {
        // Within each loop iteration, a new scope begins and ends. So all
        // vars defined inside the loop body get CTORd and DTORd at the end
        // of that iteration!
        for (int i=0; i<5; i++) {
            qDebug("Iteration %d", i);
            Mov mov1, mov2; // CTORs for local objs
            //TestObjM tomex{mov1, mov2};     // CC into params, MC into members, DTORLs for params
        }   // <-- DTORs for members, DTORs for local objs mov1, mov2.
    }
    qDebug("-----------------Initializing Members-------------------------");
    {
        Mov mov1, mov2;

        TestObjM tom{Mov(), Mov()};

        // The following is pointless/redundant since the std::move is static_casting
        // rvalues into rvalues.
        //TestObjM tom2{std::move(Mov()), std::move(Mov())};

        //TestObjM tomex{mov1, mov2};     // TestObj ctor with existing objects
        //TestObjM tomexmv{std::move(mov1), std::move(mov2)};     // TestObj ctor with existing objects
    }

    qDebug("------------------noexcept + MS on Class Members--------------------");
    {
        // From http://cppmove.com/code/basics/members.cpp.html
        Mov mo; // outside
        std::pair<Mov, Mov> prOwnMembers;            // Both members belong to this obj
        //std::pair<Mov, Mov&> prRefMember{{}, mo};    // One of the members is a ref to an external obj. First mem gets *copied*: CTOR, CC, DTOR
        std::pair<Mov, Mov&> prRefMember{Mov{}, mo};    // This one results in a move
        //std::pair<Mov, Mov&> prRefMember{std::move(Mov{}), mo};    // Also results in a move (although QtC throws an error)
        // ERROR-> std::pair<Mov, Mov&> prRefMember = std::make_pair(Mov{}, mo);    // How to make this work? "no viable ctor" error
        // https://stackoverflow.com/questions/6162201/c11-use-case-for-piecewise-construct-of-pair-and-tuple
        //std::pair<Mov, Mov&> prRefMember(std::piecewise_construct, Mov{}, mo);    // One of the members is a ref to an external obj
        // WB link to std::pair attempts:
        // https://wandbox.org/permlink/xRCyyZnzwpiSTr5X

        // Also step through this with the noexcept removed on the Mov MC/MAO: all the vector-resize
        // operations drop to copies instead of moves.
        std::vector<Mov> coll;
        qDebug() << coll.capacity();    // 0

        coll.push_back(prOwnMembers.first);          // Copies -- remember, containers make copies by default
        qDebug() << coll.capacity();    // 0->1
        coll.push_back(prOwnMembers.second);         // Copies -- remember, containers make copies by default
        qDebug() << coll.capacity();    // 1->2

        coll.push_back(std::move(prOwnMembers).first);   // Now moves; could also do std::move(pOwnMem.first)
        qDebug() << coll.capacity();    // 2->4
        coll.push_back(std::move(prOwnMembers).second);  // Now moves; could also do std::move(pOwnMem.second)
        qDebug() << coll.capacity();    // 4

        coll.push_back(std::move(prRefMember).first);   // Moves
        qDebug() << coll.capacity();    // 4->8
        coll.push_back(std::move(prRefMember).second);  // *** Copies ***
        qDebug() << coll.capacity();    // 8

    }
    qDebug("--------------------------------------------------------------");


    return 0;
}

