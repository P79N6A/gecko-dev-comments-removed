








































#include <stdio.h>
#include "nsGCCache.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>




Region nsGCCacheXlib::copyRegion = 0;

nsGCCacheXlib::nsGCCacheXlib()
{
  PR_INIT_CLIST(&GCCache);
  PR_INIT_CLIST(&GCFreeList);
  for (int i = 0; i < GC_CACHE_SIZE; i++) {
    GCCacheEntryXlib *entry = new GCCacheEntryXlib();
    entry->gc=NULL;
    PR_INSERT_LINK(&entry->clist, &GCFreeList);
  }
  DEBUG_METER(memset(&GCCacheStats, 0, sizeof(GCCacheStats));)
}

void
nsGCCacheXlib::move_cache_entry(PRCList *clist)
{
  
  PR_REMOVE_LINK(clist);
  PR_INSERT_LINK(clist, &GCFreeList);
}

void
nsGCCacheXlib::free_cache_entry(PRCList *clist)
{
  GCCacheEntryXlib *entry = (GCCacheEntryXlib *)clist;
  entry->gc->Release();
  if (entry->clipRegion)
    ::XDestroyRegion(entry->clipRegion);
  
  
  PR_REMOVE_LINK(clist);
  memset(entry, 0, sizeof(*entry));
  PR_INSERT_LINK(clist, &GCFreeList);
}

nsGCCacheXlib::~nsGCCacheXlib()
{
  PRCList *head;

  ReportStats();

  while (!PR_CLIST_IS_EMPTY(&GCCache)) {
    head = PR_LIST_HEAD(&GCCache);
    if (head == &GCCache)
      break;
    free_cache_entry(head);
  }

  while (!PR_CLIST_IS_EMPTY(&GCFreeList)) {
    head = PR_LIST_HEAD(&GCFreeList);
    if (head == &GCFreeList)
      break;
    PR_REMOVE_LINK(head);
    delete (GCCacheEntryXlib *)head;
  }
}

void
nsGCCacheXlib::ReportStats() { 
  DEBUG_METER(
              fprintf(stderr, "GC Cache:\n\thits:");
              int hits = 0;
              for (int i = 0; i < GC_CACHE_SIZE; i++) {
                fprintf(stderr, " %4d", GCCacheStats.hits[i]);
                hits+=GCCacheStats.hits[i];
              }
              int total = hits + GCCacheStats.misses;
              float percent = float(float(hits) / float(total));
              percent *= 100;
              fprintf(stderr, "\n\thits: %d, misses: %d, hit percent: %f%%\n", 
                      hits, GCCacheStats.misses, percent);
              );
}


void
nsGCCacheXlib::XCopyRegion(Region srca, Region dr_return)
{
  if (!copyRegion) copyRegion = ::XCreateRegion();
  ::XUnionRegion(srca, copyRegion, dr_return);
}


void nsGCCacheXlib::Flush(unsigned long flags)
{
  while (!PR_CLIST_IS_EMPTY(&GCCache)) {
    PRCList *head = PR_LIST_HEAD(&GCCache);
    if (head == &GCCache)
      break;
    GCCacheEntryXlib *entry = (GCCacheEntryXlib *)head;
    if (entry->flags & flags)
      free_cache_entry(head);
  }
}

xGC *nsGCCacheXlib::GetGC(Display *display, Drawable drawable, unsigned long flags, XGCValues *gcv, Region clipRegion)
{
  PRCList *iter;
  GCCacheEntryXlib *entry;
  DEBUG_METER(int i = 0;)
  
  for (iter = PR_LIST_HEAD(&GCCache); iter != &GCCache;
       iter = PR_NEXT_LINK(iter)) {

    entry = (GCCacheEntryXlib *)iter;
    if (flags == entry->flags && 
        !memcmp (gcv, &entry->gcv, sizeof (*gcv))) {
      

      if ((clipRegion && entry->clipRegion &&
           ::XEqualRegion(clipRegion, entry->clipRegion)) ||
          
          (!clipRegion && !entry->clipRegion)) {

        
        if (iter != PR_LIST_HEAD(&GCCache)) {
          PR_REMOVE_LINK(iter);
          PR_INSERT_LINK(iter, &GCCache);
        }
        DEBUG_METER(GCCacheStats.hits[i]++;)

        entry->gc->AddRef();
        return entry->gc;
      }
    }
    DEBUG_METER(++i;)
  }
    
  
  if (PR_CLIST_IS_EMPTY(&GCFreeList)) {
    DEBUG_METER(GCCacheStats.reclaim++);
    move_cache_entry(PR_LIST_TAIL(&GCCache));
  }

  DEBUG_METER(GCCacheStats.misses++;)
  
  iter = PR_LIST_HEAD(&GCFreeList);
  PR_REMOVE_LINK(iter);
  PR_INSERT_LINK(iter, &GCCache);
  entry = (GCCacheEntryXlib *)iter;

  if (!entry->gc) {
    
    entry->gc = new xGC(display, drawable, flags, gcv);
    entry->gc->AddRef(); 
    entry->flags = flags;
    entry->gcv = *gcv;
    entry->clipRegion = NULL;
    
  }
  else if (entry->gc->mRefCnt > 0) {
    
    entry->gc->Release();
    entry->gc = new xGC(display, drawable, flags, gcv);
    entry->gc->AddRef(); 
    entry->flags = flags;
    entry->gcv = *gcv;
    if (entry->clipRegion)
	XDestroyRegion(entry->clipRegion);
    entry->clipRegion = NULL;
    
  }
  else {
    ReuseGC(entry, flags, gcv);
  }

  if (clipRegion) {
    entry->clipRegion = ::XCreateRegion();
    XCopyRegion(clipRegion, entry->clipRegion);
    if (entry->clipRegion)
      ::XSetRegion(display, entry->gc->mGC, entry->clipRegion);
    
  }
  
  entry->gc->AddRef();
  return entry->gc;
}

void nsGCCacheXlib::ReuseGC(GCCacheEntryXlib *entry, unsigned long flags, XGCValues *gcv)
{
  
  

  if (entry->clipRegion) {
    
    
    gcv->clip_mask = None;
    flags |= GCClipMask;
    ::XDestroyRegion(entry->clipRegion);
    entry->clipRegion = NULL;
  }

  if (flags != 0) {
    ::XChangeGC(entry->gc->mDisplay, entry->gc->mGC,
                flags, gcv);
  }
  entry->flags = flags;  entry->gcv = *gcv;
}
