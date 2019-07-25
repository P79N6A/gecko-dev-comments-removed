






#include "xptiprivate.h"
#include "nsString.h"
#include "nsIMemoryReporter.h"

using namespace mozilla;

NS_MEMORY_REPORTER_MALLOC_SIZEOF_FUN(XPTMallocSizeOf, "xpti-working-set")

static PRInt64 GetXPTArenaSize()
{
  return XPT_SizeOfArena(gXPTIStructArena, XPTMallocSizeOf);
}

NS_MEMORY_REPORTER_IMPLEMENT(xptiWorkingSet,
                             "explicit/xpti-working-set",
                             KIND_HEAP,
                             UNITS_BYTES,
                             GetXPTArenaSize,
                             "Memory used by the XPCOM typelib system.")

#define XPTI_STRUCT_ARENA_BLOCK_SIZE    (1024 * 1)
#define XPTI_HASHTABLE_SIZE             2048

xptiWorkingSet::xptiWorkingSet()
    : mTableReentrantMonitor("xptiWorkingSet::mTableReentrantMonitor")
{
    MOZ_COUNT_CTOR(xptiWorkingSet);

    mIIDTable.Init(XPTI_HASHTABLE_SIZE);
    mNameTable.Init(XPTI_HASHTABLE_SIZE);

    gXPTIStructArena = XPT_NewArena(XPTI_STRUCT_ARENA_BLOCK_SIZE, sizeof(double),
                                    "xptiWorkingSet structs");

    NS_RegisterMemoryReporter(new NS_MEMORY_REPORTER_NAME(xptiWorkingSet));
}        

static PLDHashOperator
xpti_Invalidator(const char* keyname, xptiInterfaceEntry* entry, void* arg)
{
    entry->LockedInvalidateInterfaceInfo();
    return PL_DHASH_NEXT;
}

void 
xptiWorkingSet::InvalidateInterfaceInfos()
{
    ReentrantMonitorAutoEnter monitor(mTableReentrantMonitor);
    mNameTable.EnumerateRead(xpti_Invalidator, NULL);
}        

xptiWorkingSet::~xptiWorkingSet()
{
    MOZ_COUNT_DTOR(xptiWorkingSet);

    
    
#ifdef NS_FREE_PERMANENT_DATA
    XPT_DestroyArena(gXPTIStructArena);
#endif
}

XPTArena* gXPTIStructArena;
