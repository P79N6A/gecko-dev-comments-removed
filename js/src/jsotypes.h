

















































#ifndef PROTYPES_H
#define PROTYPES_H

#ifdef XP_BEOS





#include <SupportDefs.h>

typedef JSUintn uintn;
#ifndef _XP_Core_
typedef JSIntn intn;
#endif

#else


#ifdef XP_UNIX
#include <sys/types.h>
#else
typedef JSUintn uint;
#endif

typedef JSUintn uintn;
typedef JSUint64 uint64;
#if !defined(_WIN32) && !defined(XP_OS2)
typedef JSUint32 uint32;
#else
typedef unsigned long uint32;
#endif
typedef JSUint16 uint16;
typedef JSUint8 uint8;

#ifndef _XP_Core_
typedef JSIntn intn;
#endif







#if defined(AIX) && defined(HAVE_SYS_INTTYPES_H)
#include <sys/inttypes.h>
#else
typedef JSInt64 int64;


#if !defined(_WIN32) && !defined(XP_OS2)
typedef JSInt32 int32;
#else
typedef long int32;
#endif
typedef JSInt16 int16;
typedef JSInt8 int8;
#endif 

#endif  

typedef JSFloat64 float64;


#define TEST_BIT        JS_TEST_BIT
#define SET_BIT         JS_SET_BIT
#define CLEAR_BIT       JS_CLEAR_BIT


#define PRArena PLArena
#define PRArenaPool PLArenaPool
#define PRArenaStats PLArenaStats
#define PR_ARENA_ALIGN PL_ARENA_ALIGN
#define PR_INIT_ARENA_POOL PL_INIT_ARENA_POOL
#define PR_ARENA_ALLOCATE PL_ARENA_ALLOCATE
#define PR_ARENA_GROW PL_ARENA_GROW
#define PR_ARENA_MARK PL_ARENA_MARK
#define PR_CLEAR_UNUSED PL_CLEAR_UNUSED
#define PR_CLEAR_ARENA PL_CLEAR_ARENA
#define PR_ARENA_RELEASE PL_ARENA_RELEASE
#define PR_COUNT_ARENA PL_COUNT_ARENA
#define PR_ARENA_DESTROY PL_ARENA_DESTROY
#define PR_InitArenaPool PL_InitArenaPool
#define PR_FreeArenaPool PL_FreeArenaPool
#define PR_FinishArenaPool PL_FinishArenaPool
#define PR_CompactArenaPool PL_CompactArenaPool
#define PR_ArenaFinish PL_ArenaFinish
#define PR_ArenaAllocate PL_ArenaAllocate
#define PR_ArenaGrow PL_ArenaGrow
#define PR_ArenaRelease PL_ArenaRelease
#define PR_ArenaCountAllocation PL_ArenaCountAllocation
#define PR_ArenaCountInplaceGrowth PL_ArenaCountInplaceGrowth
#define PR_ArenaCountGrowth PL_ArenaCountGrowth
#define PR_ArenaCountRelease PL_ArenaCountRelease
#define PR_ArenaCountRetract PL_ArenaCountRetract


#define PREvent PLEvent
#define PREventQueue PLEventQueue
#define PR_CreateEventQueue PL_CreateEventQueue
#define PR_DestroyEventQueue PL_DestroyEventQueue
#define PR_GetEventQueueMonitor PL_GetEventQueueMonitor
#define PR_ENTER_EVENT_QUEUE_MONITOR PL_ENTER_EVENT_QUEUE_MONITOR
#define PR_EXIT_EVENT_QUEUE_MONITOR PL_EXIT_EVENT_QUEUE_MONITOR
#define PR_PostEvent PL_PostEvent
#define PR_PostSynchronousEvent PL_PostSynchronousEvent
#define PR_GetEvent PL_GetEvent
#define PR_EventAvailable PL_EventAvailable
#define PREventFunProc PLEventFunProc
#define PR_MapEvents PL_MapEvents
#define PR_RevokeEvents PL_RevokeEvents
#define PR_ProcessPendingEvents PL_ProcessPendingEvents
#define PR_WaitForEvent PL_WaitForEvent
#define PR_EventLoop PL_EventLoop
#define PR_GetEventQueueSelectFD PL_GetEventQueueSelectFD
#define PRHandleEventProc PLHandleEventProc
#define PRDestroyEventProc PLDestroyEventProc
#define PR_InitEvent PL_InitEvent
#define PR_GetEventOwner PL_GetEventOwner
#define PR_HandleEvent PL_HandleEvent
#define PR_DestroyEvent PL_DestroyEvent
#define PR_DequeueEvent PL_DequeueEvent
#define PR_GetMainEventQueue PL_GetMainEventQueue


#define PRHashEntry PLHashEntry
#define PRHashTable PLHashTable
#define PRHashNumber PLHashNumber
#define PRHashFunction PLHashFunction
#define PRHashComparator PLHashComparator
#define PRHashEnumerator PLHashEnumerator
#define PRHashAllocOps PLHashAllocOps
#define PR_NewHashTable PL_NewHashTable
#define PR_HashTableDestroy PL_HashTableDestroy
#define PR_HashTableRawLookup PL_HashTableRawLookup
#define PR_HashTableRawAdd PL_HashTableRawAdd
#define PR_HashTableRawRemove PL_HashTableRawRemove
#define PR_HashTableAdd PL_HashTableAdd
#define PR_HashTableRemove PL_HashTableRemove
#define PR_HashTableEnumerateEntries PL_HashTableEnumerateEntries
#define PR_HashTableLookup PL_HashTableLookup
#define PR_HashTableDump PL_HashTableDump
#define PR_HashString PL_HashString
#define PR_CompareStrings PL_CompareStrings
#define PR_CompareValues PL_CompareValues

#endif 
