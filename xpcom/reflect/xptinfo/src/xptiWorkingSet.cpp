








































#include "xptiprivate.h"
#include "nsString.h"
#include "nsIMemoryReporter.h"

using namespace mozilla;

static PRInt64 GetXPTArenaSize(void*)
{
  return XPT_SizeOfArena(gXPTIStructArena);
}

NS_MEMORY_REPORTER_IMPLEMENT(xptiWorkingSet,
                             "explicit/xpti-working-set",
                             MR_HEAP,
                             "Memory used by the XPCOM typelib system.",
                             GetXPTArenaSize,
                             nsnull)

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
