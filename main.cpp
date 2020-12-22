#include <QCoreApplication>

#include <vector>

#include <malloc.h>
#include <intsafe.h>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "nomov.h"
#include "mov.h"
#include "ro5.h"
#include "AddrClassifier.h"

// Define storage for AddrClassifier members:
uintptr_t AddrClassifier::stackTop;
uintptr_t AddrClassifier::stackBot;

// For overloading the createAndInsert function with dummy arg differences
// (Remember that you cannot overload a function by ret type)
// https://stackoverflow.com/a/34095060/3367247
constexpr uint32_t  NM  = 'N';
constexpr int32_t   M   = 'M';

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
std::vector<NoMov> createAndInsert(uint32_t nmv)
{
    Q_UNUSED(nmv)
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
std::vector<Mov> createAndInsert(int32_t m)
{
    Q_UNUSED(m)
    std::vector<Mov> coll;    // Vector of NoMovs
    coll.reserve(3);            // Reserve mem for 3 elements (this is a heap alloc)
    qDebug("coll address in createAndInsert: %s", acStr(&coll));
    qDebug("&coll[0] = %s, &coll[1] = %s, &coll[2] = %s",
           acStr(&coll[0]), acStr(&coll[1]), acStr(&coll[2]));

    Mov mv;                   // Create a NoMov

    // All containers in C++ have value semantics, which means they create copies of
    // values passed to them.
    coll.push_back(mv);          // Insert local obj
    coll.push_back(mv+mv);        // Insert anon temp obj -- CC for obj in vec; DTOR for temp
    coll.push_back(std::move(mv));         // Insert local named obj -- MC for obj in vec; DTOR for mv

    return coll;
}

void fnByVal(Mov m) {
    qDebug("fnByVal: arg m is %s", acStr(&m));
}

void fnByRRef(Mov&& m) {
    qDebug("fnByRRef: arg m is %s", acStr(&m));
    // NOTE! Within this function, m is still an lvalue (even though its *type*
    // is rvalue ref) by virtue of the simple fact that it is the fn parameter
    // and thereby has a name.
    // Therefore, in order to invoke the *move* ctor for mloc below, it is
    // still necessary to std::move(m). If you omit that, the COPY ctor
    // gets invoked!
    Mov mloc{std::move(m)};
    qDebug("fnByRRef scavenged rvref argument. Now orig obj invalid!");
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    queryStackHeap();

    qDebug("--------------------------------------------------------------");

    // Non-Movable type
    qDebug("Without Move Semantics:");
    std::vector<NoMov> vnm;
    qDebug("vnm address in main: %s", acStr(&vnm));
    qDebug("sizeof(NoMov) = %llu (=0x%llx) bytes", sizeof(NoMov), sizeof(NoMov));
    vnm = createAndInsert(NM);

    qDebug("--------------------------------------------------------------");

    // Movable type
    qDebug("WITH Move Semantics:");
    std::vector<Mov> vm;
    qDebug("vm address in main: %s", acStr(&vm));
    qDebug("sizeof(NoMov) = %llu (=0x%llx) bytes", sizeof(Mov), sizeof(Mov));
    vm = createAndInsert(M);

    qDebug("--------------------------------------------------------------");

    Mov mbvfc;
    fnByVal(mbvfc);     // CC with placement new in the location of the arg on the stack
    fnByVal(std::move(mbvfc));  // MC with placement new

    Mov mbrrfc;
    fnByRRef(std::move(mbrrfc));

    qDebug("--------------------------DTORS-------------------------------");
    // DTORs for all local objects after this

    return 0;
}

