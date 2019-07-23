







































#include <gdk/gdk.h>
#include <string.h>
#include "prclist.h"

#ifndef nsGCCache_h___
#define nsGCCache_h___

#define countof(x) ((int)(sizeof(x) / sizeof (*x)))
#define GC_CACHE_SIZE 10

#ifdef DEBUG
#define DEBUG_METER(x) x
#else
#define DEBUG_METER(x)
#endif

struct GCCacheEntry
{
  PRCList clist;
  GdkGCValuesMask flags;
  GdkGCValues gcv;
  GdkRegion *clipRegion;
  GdkGC *gc;
};

class nsGCCache
{
 public:
  nsGCCache();
  virtual ~nsGCCache();

  static void Shutdown();

  void Flush(unsigned long flags);

  GdkGC *GetGC(GdkWindow *window, GdkGCValues *gcv, GdkGCValuesMask flags, GdkRegion *clipRegion);
  
private:
  void ReuseGC(GCCacheEntry *entry, GdkGCValues *gcv, GdkGCValuesMask flags);
  PRCList GCCache;
  PRCList GCFreeList;
  void free_cache_entry(PRCList *clist);
  void move_cache_entry(PRCList *clist);
  static GdkRegion *copyRegion;
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
