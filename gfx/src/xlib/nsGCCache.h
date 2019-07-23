








































#ifndef nsGCCacheXlib_h___
#define nsGCCacheXlib_h___

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "nscore.h"
#include "nsDebug.h"
#include "prclist.h"
#include <stdlib.h>
#include <string.h>

#define countof(x) ((int)(sizeof(x) / sizeof (*x)))
#define GC_CACHE_SIZE 64

#ifdef DEBUG
#define DEBUG_METER(x) x
#else
#define DEBUG_METER(x)
#endif

class nsGCCacheXlib;

class xGC {
  friend class nsGCCacheXlib;

public:
  xGC(Display *display, Drawable d, unsigned long valuemask, XGCValues *values)
  {
    mRefCnt = 0;
    mDisplay = display;
    mGC = ::XCreateGC(display, d, valuemask, values);
  }

  virtual ~xGC() {
    ::XFreeGC(mDisplay, mGC);
  }

  PRInt32 AddRef(void) {
#ifdef DEBUG  
    if(mRefCnt>400) abort();
#endif 
    NS_PRECONDITION(PRInt32(mRefCnt) >= 0, "illegal refcnt");
    ++mRefCnt;
    return mRefCnt;
  }
  PRInt32 Release(void) {
    NS_PRECONDITION(0 != mRefCnt, "dup release");
    --mRefCnt;
    if (mRefCnt == 0) {
      mRefCnt = 1; 
      delete this;
      return 0;
    }
    return mRefCnt;
  }

  
  operator const GC() { return (const GC)mGC; }

private:
  PRInt32 mRefCnt;
  Display *mDisplay;
  GC mGC;
};


struct GCCacheEntryXlib
{
  PRCList clist;
  unsigned long flags;
  XGCValues gcv;
  Region clipRegion;
  xGC *gc;
};

class nsGCCacheXlib
{
 public:
  nsGCCacheXlib();
  virtual ~nsGCCacheXlib();

  void Flush(unsigned long flags);

  xGC *GetGC(Display *display, Window window, unsigned long flags, XGCValues *gcv, Region clipRegion);
  
private:
  void ReuseGC(GCCacheEntryXlib *entry, unsigned long flags, XGCValues *gcv);
  PRCList GCCache;
  PRCList GCFreeList;
  void free_cache_entry(PRCList *clist);
  void move_cache_entry(PRCList *clist);
  static void XCopyRegion(Region src, Region dr_return);
  static Region copyRegion;
  void ReportStats();

  DEBUG_METER(
              struct {
                int hits[GC_CACHE_SIZE];
                int misses;
                int reclaim;
              } GCCacheStats;
              )

};

#endif
