



#ifndef nsAtomicRefcnt_h__
#define nsAtomicRefcnt_h__

#include "nscore.h"
#include "pratom.h"

class nsAutoRefCnt;




#if defined(XP_WIN)

#if PR_BYTES_PER_LONG == 4
typedef volatile long nsAtomicRefcnt;
#else
#error "Windows should have 4 bytes per long."
#endif

#else 

typedef int32_t  nsAtomicRefcnt;

#endif

inline int32_t
NS_AtomicIncrementRefcnt(int32_t &refcnt)
{
  return PR_ATOMIC_INCREMENT(&refcnt);
}

inline nsrefcnt
NS_AtomicIncrementRefcnt(nsrefcnt &refcnt)
{
  return (nsrefcnt) PR_ATOMIC_INCREMENT((nsAtomicRefcnt*)&refcnt);
}

inline nsrefcnt
NS_AtomicIncrementRefcnt(nsAutoRefCnt &refcnt)
{
  
  return (nsrefcnt) PR_ATOMIC_INCREMENT((nsAtomicRefcnt*)&refcnt);
}

inline nsrefcnt
NS_AtomicDecrementRefcnt(nsrefcnt &refcnt)
{
  return (nsrefcnt) PR_ATOMIC_DECREMENT((nsAtomicRefcnt*)&refcnt);
}

inline nsrefcnt
NS_AtomicDecrementRefcnt(nsAutoRefCnt &refcnt)
{
  return (nsrefcnt) PR_ATOMIC_DECREMENT((nsAtomicRefcnt*)&refcnt);
}

inline int32_t
NS_AtomicDecrementRefcnt(int32_t &refcnt)
{
  return PR_ATOMIC_DECREMENT(&refcnt);
}

#endif
