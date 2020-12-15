#include <QCoreApplication>

#ifdef Q_OS_WIN
#include <windows.h>
#include <malloc.h>
#include <intsafe.h>
#endif

#include "nomov.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

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

    SIZE_T stackObject;
    SIZE_T stackBot = (((SIZE_T) &stackObject) & ~0xfff);  // For a 4kB stack frame
    SIZE_T stackTop = stackBot + 0xfff;

    qDebug("Stack: 0x%llx - 0x%llx (%p)", stackTop, stackBot, &stackObject);

#endif

#ifdef Q_OS_LINUX
    // Read from /proc/self/maps, parse out stack and heap


#endif

    int i = 10;
    qDebug("Stack var i addr in main() = %p", &i);

    int *ptr = new int[64];
    qDebug("int array malloc addr in main() = %p", ptr);
    delete [] ptr;

    NoMov* pnm = new NoMov();
    qDebug("NoMov alloc addr in main() = %p", pnm);
    delete pnm;

    return 0;
}
