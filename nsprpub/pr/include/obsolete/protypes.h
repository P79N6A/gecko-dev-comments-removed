











































#if !defined(PROTYPES_H)
#define PROTYPES_H

typedef PRUintn uintn;
#ifndef _XP_Core_
typedef PRIntn intn;
#endif














#ifdef XP_BEOS
#include <support/SupportDefs.h>
#endif





#ifdef VMS
#include <ints.h>
#include <types.h>
#endif







#ifdef XP_UNIX
#include <sys/types.h>
#endif


#ifdef HPUX
#include <model.h>
#endif





#if !defined(XP_BEOS) && !defined(VMS) \
    && !defined(XP_UNIX) || defined(NTO)
typedef PRUintn uint;
#endif





#if !defined(XP_BEOS) && !defined(VMS)
typedef PRUint64 uint64;
#endif





#if !defined(XP_BEOS) && !defined(VMS)
#if !defined(XP_MAC) && !defined(_WIN32) && !defined(XP_OS2) && !defined(NTO)
typedef PRUint32 uint32;
#else
typedef unsigned long uint32;
#endif
#endif





#if !defined(XP_BEOS) && !defined(VMS)
typedef PRUint16 uint16;
#endif





#if !defined(XP_BEOS) && !defined(VMS)
typedef PRUint8 uint8;
#endif





#if !defined(XP_BEOS) && !defined(VMS) \
    && !defined(_PR_AIX_HAVE_BSD_INT_TYPES)
typedef PRInt64 int64;
#endif





#if !defined(XP_BEOS) && !defined(VMS) \
    && !defined(_PR_AIX_HAVE_BSD_INT_TYPES) \
    && !defined(HPUX)
#if !defined(XP_MAC) && !defined(_WIN32) && !defined(XP_OS2) && !defined(NTO)
typedef PRInt32 int32;
#else
typedef long int32;
#endif
#endif





#if !defined(XP_BEOS) && !defined(VMS) \
    && !defined(_PR_AIX_HAVE_BSD_INT_TYPES) \
    && !defined(HPUX)
typedef PRInt16 int16;
#endif





#if !defined(XP_BEOS) && !defined(VMS) \
    && !defined(_PR_AIX_HAVE_BSD_INT_TYPES) \
    && !defined(HPUX)
typedef PRInt8 int8;
#endif

typedef PRFloat64 float64;
typedef PRUptrdiff uptrdiff_t;
typedef PRUword uprword_t;
typedef PRWord prword_t;



#define TEST_BIT	PR_TEST_BIT
#define SET_BIT		PR_SET_BIT
#define CLEAR_BIT	PR_CLEAR_BIT


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

#if defined(XP_MAC)
#ifndef TRUE				
	#define TRUE 1
#endif
#ifndef FALSE				
	#define FALSE 0
#endif
#endif

#endif 
