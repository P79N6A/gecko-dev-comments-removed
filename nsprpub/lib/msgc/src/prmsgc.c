




































#include <string.h>
#include <stddef.h>
#include <stdarg.h>
#include <time.h>

#ifdef WIN32
#include <windef.h>
#include <winbase.h>
#endif

#include "prclist.h"
#include "prbit.h"

#include "prtypes.h"
#include "prenv.h"
#include "prgc.h"
#include "prthread.h"
#include "prlog.h"
#include "prlong.h"
#include "prinrval.h"
#include "prprf.h"
#include "gcint.h"

#if defined(XP_MAC)
#include "pprthred.h"
#else
#include "private/pprthred.h"
#endif

typedef void (*PRFileDumper)(FILE *out, PRBool detailed);

PR_EXTERN(void)
PR_DumpToFile(char* filename, char* msg, PRFileDumper dump, PRBool detailed);







PRLogModuleInfo *_pr_msgc_lm;
PRLogModuleInfo* GC;

static PRInt32 _pr_pageShift;
static PRInt32 _pr_pageSize;

#ifdef DEBUG
#define GCMETER
#endif
#ifdef DEBUG_jwz
# undef GCMETER
#endif 

#ifdef GCMETER
#define METER(x) x
#else
#define METER(x)
#endif





#define MAX_SCAN_Q    100L

#if defined(XP_PC) && !defined(WIN32)
#define MAX_SEGS            400L
#define MAX_SEGMENT_SIZE    (65536L - 4096L)
#define SEGMENT_SIZE        (65536L - 4096L)
#define MAX_ALLOC_SIZE      (65536L - 4096L)
#else
#define MAX_SEGS                    400L
#define MAX_SEGMENT_SIZE            (2L * 256L * 1024L)
#define SEGMENT_SIZE                (1L * 256L * 1024L)
#define MAX_ALLOC_SIZE              (4L * 1024L * 1024L)
#endif  





 
#define MAX_INT ((1UL << (PR_BITS_PER_INT - 1)) - 1)







#define MAX_ALLOC ( (1L << (PR_BYTES_PER_WORD_LOG2 + WORDS_BITS )) -1)




#ifdef XP_MAC
#define MIN_FREE_THRESHOLD_AFTER_GC 10L
#else
#define MIN_FREE_THRESHOLD_AFTER_GC 20L
#endif

static PRInt32 segmentSize = SEGMENT_SIZE;

static PRInt32 collectorCleanupNeeded;

#ifdef GCMETER
PRUint32 _pr_gcMeter;

#define _GC_METER_STATS         0x01L
#define _GC_METER_GROWTH        0x02L
#define _GC_METER_FREE_LIST     0x04L
#endif



#define LINEAR_BIN_EXPONENT 5
#define NUM_LINEAR_BINS ((PRUint32)1 << LINEAR_BIN_EXPONENT)
#define FIRST_LOG_BIN (NUM_LINEAR_BINS - LINEAR_BIN_EXPONENT)



#define NUM_BINS        (FIRST_LOG_BIN + 32)





#define InlineBinNumber(_bin,_bytes)  \
{  \
    PRUint32 _t, _n = (PRUint32) _bytes / 4;  \
    if (_n < NUM_LINEAR_BINS) {  \
        _bin = _n;  \
    } else {  \
        _bin = FIRST_LOG_BIN;  \
        if ((_t = (_n >> 16)) != 0) { _bin += 16; _n = _t; }  \
        if ((_t = (_n >> 8)) != 0)  { _bin += 8; _n = _t; }  \
        if ((_t = (_n >> 4)) != 0)  { _bin += 4; _n = _t; }  \
        if ((_t = (_n >> 2)) != 0) { _bin += 2; _n = _t; }  \
        if ((_n >> 1) != 0) _bin++;  \
    }  \
}

#define BIG_ALLOC       16384L

#define MIN_FREE_CHUNK_BYTES    ((PRInt32)sizeof(GCFreeChunk))



typedef struct GCFreeChunk {
    struct GCFreeChunk *next;
    struct GCSeg *segment;
    PRInt32 chunkSize;
} GCFreeChunk;

typedef struct GCSegInfo {
    struct GCSegInfo *next;
    char *base;
    char *limit;
    PRWord *hbits;
    int fromMalloc;
} GCSegInfo;
    
typedef struct GCSeg {
    char *base;
    char *limit;
    PRWord *hbits;
    GCSegInfo *info;
} GCSeg;

#ifdef GCMETER
typedef struct GCMeter {
    PRInt32 allocBytes;
    PRInt32 wastedBytes;
    PRInt32 numFreeChunks;
    PRInt32 skippedFreeChunks;
} GCMeter;
static GCMeter meter;
#endif




static GCSeg segs[MAX_SEGS];
static GCSegInfo *freeSegs;
static GCSeg* lastInHeap;
static int nsegs;

static GCFreeChunk *bins[NUM_BINS];
static PRInt32 minBin;
static PRInt32 maxBin;





typedef struct GCScanQStr {
    PRWord *q[MAX_SCAN_Q];
    int queued;
} GCScanQ;

static GCScanQ *pScanQ;

#ifdef GCMETER
PRInt32 _pr_maxScanDepth;
PRInt32 _pr_scanDepth;
#endif







#define BIG_ALLOC_GC_SIZE       (4*SEGMENT_SIZE)
static PRWord bigAllocBytes = 0;






#define TYPEIX_BITS    8L
#define WORDS_BITS    20L
#define MAX_CBS        (1L << GC_TYPEIX_BITS)
#define MAX_WORDS    (1L << GC_WORDS_BITS)
#define TYPEIX_SHIFT    24L
#define MAX_TYPEIX    ((1L << TYPEIX_BITS) - 1L)
#define TYPEIX_MASK    PR_BITMASK(TYPEIX_BITS)
#define WORDS_SHIFT    2L
#define WORDS_MASK    PR_BITMASK(WORDS_BITS)
#define MARK_BIT    1L
#define FINAL_BIT    2L



#define GC_USER_BITS_SHIFT 22L
#define GC_USER_BITS    0x00c00000L

#define MAKE_HEADER(_cbix,_words)              \
    ((PRWord) (((unsigned long)(_cbix) << TYPEIX_SHIFT) \
         | ((unsigned long)(_words) << WORDS_SHIFT)))

#define GET_TYPEIX(_h) \
    (((PRUword)(_h) >> TYPEIX_SHIFT) & 0xff)

#define MARK(_sp,_p) \
    (((PRWord *)(_p))[0] |= MARK_BIT)
#define IS_MARKED(_sp,_p) \
    (((PRWord *)(_p))[0] & MARK_BIT)
#define OBJ_BYTES(_h) \
    (((PRInt32) (_h) & 0x003ffffcL) << (PR_BYTES_PER_WORD_LOG2-2L))

#define GC_GET_USER_BITS(_h) (((_h) & GC_USER_BITS) >> GC_USER_BITS_SHIFT)









#define SET_HBIT(_sp,_ph) \
    SET_BIT((_sp)->hbits, (((PRWord*)(_ph)) - ((PRWord*) (_sp)->base)))

#define CLEAR_HBIT(_sp,_ph) \
    CLEAR_BIT((_sp)->hbits, (((PRWord*)(_ph)) - ((PRWord*) (_sp)->base)))

#define IS_HBIT(_sp,_ph) \
    TEST_BIT((_sp)->hbits, (((PRWord*)(_ph)) - ((PRWord*) (_sp)->base)))








static PRWord *FindObject(GCSeg *sp, PRWord *p)
{
    PRWord *base;
    
    
    p = (PRWord*) ((PRWord)p & ~(PR_BYTES_PER_WORD-1L));

    base = (PRWord *) sp->base;
    do {
    if (IS_HBIT(sp, p)) {
        return (p);
    }
    p--;
    } while ( p >= base );

    
    _GCTRACE(GC_TRACE, ("ERROR: The heap is corrupted!!! aborting now!"));
    abort();
    return NULL;
}


#if !defined(XP_PC) || defined(XP_OS2)
#define OutputDebugString(msg)
#endif 

#define IN_SEGMENT(_sp, _p)             \
    ((((char *)(_p)) >= (_sp)->base) &&    \
     (((char *)(_p)) < (_sp)->limit))

static GCSeg *InHeap(void *p)
{
    GCSeg *sp, *esp;

    if (lastInHeap && IN_SEGMENT(lastInHeap, p)) {
    return lastInHeap;
    }

    sp = segs;
    esp = segs + nsegs;
    for (; sp < esp; sp++) {
    if (IN_SEGMENT(sp, p)) {
        lastInHeap = sp;
        return sp;
    }
    }
    return 0;
}





static GCSeg* DoGrowHeap(PRInt32 requestedSize, PRBool exactly)
{
    GCSeg *sp;
    GCSegInfo *segInfo;
    GCFreeChunk *cp;
    char *base;
    PRWord *hbits;
    PRInt32 nhbytes, nhbits;
    PRUint32 allocSize;

    if (nsegs == MAX_SEGS) {
    
    return 0;
    }

    segInfo = (GCSegInfo*) PR_MALLOC(sizeof(GCSegInfo));
#ifdef DEBUG
    {
    char str[256];
    sprintf(str, "[1] Allocated %ld bytes at %p\n",
        (long) sizeof(GCSegInfo), segInfo);
    OutputDebugString(str);
    }
#endif
    if (!segInfo) {
    return 0;
    }

    
    if (exactly) {
    allocSize = requestedSize;
    base = (char *) PR_MALLOC(requestedSize);
    } else {
    allocSize = requestedSize;
    allocSize = (allocSize + _pr_pageSize - 1L) >> _pr_pageShift;
    allocSize <<= _pr_pageShift;
    base = (char*)_MD_GrowGCHeap(&allocSize);
    }
    if (!base) {
    PR_DELETE(segInfo);
    return 0;
    }

    nhbits = (PRInt32)(
        (allocSize + PR_BYTES_PER_WORD - 1L) >> PR_BYTES_PER_WORD_LOG2);
    nhbytes = ((nhbits + PR_BITS_PER_WORD - 1L) >> PR_BITS_PER_WORD_LOG2)
    * sizeof(PRWord);

    
    hbits = (PRWord *) PR_CALLOC((PRUint32)nhbytes);
    if (!hbits) {
    
    PR_DELETE(segInfo);
    if (exactly) {
        PR_DELETE(base);
    } else {
      
      
    }
    return 0;
    }

    


    sp = &segs[nsegs++];
    segInfo->base = sp->base = base;
    segInfo->limit = sp->limit = base + allocSize;
    segInfo->hbits = sp->hbits = hbits;
    sp->info = segInfo;
    segInfo->fromMalloc = exactly;
    memset(base, 0, allocSize);

#ifdef GCMETER
    if (_pr_gcMeter & _GC_METER_GROWTH) {
        fprintf(stderr, "[GC: new segment base=%p size=%ld]\n",
                sp->base, (long) allocSize);
    }
#endif    

    _pr_gcData.allocMemory += allocSize;
    _pr_gcData.freeMemory  += allocSize;

    if (!exactly) {
    PRInt32 bin;

        
        cp = (GCFreeChunk *) base;
        cp->segment = sp;
        cp->chunkSize = allocSize;
        InlineBinNumber(bin, allocSize)
        cp->next = bins[bin];
        bins[bin] = cp;
    if (bin < minBin) minBin = bin;
    if (bin > maxBin) maxBin = bin;
    } else {
        



    }

    if (!_pr_gcData.lowSeg) {
    _pr_gcData.lowSeg  = (PRWord*) sp->base;
    _pr_gcData.highSeg = (PRWord*) sp->limit;
    } else {
    if ((PRWord*)sp->base < _pr_gcData.lowSeg) {
        _pr_gcData.lowSeg = (PRWord*) sp->base;
    }
    if ((PRWord*)sp->limit > _pr_gcData.highSeg) {
        _pr_gcData.highSeg = (PRWord*) sp->limit;
    }
    }

    



 
    memset(&base, 0, sizeof(base));  

    PR_LOG(_pr_msgc_lm, PR_LOG_WARNING, ("grow heap: total gc memory now %d",
                      _pr_gcData.allocMemory));

    return sp;
}

#ifdef USE_EXTEND_HEAP
static PRBool ExtendHeap(PRInt32 requestedSize) {
  GCSeg* sp;
  PRUint32 allocSize;
  PRInt32 oldSize, newSize;
  PRInt32 newHBits, newHBytes;
  PRInt32 oldHBits, oldHBytes;
  PRWord* hbits;
  GCFreeChunk* cp;
  PRInt32 bin;

  
  if (nsegs == 0) return PR_FALSE;

  
  allocSize = (PRUint32) requestedSize;
  allocSize = (allocSize + _pr_pageSize - 1L) >> _pr_pageShift;
  allocSize <<= _pr_pageShift;

  
  sp = segs;
  oldSize = sp->limit - sp->base;
  newSize = oldSize + allocSize;
  newHBits = (newSize + PR_BYTES_PER_WORD - 1L) >> PR_BYTES_PER_WORD_LOG2;
  newHBytes = ((newHBits + PR_BITS_PER_WORD - 1L) >> PR_BITS_PER_WORD_LOG2)
    * sizeof(PRWord);
  hbits = (PRWord*) PR_MALLOC(newHBytes);
  if (0 == hbits) return PR_FALSE;

  
  if (_MD_ExtendGCHeap(sp->base, oldSize, newSize)) {
    oldHBits = (oldSize + PR_BYTES_PER_WORD - 1L) >> PR_BYTES_PER_WORD_LOG2;
    oldHBytes = ((oldHBits + PR_BITS_PER_WORD - 1L) >> PR_BITS_PER_WORD_LOG2)
      * sizeof(PRWord);

    
    memset(hbits, 0, newHBytes);
    memcpy(hbits, sp->hbits, oldHBytes);
    PR_DELETE(sp->hbits);
    memset(sp->base + oldSize, 0, allocSize);

    
    sp->limit += allocSize;
    sp->hbits = hbits;
    sp->info->limit = sp->limit;
    sp->info->hbits = hbits;

    
    cp = (GCFreeChunk *) (sp->base + oldSize);
    cp->segment = sp;
    cp->chunkSize = allocSize;
    InlineBinNumber(bin, allocSize)
    cp->next = bins[bin];
    bins[bin] = cp;
    if (bin < minBin) minBin = bin;
    if (bin > maxBin) maxBin = bin;

    

    memset(&cp, 0, sizeof(cp));

    
    if ((PRWord*)sp->limit > _pr_gcData.highSeg) {
      _pr_gcData.highSeg = (PRWord*) sp->limit;
    }
    _pr_gcData.allocMemory += allocSize;
    _pr_gcData.freeMemory  += allocSize;

    return PR_TRUE;
  }
  PR_DELETE(hbits);
  return PR_FALSE;
}
#endif 

static GCSeg *GrowHeapExactly(PRInt32 requestedSize)
{
    GCSeg *sp = DoGrowHeap(requestedSize, PR_TRUE);
    return sp;
}

static PRBool GrowHeap(PRInt32 requestedSize)
{
  void *p;
#ifdef USE_EXTEND_HEAP
  if (ExtendHeap(requestedSize)) {
    return PR_TRUE;
  }
#endif
  p = DoGrowHeap(requestedSize, PR_FALSE);
  return (p != NULL ? PR_TRUE : PR_FALSE);
}




static void ShrinkGCHeap(GCSeg *sp)
{
#ifdef GCMETER
    if (_pr_gcMeter & _GC_METER_GROWTH) {
        fprintf(stderr, "[GC: free segment base=%p size=%ld]\n",
                sp->base, (long) (sp->limit - sp->base));
    }
#endif    

    




    sp->info->next = freeSegs;
    freeSegs = sp->info;
    collectorCleanupNeeded = 1;
    _pr_gcData.allocMemory -= sp->limit - sp->base;
    if (sp == lastInHeap) lastInHeap = 0;

    
    --nsegs;
    if ((sp - segs) != nsegs) {
        *sp = segs[nsegs];
    } else {
        sp->base = 0;
        sp->limit = 0;
        sp->hbits = 0;
    sp->info = 0;
    }

    
    _pr_gcData.lowSeg  = (PRWord*) segs[0].base;
    _pr_gcData.highSeg = (PRWord*) segs[0].limit;
    for (sp = segs; sp < &segs[nsegs]; sp++) {
    if ((PRWord*)sp->base < _pr_gcData.lowSeg) {
        _pr_gcData.lowSeg = (PRWord*) sp->base;
    }
    if ((PRWord*)sp->limit > _pr_gcData.highSeg) {
        _pr_gcData.highSeg = (PRWord*) sp->limit;
    }
    }
}

static void FreeSegments(void)
{
    GCSegInfo *si;

    while (0 != freeSegs) {
    LOCK_GC();
    si = freeSegs;
    if (si) {
        freeSegs = si->next;
    }
    UNLOCK_GC();

    if (!si) {
        break;
    }
    PR_DELETE(si->base);
    PR_DELETE(si->hbits);
    PR_DELETE(si);
    }
}



void ScanScanQ(GCScanQ *iscan)
{
    PRWord *p;
    PRWord **pp;
    PRWord **epp;
    GCScanQ nextQ, *scan, *next, *temp;
    CollectorType *ct;

    if (!iscan->queued) return;

    _GCTRACE(GC_MARK, ("begin scanQ @ 0x%x (%d)", iscan, iscan->queued));
    scan = iscan;
    next = &nextQ;
    while (scan->queued) {
	_GCTRACE(GC_MARK, ("continue scanQ @ 0x%x (%d)", scan, scan->queued));
    



    pScanQ = next;
    next->queued = 0;

    
    pp = scan->q;
    epp = &scan->q[scan->queued];
    scan->queued = 0;
    while (pp < epp) {
        p = *pp++;
        ct = &_pr_collectorTypes[GET_TYPEIX(p[0])];
        PR_ASSERT(0 != ct->gctype.scan);
        
        (*ct->gctype.scan)(p + 1);
    }

    
    temp = scan;
    scan = next;
    next = temp;
    }

    pScanQ = iscan;
    PR_ASSERT(nextQ.queued == 0);
    PR_ASSERT(iscan->queued == 0);
}







static void PR_CALLBACK ProcessRootBlock(void **base, PRInt32 count)
{
    GCSeg *sp;
    PRWord *p0, *p, h, tix, *low, *high, *segBase;
    CollectorType *ct;
#ifdef DEBUG
    void **base0 = base;
#endif

    low = _pr_gcData.lowSeg;
    high = _pr_gcData.highSeg;
    while (--count >= 0) {
        p0 = (PRWord*) *base++;
        if (p0 < low) continue;                  
        if (p0 >= high) continue;                
        
        
    sp = lastInHeap;
        if (!sp || !IN_SEGMENT(sp,p0)) {
            GCSeg *esp;
            sp = segs;
        esp = segs + nsegs;
            for (; sp < esp; sp++) {
                if (IN_SEGMENT(sp, p0)) {
                    lastInHeap = sp;
                    goto find_object;
                }
            }
            continue;
        }

      find_object:
        
        
        p = (PRWord*) ((PRWord)p0 & ~(PR_BYTES_PER_WORD-1L));
        segBase = (PRWord *) sp->base;
        do {
            if (IS_HBIT(sp, p)) {
                goto winner;
            }
            p--;
        } while (p >= segBase);

        





#ifdef DEBUG
        PR_Abort();
#endif

      winner:
        h = p[0];
        if ((h & MARK_BIT) == 0) {
#ifdef DEBUG
            _GCTRACE(GC_ROOTS,
            ("root 0x%p (%d) base0=%p off=%d",
             p, OBJ_BYTES(h), base0, (base-1) - base0));
#endif

            
            p[0] = h | MARK_BIT;

            



            tix = (PRWord)GET_TYPEIX(h);
        ct = &_pr_collectorTypes[tix];
        if (0 == ct->gctype.scan) {
        continue;
        }

            








            pScanQ->q[pScanQ->queued++] = p;
            if (pScanQ->queued == MAX_SCAN_Q) {
                METER(_pr_scanDepth++);
                ScanScanQ(pScanQ);
            }
        }
    }
}

static void PR_CALLBACK ProcessRootPointer(void *ptr)
{
  PRWord *p0, *p, h, tix, *segBase;
  GCSeg* sp;
  CollectorType *ct;

  p0 = (PRWord*) ptr;

  if (p0 < _pr_gcData.lowSeg) return;                  
  if (p0 >= _pr_gcData.highSeg) return;                

  
  
  sp = lastInHeap;
  if (!sp || !IN_SEGMENT(sp,p0)) {
    GCSeg *esp;
    sp = segs;
    esp = segs + nsegs;
    for (; sp < esp; sp++) {
      if (IN_SEGMENT(sp, p0)) {
    lastInHeap = sp;
    goto find_object;
      }
    }
    return;
  }

 find_object:
  
  
    p = (PRWord*) ((PRWord)p0 & ~(BYTES_PER_WORD-1L));
    segBase = (PRWord *) sp->base;
    do {
      if (IS_HBIT(sp, p)) {
    goto winner;
      }
      p--;
    } while (p >= segBase);

    





#ifdef DEBUG
    PR_Abort();
#endif

 winner:
  h = p[0];
  if ((h & MARK_BIT) == 0) {
#ifdef DEBUG
    _GCTRACE(GC_ROOTS, ("root 0x%p (%d)", p, OBJ_BYTES(h)));
#endif

    
    p[0] = h | MARK_BIT;

    



    tix = (PRWord)GET_TYPEIX(h);
    ct = &_pr_collectorTypes[tix];
    if (0 == ct->gctype.scan) {
      return;
    }

    








    pScanQ->q[pScanQ->queued++] = p;
    if (pScanQ->queued == MAX_SCAN_Q) {
      METER(_pr_scanDepth++);
      ScanScanQ(pScanQ);
    }
  }
}









static void EmptyFreelists(void)
{
    GCFreeChunk *cp;
    GCFreeChunk *next;
    GCSeg *sp;
    PRWord *p;
    PRInt32 chunkSize;
    PRInt32 bin;

    



    for (bin = 0; bin <= NUM_BINS-1; bin++) {
        cp = bins[bin];
        while (cp) {
            next = cp->next;
            sp = cp->segment;
            chunkSize = cp->chunkSize >> BYTES_PER_WORD_LOG2;
            p = (PRWord*) cp;
            PR_ASSERT(chunkSize != 0);
            p[0] = MAKE_HEADER(FREE_MEMORY_TYPEIX, chunkSize);
            SET_HBIT(sp, p);
            cp = next;
        }
        bins[bin] = 0;
    }
    minBin = NUM_BINS - 1;
    maxBin = 0;
}

typedef struct GCBlockEnd {
    PRInt32	check;
#ifdef GC_CHECK
    PRInt32	requestedBytes;
#endif
#ifdef GC_STATS
    PRInt32	bin;
    PRInt64	allocTime; 
#endif
#ifdef GC_TRACEROOTS
    PRInt32	traceGeneration;	
#endif
} GCBlockEnd;

#define PR_BLOCK_END	0xDEADBEEF



#ifdef GC_STATS

typedef struct GCStat {
    PRInt32	nallocs;
    double	allocTime;
    double	allocTimeVariance;
    PRInt32	nfrees;
    double	lifetime;
    double	lifetimeVariance;
} GCStat;

#define GCSTAT_BINS	NUM_BINS

GCStat gcstats[GCSTAT_BINS];

#define GCLTFREQ_BINS	NUM_BINS

PRInt32 gcltfreq[GCSTAT_BINS][GCLTFREQ_BINS];

#include <math.h>

static char* 
pr_GetSizeString(PRUint32 size)
{
    char* sizeStr;
    if (size < 1024)
	sizeStr = PR_smprintf("<= %ld", size);
    else if (size < 1024 * 1024)
	sizeStr = PR_smprintf("<= %ldk", size / 1024);
    else 
	sizeStr = PR_smprintf("<= %ldM", size / (1024 * 1024));
    return sizeStr;
}

static void
pr_FreeSizeString(char *sizestr)
{
	PR_smprintf_free(sizestr);
}


static void
pr_PrintGCAllocStats(FILE* out)
{
    PRInt32 i, j;
    _PR_DebugPrint(out, "\n--Allocation-Stats-----------------------------------------------------------");
    _PR_DebugPrint(out, "\n--Obj-Size----Count-----Avg-Alloc-Time-----------Avg-Lifetime---------%%Freed-\n");
    for (i = 0; i < GCSTAT_BINS; i++) {
	GCStat stat = gcstats[i];
	double allocTimeMean = 0.0, allocTimeVariance = 0.0, lifetimeMean = 0.0, lifetimeVariance = 0.0;
	PRUint32 maxSize = (1 << i);
	char* sizeStr;
	if (stat.nallocs != 0.0) {
	    allocTimeMean = stat.allocTime / stat.nallocs;
	    allocTimeVariance = fabs(stat.allocTimeVariance / stat.nallocs - allocTimeMean * allocTimeMean);
	}
	if (stat.nfrees != 0.0) {
	    lifetimeMean = stat.lifetime / stat.nfrees;
	    lifetimeVariance = fabs(stat.lifetimeVariance / stat.nfrees - lifetimeMean * lifetimeMean);
	}
	sizeStr = pr_GetSizeString(maxSize);
	_PR_DebugPrint(out, "%10s %8lu %10.3f +- %10.3f %10.3f +- %10.3f (%2ld%%)\n",
		       sizeStr, stat.nallocs,
		       allocTimeMean, sqrt(allocTimeVariance),
		       lifetimeMean, sqrt(lifetimeVariance),
		       (stat.nallocs ? (stat.nfrees * 100 / stat.nallocs) : 0));
	pr_FreeSizeString(sizeStr);
    }
    _PR_DebugPrint(out, "--Lifetime-Frequency-Counts----------------------------------------------------\n");
    _PR_DebugPrint(out, "size\\cnt");
    for (j = 0; j < GCLTFREQ_BINS; j++) {
	_PR_DebugPrint(out, "\t%lu", j);
    }
    _PR_DebugPrint(out, "\n");
    for (i = 0; i < GCSTAT_BINS; i++) {
	PRInt32* freqs = gcltfreq[i];
	_PR_DebugPrint(out, "%lu", (1 << i));
	for (j = 0; j < GCLTFREQ_BINS; j++) {
	    _PR_DebugPrint(out, "\t%lu", freqs[j]);
	}
	_PR_DebugPrint(out, "\n");
    }
    _PR_DebugPrint(out, "-------------------------------------------------------------------------------\n");
}

PR_PUBLIC_API(void)
PR_PrintGCAllocStats(void)
{
    pr_PrintGCAllocStats(stderr);
}

#endif 







static PRBool SweepSegment(GCSeg *sp)
{
    PRWord h, tix;
    PRWord *p;
    PRWord *np;
    PRWord *limit;
    GCFreeChunk *cp;
    PRInt32 bytes, chunkSize, segmentSize, totalFree;
    CollectorType *ct;
    PRInt32 bin;

    



    totalFree = 0;
    segmentSize = sp->limit - sp->base;
    p = (PRWord *) sp->base;
    limit = (PRWord *) sp->limit;
    PR_ASSERT(segmentSize > 0);
    while (p < limit) {
    chunkSize = 0;
    cp = (GCFreeChunk *) p;

    
    for (;;) {
        PR_ASSERT(IS_HBIT(sp, p) != 0);
        h = p[0];
        bytes = OBJ_BYTES(h);
        PR_ASSERT(bytes != 0);
        np = (PRWord *) ((char *)p + bytes);
        tix = (PRWord)GET_TYPEIX(h);
        if ((h & MARK_BIT) && (tix != FREE_MEMORY_TYPEIX)) {
#ifdef DEBUG
        if (tix != FREE_MEMORY_TYPEIX) {
            PR_ASSERT(_pr_collectorTypes[tix].flags != 0);
        }
#endif
        p[0] = h & ~(MARK_BIT|FINAL_BIT);
		_GCTRACE(GC_SWEEP, ("busy 0x%x (%d)", p, bytes));
		break;
	    }
	    _GCTRACE(GC_SWEEP, ("free 0x%x (%d)", p, bytes));

	    
#ifdef GC_STATS
	    {
		PRInt32 userSize = bytes - sizeof(GCBlockEnd);
		GCBlockEnd* end = (GCBlockEnd*)((char*)p + userSize);
		if (userSize >= 0 && end->check == PR_BLOCK_END) {
		    PRInt64 now = PR_Now();
		    double nowd, delta;
		    PRInt32 freq;
		    LL_L2D(nowd, now);
		    delta = nowd - end->allocTime;
		    gcstats[end->bin].nfrees++;
		    gcstats[end->bin].lifetime += delta;
		    gcstats[end->bin].lifetimeVariance += delta * delta;

		    InlineBinNumber(freq, delta);
		    gcltfreq[end->bin][freq]++;

		    end->check = 0;
		}
	    }
#endif
        CLEAR_HBIT(sp, p);
        ct = &_pr_collectorTypes[tix];
        if (0 != ct->gctype.free) {
                (*ct->gctype.free)(p + 1);
            }
        chunkSize = chunkSize + bytes;
        if (np == limit) {
        
        break;
        }
        PR_ASSERT(np < limit);
        p = np;
    }

    if (chunkSize) {
        _GCTRACE(GC_SWEEP, ("free chunk 0x%p to 0x%p (%d)",
                   cp, (char*)cp + chunkSize - 1, chunkSize));
        if (chunkSize < MIN_FREE_CHUNK_BYTES) {
        
                METER(meter.wastedBytes += chunkSize);
        p = (PRWord *) cp;
        chunkSize >>= BYTES_PER_WORD_LOG2;
        PR_ASSERT(chunkSize != 0);
        p[0] = MAKE_HEADER(FREE_MEMORY_TYPEIX, chunkSize);
        SET_HBIT(sp, p);
        } else {
                
                if (chunkSize == segmentSize) {
                    
            if (sp->info->fromMalloc) {
                    ShrinkGCHeap(sp);
                    return PR_TRUE;
                }
                }

                
                cp->segment = sp;
        cp->chunkSize = chunkSize;
                InlineBinNumber(bin, chunkSize)
                cp->next = bins[bin];
                bins[bin] = cp;
        if (bin < minBin) minBin = bin;
        if (bin > maxBin) maxBin = bin;

        
        memset(cp+1, 0, chunkSize - sizeof(*cp));
                METER(meter.numFreeChunks++);
        totalFree += chunkSize;
        }
    }

    
    p = np;
    }

    PR_ASSERT(totalFree <= segmentSize);

    _pr_gcData.freeMemory += totalFree;
    _pr_gcData.busyMemory += (sp->limit - sp->base) - totalFree;
    return PR_FALSE;
}






PRCList _pr_finalizeableObjects;



PRCList _pr_finalQueue;





typedef struct GCFinalStr {
    PRCList links;
    PRWord *object;
} GCFinal;


#define FinalPtr(_qp) \
    ((GCFinal*) ((char*) (_qp) - offsetof(GCFinal,links)))

static GCFinal *AllocFinalNode(void)
{
    return PR_NEWZAP(GCFinal);
}

static void FreeFinalNode(GCFinal *node)
{
    PR_DELETE(node);
}













static void PrepareFinalize(void)
{
    PRCList *qp;
    GCFinal *fp;
    PRWord h;
    PRWord *p;
    void (PR_CALLBACK *livePointer)(void *ptr);
#ifdef DEBUG
    CollectorType *ct;
#endif

    
    PR_ASSERT( GC_IS_LOCKED() );

    
    livePointer = _pr_gcData.livePointer;

    



    qp = _pr_finalizeableObjects.next;
    while (qp != &_pr_finalizeableObjects) {
    fp = FinalPtr(qp);
    qp = qp->next;
    h = fp->object[0];        
    if (h & MARK_BIT) {
        
        continue;
    }

#ifdef DEBUG
    ct = &_pr_collectorTypes[GET_TYPEIX(h)];
    PR_ASSERT((0 != ct->flags) && (0 != ct->gctype.finalize));
#endif
    fp->object[0] |= FINAL_BIT;
    _GCTRACE(GC_FINAL, ("moving %p (%d) to finalQueue",
               fp->object, OBJ_BYTES(h)));
    }

    



    qp = _pr_finalizeableObjects.next;
    while (qp != &_pr_finalizeableObjects) {
    fp = FinalPtr(qp);
    qp = qp->next;
    h = fp->object[0];        
    if ((h & FINAL_BIT) == 0) {
        continue;
    }

    
        p = &fp->object[1];
    (*livePointer)(p);
    PR_REMOVE_LINK(&fp->links);
    PR_APPEND_LINK(&fp->links, &_pr_finalQueue);
    }
}









extern void PR_CALLBACK _PR_ScanFinalQueue(void *notused)
{
#ifdef XP_MAC
#pragma unused (notused)
#endif
    PRCList *qp;
    GCFinal *fp;
    PRWord *p;
    void ( PR_CALLBACK *livePointer)(void *ptr);

    livePointer = _pr_gcData.livePointer;
    qp = _pr_finalQueue.next;
    while (qp != &_pr_finalQueue) {
    fp = FinalPtr(qp);
	_GCTRACE(GC_FINAL, ("marking 0x%x (on final queue)", fp->object));
        p = &fp->object[1];
    (*livePointer)(p);
    qp = qp->next;
    }
}

void PR_CALLBACK FinalizerLoop(void* unused)
{
#ifdef XP_MAC
#pragma unused (unused)
#endif
    GCFinal *fp;
    PRWord *p;
    PRWord h, tix;
    CollectorType *ct;

    LOCK_GC();
    for (;;) {
	p = 0; h = 0;		
    while (PR_CLIST_IS_EMPTY(&_pr_finalQueue))
        PR_Wait(_pr_gcData.lock, PR_INTERVAL_NO_TIMEOUT);

    _GCTRACE(GC_FINAL, ("begin finalization"));
    while (_pr_finalQueue.next != &_pr_finalQueue) {
        fp = FinalPtr(_pr_finalQueue.next);
        PR_REMOVE_LINK(&fp->links);
        p = fp->object;

        h = p[0];        
        tix = (PRWord)GET_TYPEIX(h);
        ct = &_pr_collectorTypes[tix];
	    _GCTRACE(GC_FINAL, ("finalize 0x%x (%d)", p, OBJ_BYTES(h)));

        




        UNLOCK_GC();
        FreeFinalNode(fp);
        PR_ASSERT(ct->gctype.finalize != 0);
        (*ct->gctype.finalize)(p + 1);
        LOCK_GC();
    }
    _GCTRACE(GC_FINAL, ("end finalization"));
    PR_Notify(_pr_gcData.lock);
    }
}

static void NotifyFinalizer(void)
{
    if (!PR_CLIST_IS_EMPTY(&_pr_finalQueue)) {
    PR_ASSERT( GC_IS_LOCKED() );
    PR_Notify(_pr_gcData.lock);
    }
}

void _PR_CreateFinalizer(PRThreadScope scope)
{
    if (!_pr_gcData.finalizer) {
    _pr_gcData.finalizer = PR_CreateThreadGCAble(PR_SYSTEM_THREAD,
                                        FinalizerLoop, 0,
                                        PR_PRIORITY_LOW, scope,
                                        PR_UNJOINABLE_THREAD, 0);
    
    if (_pr_gcData.finalizer == NULL)
        
        PR_Abort();

    }
}

void pr_FinalizeOnExit(void)
{
#ifdef DEBUG_warren
    OutputDebugString("### Doing finalize-on-exit pass\n");
#endif
    PR_ForceFinalize();
#ifdef DEBUG_warren
    OutputDebugString("### Finalize-on-exit complete. Dumping object left to memory.out\n");
    PR_DumpMemorySummary();
    PR_DumpMemory(PR_TRUE);
#endif
}

PR_IMPLEMENT(void) PR_ForceFinalize()
{
    LOCK_GC();
    NotifyFinalizer();
    while (!PR_CLIST_IS_EMPTY(&_pr_finalQueue)) {
    PR_ASSERT( GC_IS_LOCKED() );
    (void) PR_Wait(_pr_gcData.lock, PR_INTERVAL_NO_TIMEOUT);
    }
    UNLOCK_GC();

    
}



typedef struct GCWeakStr {
    PRCList links;
    PRWord *object;
} GCWeak;




#define WeakPtr(_qp) \
    ((GCWeak*) ((char*) (_qp) - offsetof(GCWeak,links)))

PRCList _pr_weakLinks = PR_INIT_STATIC_CLIST(&_pr_weakLinks);
PRCList _pr_freeWeakLinks = PR_INIT_STATIC_CLIST(&_pr_freeWeakLinks);

#define WEAK_FREELIST_ISEMPTY() (_pr_freeWeakLinks.next == &_pr_freeWeakLinks)





static void PR_CALLBACK ScanWeakFreeList(void *notused) {
#ifdef XP_MAC
#pragma unused (notused)
#endif
    PRCList *qp = _pr_freeWeakLinks.next;
    while (qp != &_pr_freeWeakLinks) {
    GCWeak *wp = WeakPtr(qp);
    qp = qp->next;
    ProcessRootPointer(wp->object);
    }
}







static void EmptyWeakFreeList(void) {
    if (!WEAK_FREELIST_ISEMPTY()) {
    PRCList *qp, freeLinks;

    PR_INIT_CLIST(&freeLinks);

    



    LOCK_GC();
    qp = _pr_freeWeakLinks.next;
    while (qp != &_pr_freeWeakLinks) {
        GCWeak *wp = WeakPtr(qp);
        qp = qp->next;
        PR_REMOVE_LINK(&wp->links);
        PR_APPEND_LINK(&wp->links, &freeLinks);
    }
    UNLOCK_GC();

    
    qp = freeLinks.next;
    while (qp != &freeLinks) {
        GCWeak *wp = WeakPtr(qp);
        qp = qp->next;
        PR_DELETE(wp);
    }
    }
}




static GCWeak *AllocWeakNode(void)
{
    EmptyWeakFreeList();
    return PR_NEWZAP(GCWeak);
}

static void FreeWeakNode(GCWeak *node)
{
    PR_DELETE(node);
}







static void CheckWeakLinks(void) {
    PRCList *qp;
    GCWeak *wp;
    PRWord *p, h, tix, **weakPtrAddress;
    CollectorType *ct;
    PRUint32 offset;

    qp = _pr_weakLinks.next;
    while (qp != &_pr_weakLinks) {
    wp = WeakPtr(qp);
    qp = qp->next;
    if ((p = wp->object) != 0) {
        h = p[0];        
        if ((h & MARK_BIT) == 0) {
        





        PR_REMOVE_LINK(&wp->links);
        PR_APPEND_LINK(&wp->links, &_pr_freeWeakLinks);
        collectorCleanupNeeded = 1;
        continue;
        }
        
	    
        tix = GET_TYPEIX(h);
        ct = &_pr_collectorTypes[tix];
        PR_ASSERT((ct->flags != 0) && (ct->gctype.getWeakLinkOffset != 0));
        if (0 == ct->gctype.getWeakLinkOffset) {
        
        continue;
        }

        
        offset = (*ct->gctype.getWeakLinkOffset)(p + 1);

        
        weakPtrAddress = (PRWord**)((char*)(p + 1) + offset);
        p = *weakPtrAddress;
        if (p != 0) {
        h = p[-1];    
        if (h & MARK_BIT) {
            
            continue;
        }
        
        *weakPtrAddress = 0;
        }
    }
    }
}







extern GCLockHook *_pr_GCLockHook;

static void dogc(void)
{
    RootFinder *rf;
    GCLockHook* lhook;

    GCScanQ scanQ;
    GCSeg *sp, *esp;
    PRInt64 start, end, diff;

#if defined(GCMETER) || defined(GCTIMINGHOOK)
    start = PR_Now();
#endif

    




    



    if (_pr_GCLockHook) {
        for (lhook = _pr_GCLockHook->next; lhook != _pr_GCLockHook; 
          lhook = lhook->next) {
          (*lhook->func)(PR_GCBEGIN, lhook->arg);
        }
    }

    PR_SuspendAll();

#ifdef GCMETER
    
    if (_pr_gcMeter & _GC_METER_STATS) {
        fprintf(stderr,
                "[GCSTATS: busy:%ld skipped:%ld, alloced:%ld+wasted:%ld+free:%ld = total:%ld]\n",
                (long) _pr_gcData.busyMemory,
                (long) meter.skippedFreeChunks,
                (long) meter.allocBytes,
                (long) meter.wastedBytes,
                (long) _pr_gcData.freeMemory,
                (long) _pr_gcData.allocMemory);
    }        
    memset(&meter, 0, sizeof(meter));
#endif

    PR_LOG(_pr_msgc_lm, PR_LOG_ALWAYS, ("begin mark phase; busy=%d free=%d total=%d",
                     _pr_gcData.busyMemory, _pr_gcData.freeMemory,
                     _pr_gcData.allocMemory));

    if (_pr_beginGCHook) {
    (*_pr_beginGCHook)(_pr_beginGCHookArg);
    }

    



    memset(&scanQ, 0, sizeof(scanQ));
    pScanQ = &scanQ;

    
    

    EmptyFreelists();

    
    PR_LOG(_pr_msgc_lm, PR_LOG_WARNING,
           ("begin mark phase; busy=%d free=%d total=%d",
        _pr_gcData.busyMemory, _pr_gcData.freeMemory,
            _pr_gcData.allocMemory));
    METER(_pr_scanDepth = 0);
    rf = _pr_rootFinders;
    while (rf) {
    _GCTRACE(GC_ROOTS, ("finding roots in %s", rf->name));
    (*rf->func)(rf->arg);
    rf = rf->next;
    }
    _GCTRACE(GC_ROOTS, ("done finding roots"));

    
    ScanScanQ(&scanQ);
    PR_ASSERT(pScanQ == &scanQ);
    PR_ASSERT(scanQ.queued == 0);
    METER({
    if (_pr_scanDepth > _pr_maxScanDepth) {
        _pr_maxScanDepth = _pr_scanDepth;
    }
    });

    
    

    METER(_pr_scanDepth = 0);
    PrepareFinalize();

    
    ScanScanQ(&scanQ);
    PR_ASSERT(pScanQ == &scanQ);
    PR_ASSERT(scanQ.queued == 0);
    METER({
    if (_pr_scanDepth > _pr_maxScanDepth) {
        _pr_maxScanDepth = _pr_scanDepth;
    }
    });
    pScanQ = 0;

    
    

    



    CheckWeakLinks();
    _GCTRACE(GC_SWEEP, ("begin sweep phase"));
    _pr_gcData.freeMemory = 0;
    _pr_gcData.busyMemory = 0;
    sp = segs;
    esp = sp + nsegs;
    while (sp < esp) {
        if (SweepSegment(sp)) {
            



            esp--;
            continue;
        }
        sp++;
    }

#if defined(GCMETER) || defined(GCTIMINGHOOK)
    end = PR_Now();
#endif
#ifdef GCMETER
    LL_SUB(diff, end, start);
    PR_LOG(GC, PR_LOG_ALWAYS,
	   ("done; busy=%d free=%d chunks=%d total=%d time=%lldms",
	    _pr_gcData.busyMemory, _pr_gcData.freeMemory,
	    meter.numFreeChunks, _pr_gcData.allocMemory, diff));
    if (_pr_gcMeter & _GC_METER_FREE_LIST) {
        PRIntn bin;
        fprintf(stderr, "Freelist bins:\n");
        for (bin = 0; bin < NUM_BINS; bin++) {
            GCFreeChunk *cp = bins[bin];
            while (cp != NULL) {
                fprintf(stderr, "%3d: %p %8ld\n",
                        bin, cp, (long) cp->chunkSize);
                cp = cp->next;
            }
        }
    }
#endif

    if (_pr_endGCHook) {
    (*_pr_endGCHook)(_pr_endGCHookArg);
    }

    
    bigAllocBytes = 0;

    
    PR_ResumeAll();

    if (_pr_GCLockHook) {
        for (lhook = _pr_GCLockHook->prev; lhook != _pr_GCLockHook; 
          lhook = lhook->prev) {
          (*lhook->func)(PR_GCEND, lhook->arg);
        }
    }

    
    NotifyFinalizer();
#ifdef GCTIMINGHOOK
    if (_pr_gcData.gcTimingHook) {
	PRInt32 time;
	LL_SUB(diff, end, start);
	LL_L2I(time, diff);
	_pr_gcData.gcTimingHook(time);
    }
#endif
}

PR_IMPLEMENT(void) PR_GC(void)
{
    LOCK_GC();
    dogc();
    UNLOCK_GC();

    EmptyWeakFreeList();
}











static PRInt32 PR_CALLBACK
pr_ConservativeWalkPointer(void* ptr, PRWalkFun walkRootPointer, void* data)
{
  PRWord *p0, *p, *segBase;
  GCSeg* sp;

  p0 = (PRWord*) ptr;

  if (p0 < _pr_gcData.lowSeg) return 0;                  
  if (p0 >= _pr_gcData.highSeg) return 0;                

  
  
  sp = lastInHeap;
  if (!sp || !IN_SEGMENT(sp,p0)) {
    GCSeg *esp;
    sp = segs;
    esp = segs + nsegs;
    for (; sp < esp; sp++) {
      if (IN_SEGMENT(sp, p0)) {
	lastInHeap = sp;
	goto find_object;
      }
    }
    return 0;
  }

  find_object:
    
    
    p = (PRWord*) ((PRWord)p0 & ~(BYTES_PER_WORD-1L));
    segBase = (PRWord *) sp->base;
    do {
        if (IS_HBIT(sp, p)) {
            goto winner;
        }
        p--;
    } while (p >= segBase);

    





#ifdef DEBUG
    PR_Abort();
#endif
    return 0;

 winner:
    return walkRootPointer(p, data);
}

static PRInt32 PR_CALLBACK
pr_ConservativeWalkBlock(void **base, PRInt32 count,
			 PRWalkFun walkRootPointer, void* data)
{
    PRWord *p0;
    while (--count >= 0) {
	PRInt32 status;
        p0 = (PRWord*) *base++;
	status = pr_ConservativeWalkPointer(p0, walkRootPointer, data);
	if (status) return status;
    }
    return 0;
}



typedef void (*WalkObject_t)(FILE *out, GCType* tp, PRWord *obj,
			     size_t bytes, PRBool detailed);
typedef void (*WalkUnknown_t)(FILE *out, GCType* tp, PRWord tix, PRWord *p,
			      size_t bytes, PRBool detailed);
typedef void (*WalkFree_t)(FILE *out, PRWord *p, size_t size, PRBool detailed);
typedef void (*WalkSegment_t)(FILE *out, GCSeg* sp, PRBool detailed);

static void
pr_WalkSegment(FILE* out, GCSeg* sp, PRBool detailed,
           char* enterMsg, char* exitMsg,
           WalkObject_t walkObject, WalkUnknown_t walkUnknown, WalkFree_t walkFree)
{
    PRWord *p, *limit;

    p = (PRWord *) sp->base;
    limit = (PRWord *) sp->limit;
    if (enterMsg)
    fprintf(out, enterMsg, p);
    while (p < limit)
    {
    if (IS_HBIT(sp, p)) 
    {
        PRWord h = p[0];
        PRWord tix = GET_TYPEIX(h);
        size_t bytes = OBJ_BYTES(h);
        PRWord* np = (PRWord*) ((char*)p + bytes);

        GCType* tp = &_pr_collectorTypes[tix].gctype;
        if ((0 != tp) && walkObject)
        walkObject(out, tp, p, bytes, detailed);
        else if (walkUnknown)
        walkUnknown(out, tp, tix, p, bytes, detailed);
        p = np;
    }
    else
    {
        
        size_t size = ((GCFreeChunk*)p)->chunkSize;
        if (walkFree)
        walkFree(out, p, size, detailed);
        p = (PRWord*)((char*)p + size);
    }
    }
    if (p != limit)
    fprintf(out, "SEGMENT OVERRUN (end should be at 0x%p)\n", limit);
    if (exitMsg)
    fprintf(out, exitMsg, p);
}

static void
pr_WalkSegments(FILE *out, WalkSegment_t walkSegment, PRBool detailed)
{
    GCSeg *sp = segs;
    GCSeg *esp;

    LOCK_GC();
    esp = sp + nsegs;
    while (sp < esp)
    {
    walkSegment(out, sp, detailed);
    sp++;
    }
    fprintf(out, "End of heap\n");
    UNLOCK_GC();
}





PR_IMPLEMENT(void)
PR_DumpIndent(FILE *out, int indent)
{
    while (--indent >= 0)
    fprintf(out, " ");
}

static void
PR_DumpHexWords(FILE *out, PRWord *p, int nWords,
        int indent, int nWordsPerLine)
{
    while (nWords > 0)
    {
    int i;

    PR_DumpIndent(out, indent);
    i = nWordsPerLine;
    if (i > nWords)
        i = nWords;
    nWords -= i;
    while (i--)
    {
        fprintf(out, "0x%.8lX", (long) *p++);
        if (i)
        fputc(' ', out);
    }
    fputc('\n', out);
    }
}

static void PR_CALLBACK
pr_DumpObject(FILE *out, GCType* tp, PRWord *p, 
          size_t bytes, PRBool detailed)
{
    char kindChar = tp->kindChar;
    fprintf(out, "0x%p: 0x%.6lX %c  ",
            p, (long) bytes, kindChar ? kindChar : '?');
    if (tp->dump)
    (*tp->dump)(out, (void*) (p + 1), detailed, 0);
    if (detailed)
    PR_DumpHexWords(out, p, bytes>>2, 22, 4);
}
    
static void PR_CALLBACK
pr_DumpUnknown(FILE *out, GCType* tp, PRWord tix, PRWord *p, 
           size_t bytes, PRBool detailed)
{
    char kindChar = tp->kindChar;
    fprintf(out, "0x%p: 0x%.6lX %c  ",
            p, (long) bytes, kindChar ? kindChar : '?');
    fprintf(out, "UNKNOWN KIND %ld\n", (long) tix);
    if (detailed)
    PR_DumpHexWords(out, p, bytes>>2, 22, 4);
}

static void PR_CALLBACK
pr_DumpFree(FILE *out, PRWord *p, size_t size, PRBool detailed)
{
#if defined(XP_MAC) && XP_MAC
# pragma unused( detailed )
#endif

    fprintf(out, "0x%p: 0x%.6lX -  FREE\n", p, (long) size);
}

static void PR_CALLBACK
pr_DumpSegment(FILE* out, GCSeg* sp, PRBool detailed)
{
    pr_WalkSegment(out, sp, detailed,
           "\n   Address: Length\n0x%p: Beginning of segment\n",
           "0x%p: End of segment\n\n",
           pr_DumpObject, pr_DumpUnknown, pr_DumpFree);
}

static void pr_DumpRoots(FILE *out);




PR_IMPLEMENT(void)
PR_DumpGCHeap(FILE *out, PRBool detailed)
{
    fprintf(out, "\n"
        "The kinds are:\n"
        " U unscanned block\n"
        " W weak link block\n"
        " S scanned block\n"
        " F scanned and final block\n"
        " C class record\n"
        " X context record\n"
        " - free list item\n"
        " ? other\n");
    LOCK_GC();
    pr_WalkSegments(out, pr_DumpSegment, detailed);
    if (detailed)
    pr_DumpRoots(out);
    UNLOCK_GC();
}

PR_IMPLEMENT(void)
PR_DumpMemory(PRBool detailed)
{
    PR_DumpToFile("memory.out", "Dumping memory", PR_DumpGCHeap, detailed);
}



static PRInt32 PR_CALLBACK
pr_DumpRootPointer(PRWord* p, void* data)
{
#ifdef XP_MAC
#pragma unused(data)
#endif
    PRWord h = p[0];
    PRWord tix = GET_TYPEIX(h);
      size_t bytes = OBJ_BYTES(h);
      
      GCType* tp = &_pr_collectorTypes[tix].gctype;
      if (0 != tp)
      pr_DumpObject(_pr_gcData.dumpOutput, tp, p, bytes, PR_FALSE);
      else
      pr_DumpUnknown(_pr_gcData.dumpOutput, tp, tix, p, bytes, PR_FALSE);
    return 0;
}

static void PR_CALLBACK
pr_ConservativeDumpRootPointer(void* ptr)
{
    (void)pr_ConservativeWalkPointer(ptr, (PRWalkFun) pr_DumpRootPointer, NULL);
}

static void PR_CALLBACK
pr_ConservativeDumpRootBlock(void **base, PRInt32 count)
{
    (void)pr_ConservativeWalkBlock(base, count, (PRWalkFun) pr_DumpRootPointer, NULL);
}

extern int
DumpThreadRoots(PRThread *t, int i, void *notused);

static void
pr_DumpRoots(FILE *out)
{
    RootFinder *rf;
    void (*liveBlock)(void **base, PRInt32 count);
    void (*livePointer)(void *ptr);
    void (*processRootBlock)(void **base, PRInt32 count);
    void (*processRootPointer)(void *ptr);

    LOCK_GC();

    liveBlock = _pr_gcData.liveBlock;
    livePointer = _pr_gcData.livePointer;
    processRootBlock = _pr_gcData.processRootBlock;
    processRootPointer = _pr_gcData.processRootPointer;
    
    _pr_gcData.liveBlock = pr_ConservativeDumpRootBlock;
    _pr_gcData.livePointer = pr_ConservativeDumpRootPointer;
    _pr_gcData.processRootBlock = pr_ConservativeDumpRootBlock;
    _pr_gcData.processRootPointer = pr_ConservativeDumpRootPointer;
    _pr_gcData.dumpOutput = out;

    rf = _pr_rootFinders;
    while (rf) {
    fprintf(out, "\n===== Roots for %s\n", rf->name);
    (*rf->func)(rf->arg);
    rf = rf->next;
    }

    _pr_gcData.liveBlock = liveBlock;
    _pr_gcData.livePointer = livePointer;
    _pr_gcData.processRootBlock = processRootBlock;
    _pr_gcData.processRootPointer = processRootPointer;
    _pr_gcData.dumpOutput = NULL;

    UNLOCK_GC();
}





PRSummaryPrinter summaryPrinter = NULL;
void* summaryPrinterClosure = NULL;

PR_IMPLEMENT(void) 
PR_RegisterSummaryPrinter(PRSummaryPrinter fun, void* closure)
{
    summaryPrinter = fun;
    summaryPrinterClosure = closure;
}

static void PR_CALLBACK
pr_SummarizeObject(FILE *out, GCType* tp, PRWord *p,
           size_t bytes, PRBool detailed)
{
#if defined(XP_MAC) && XP_MAC
# pragma unused( out, detailed )
#endif

    if (tp->summarize)
    (*tp->summarize)((void GCPTR*)(p + 1), bytes);
}

static void PR_CALLBACK
pr_DumpSummary(FILE* out, GCSeg* sp, PRBool detailed)
{
    pr_WalkSegment(out, sp, detailed, NULL, NULL,
           pr_SummarizeObject, NULL, NULL);
}

PR_IMPLEMENT(void)
PR_DumpGCSummary(FILE *out, PRBool detailed)
{
    if (summaryPrinter) {
    pr_WalkSegments(out, pr_DumpSummary, detailed);
    summaryPrinter(out, summaryPrinterClosure);
    }
#if 0
    fprintf(out, "\nFinalizable objects:\n");
    {
    PRCList *qp;
    qp = _pr_pendingFinalQueue.next;
    while (qp != &_pr_pendingFinalQueue) {
        GCFinal* fp = FinalPtr(qp);
        PRWord h = fp->object[0];        
        PRWord tix = GET_TYPEIX(h);
        GCType* tp = _pr_gcTypes[tix];
        size_t bytes = OBJ_BYTES(h);
        pr_DumpObject(out, tp, fp->object, bytes, PR_FALSE);
        qp = qp->next;
    }
    }
#endif
}

PR_IMPLEMENT(void)
PR_DumpMemorySummary(void)
{
    PR_DumpToFile("memory.out", "Memory Summary", PR_DumpGCSummary, PR_FALSE);
}





#ifdef GC_TRACEROOTS

PRInt32 pr_traceGen = 0;

static PRBool
pr_IsMarked(PRWord* p)
{
    GCBlockEnd* end = (GCBlockEnd*)((char*)p + OBJ_BYTES(p[0]) - sizeof(GCBlockEnd));
    PR_ASSERT(end->check == PR_BLOCK_END);
    return end->traceGeneration == pr_traceGen;
}

static void
pr_Mark(PRWord* p)
{
    GCBlockEnd* end = (GCBlockEnd*)((char*)p + OBJ_BYTES(p[0]) - sizeof(GCBlockEnd));
    PR_ASSERT(end->check == PR_BLOCK_END);
    end->traceGeneration = pr_traceGen;
}

PRWord* pr_traceObj;	

static PRInt32 PR_CALLBACK
pr_TraceRootObject(void* obj, void* data);

static PRInt32 PR_CALLBACK
pr_TraceRootPointer(PRWord *p, void* data)
{
    PRInt32 printTrace = 0;
    PRWord h = p[0];
    PRWord tix = GET_TYPEIX(h);
    GCType* tp = &_pr_collectorTypes[tix].gctype;
    FILE* out = _pr_gcData.dumpOutput;

    PR_ASSERT(tp);
    if (pr_IsMarked(p))
	return printTrace;

    pr_Mark(p);
    if (p == pr_traceObj) {
	fprintf(out, "\n### Found path to:\n");
	printTrace = 1;
    }
    else {
	if (PR_StackSpaceLeft(PR_GetCurrentThread()) < 512) {
	    fprintf(out, "\n### Path too deep (giving up):\n");
	    printTrace = 1;
	}
	else if (tp->walk) {
	    printTrace = tp->walk((void*)(p + 1), pr_TraceRootObject, data);
	}
	

    }

    if (printTrace == 1) {
	PR_ASSERT(tp->dump);
	fprintf(out, "0x%p: ", p);
	tp->dump(out, (void*)(p + 1), PR_FALSE, 1);
    }
    return printTrace;
}

static PRInt32 PR_CALLBACK
pr_TraceRootObject(void* obj, void* data)
{
    

    return pr_TraceRootPointer((PRWord*)obj - 1, data);
}

static void PR_CALLBACK
pr_ConservativeTraceRootPointer(PRWord *p)
{
    PRInt32 status;
    ++pr_traceGen;
    status = pr_ConservativeWalkPointer(p, pr_TraceRootPointer, NULL);
    if (status) {
	FILE* out = _pr_gcData.dumpOutput;
	fprintf(out, "### from root at 0x%p\n\n", p);
    }
}

static void PR_CALLBACK
pr_ConservativeTraceRootBlock(void **base, PRInt32 count)
{
    PRInt32 status;
    ++pr_traceGen;
    status = pr_ConservativeWalkBlock(base, count, pr_TraceRootPointer, NULL);
    if (status) {
	FILE* out = _pr_gcData.dumpOutput;
	fprintf(out, "### from root in range 0x%p + 0x%lx\n\n",
                base, (long) count);
    }
}

static void
PR_TraceRoot1(FILE* out, PRBool detailed)
{
    RootFinder *rf;
    void (*liveBlock)(void **base, PRInt32 count);
    void (*livePointer)(void *ptr);
    void (*processRootBlock)(void **base, PRInt32 count);
    void (*processRootPointer)(void *ptr);

    LOCK_GC();

    liveBlock = _pr_gcData.liveBlock;
    livePointer = _pr_gcData.livePointer;
    processRootBlock = _pr_gcData.processRootBlock;
    processRootPointer = _pr_gcData.processRootPointer;
    
    _pr_gcData.liveBlock = pr_ConservativeTraceRootBlock;
    _pr_gcData.livePointer = pr_ConservativeTraceRootPointer;
    _pr_gcData.processRootBlock = pr_ConservativeTraceRootBlock;
    _pr_gcData.processRootPointer = pr_ConservativeTraceRootPointer;
    _pr_gcData.dumpOutput = out;

    fprintf(out, "### Looking for paths to 0x%p\n\n", pr_traceObj);

    rf = _pr_rootFinders;
    while (rf) {
	fprintf(out, "\n===== Roots for %s\n", rf->name);
	(*rf->func)(rf->arg);
	rf = rf->next;
    }

    _pr_gcData.liveBlock = liveBlock;
    _pr_gcData.livePointer = livePointer;
    _pr_gcData.processRootBlock = processRootBlock;
    _pr_gcData.processRootPointer = processRootPointer;
    _pr_gcData.dumpOutput = NULL;

    UNLOCK_GC();
}

PR_PUBLIC_API(void)
PR_TraceRoot()
{
    









    PR_DumpToFile("memory.out", "Tracing Roots", PR_TraceRoot1, PR_FALSE);
}

#endif 



#if defined(DEBUG) && defined(WIN32)
static void DumpApplicationHeap(FILE *out, HANDLE heap)
{
    PROCESS_HEAP_ENTRY entry;
    DWORD err;

    if (!HeapLock(heap))
    OutputDebugString("Can't lock the heap.\n");
    entry.lpData = 0;
    fprintf(out, "   address:       size ovhd region\n");
    while (HeapWalk(heap, &entry))
    {
    WORD flags = entry.wFlags;

    fprintf(out, "0x%.8X: 0x%.8X 0x%.2X 0x%.2X  ", entry.lpData, entry.cbData,
        entry.cbOverhead, entry.iRegionIndex);
    if (flags & PROCESS_HEAP_REGION)
        fprintf(out, "REGION  committedSize=0x%.8X uncommittedSize=0x%.8X firstBlock=0x%.8X lastBlock=0x%.8X",
            entry.Region.dwCommittedSize, entry.Region.dwUnCommittedSize,
            entry.Region.lpFirstBlock, entry.Region.lpLastBlock);
    else if (flags & PROCESS_HEAP_UNCOMMITTED_RANGE)
        fprintf(out, "UNCOMMITTED");
    else if (flags & PROCESS_HEAP_ENTRY_BUSY)
    {
        if (flags & PROCESS_HEAP_ENTRY_DDESHARE)
        fprintf(out, "DDEShare ");
        if (flags & PROCESS_HEAP_ENTRY_MOVEABLE)
        fprintf(out, "Moveable Block  handle=0x%.8X", entry.Block.hMem);
        else
        fprintf(out, "Block");
    }
    fprintf(out, "\n");
    }
    if ((err = GetLastError()) != ERROR_NO_MORE_ITEMS)
    fprintf(out, "ERROR %d iterating through the heap\n", err);
    if (!HeapUnlock(heap))
    OutputDebugString("Can't unlock the heap.\n");
}
#endif

#if defined(DEBUG) && defined(WIN32)
static void DumpApplicationHeaps(FILE *out)
{
    HANDLE mainHeap;
    HANDLE heaps[100];
    DWORD nHeaps;
    PRInt32 i;

    mainHeap = GetProcessHeap();
    nHeaps = GetProcessHeaps(100, heaps);
    if (nHeaps > 100)
    nHeaps = 0;
    fprintf(out, "%ld heaps:\n", (long) nHeaps);
    for (i = 0; i<nHeaps; i++)
    {
    HANDLE heap = heaps[i];

    fprintf(out, "Heap at 0x%.8lX", (long) heap);
    if (heap == mainHeap)
        fprintf(out, " (main)");
    fprintf(out, ":\n");
    DumpApplicationHeap(out, heap);
    fprintf(out, "\n");
    }
    fprintf(out, "End of heap dump\n\n");
}
#endif

#if defined(DEBUG) && defined(WIN32)
PR_IMPLEMENT(void) PR_DumpApplicationHeaps(void)
{
    FILE *out;

    OutputDebugString("Dumping heaps...");
    out = fopen("heaps.out", "a");
    if (!out)
    OutputDebugString("Can't open \"heaps.out\"\n");
    else
    {
    struct tm *newtime;
    time_t aclock;

    time(&aclock);
    newtime = localtime(&aclock);
    fprintf(out, "Heap dump on %s\n", asctime(newtime));    
    DumpApplicationHeaps(out);
    fprintf(out, "\n\n");
    fclose(out);
    }
    OutputDebugString(" done\n");
}
#else

PR_IMPLEMENT(void) PR_DumpApplicationHeaps(void)
{
    fprintf(stderr, "Native heap dumping is currently implemented only for Windows32.\n");
}
#endif









static PRWord *BinAlloc(int cbix, PRInt32 bytes, int dub)
{
    GCFreeChunk **cpp, *cp, *cpNext;
    GCSeg *sp;
    PRInt32 chunkSize, remainder;
    PRWord *p, *np;
    PRInt32 bin, newbin;

    
    InlineBinNumber(bin,bytes)
    if (bin < minBin) {
    bin = minBin;    
    }

    
    for (; bin <= NUM_BINS-1; bin++) {
        cpp = &bins[bin];
    while ((cp = *cpp) != 0) {
        chunkSize = cp->chunkSize;
        if (chunkSize < bytes) {
        
            METER(meter.skippedFreeChunks++);
        cpp = &cp->next;
        continue;
        }

        
        p = (PRWord*) cp;
        sp = cp->segment;
        cpNext = cp->next;
#ifndef IS_64
        if (dub && (((PRWord)p & (PR_BYTES_PER_DWORD-1)) == 0)) {
        





        p[0] = MAKE_HEADER(FREE_MEMORY_TYPEIX, 1);
        SET_HBIT(sp, p);
        p++;
        chunkSize -= PR_BYTES_PER_WORD;
        bytes -= PR_BYTES_PER_WORD;
        PR_ASSERT(((PRWord)p & (PR_BYTES_PER_DWORD-1)) != 0);
        _pr_gcData.freeMemory -= PR_BYTES_PER_WORD;
        _pr_gcData.busyMemory += PR_BYTES_PER_WORD;
        }
#endif
        np = (PRWord*) ((char*) p + bytes);
        remainder = chunkSize - bytes;
        if (remainder >= MIN_FREE_CHUNK_BYTES) {
        
        cp = (GCFreeChunk*) np;
        cp->segment = sp;
        cp->chunkSize = remainder;
        InlineBinNumber(newbin, remainder)
        if (newbin != bin) {
            *cpp = (GCFreeChunk*) cpNext; 
            cp->next = bins[newbin];      
            bins[newbin] = cp;
            if (newbin < minBin) minBin = newbin;
            if (newbin > maxBin) maxBin = newbin;
        } else {
            
            cp->next = cpNext;
            *cpp = (GCFreeChunk*) np;
        }
        } else {
        




        *cpp = cpNext;
        bytes = chunkSize;
        }
        p[0] = MAKE_HEADER(cbix, (bytes >> PR_BYTES_PER_WORD_LOG2));
        SET_HBIT(sp, p);
        _pr_gcData.freeMemory -= bytes;
        _pr_gcData.busyMemory += bytes;
        return p;
    }
    }
    return 0;
}






static PRWord *BigAlloc(int cbix, PRInt32 bytes, int dub)
{
    GCSeg *sp;
    PRWord *p, h;
    PRInt32 chunkSize;

    



    if (bigAllocBytes >= BIG_ALLOC_GC_SIZE) {
        dogc();
    }
    bigAllocBytes += bytes;

    
    sp = GrowHeapExactly(bytes);

    if (sp) {
        p = (PRWord*) sp->base;
        chunkSize = sp->limit - sp->base;

        
#ifndef IS_64
        if (dub && (((PRWord)p & (PR_BYTES_PER_DWORD-1)) == 0)) {
            



            p[0] = MAKE_HEADER(FREE_MEMORY_TYPEIX, 1);
            SET_HBIT(sp, p);
            p++;
            chunkSize -= PR_BYTES_PER_WORD;
            _pr_gcData.freeMemory -= PR_BYTES_PER_WORD;
            _pr_gcData.busyMemory += PR_BYTES_PER_WORD;
            PR_ASSERT(((PRWord)p & (PR_BYTES_PER_DWORD-1)) != 0);
        }
#endif

        
        h = MAKE_HEADER(cbix, (chunkSize >> PR_BYTES_PER_WORD_LOG2));
        p[0] = h;
        SET_HBIT(sp, p);
        _pr_gcData.freeMemory -= chunkSize;
        _pr_gcData.busyMemory += chunkSize;
    return p;
    }
    return 0;
}


static PRBool allocationEnabled = PR_TRUE;

PR_IMPLEMENT(void) PR_EnableAllocation(PRBool yesOrNo)
{
    allocationEnabled = yesOrNo;
}

static void CollectorCleanup(void) {
    while (collectorCleanupNeeded) {
    LOCK_GC();
    collectorCleanupNeeded = 0;
    UNLOCK_GC();
    if (freeSegs) {
        FreeSegments();
    }
    if (!WEAK_FREELIST_ISEMPTY()) {
        EmptyWeakFreeList();
    }
    }
}



#ifdef GC_CHECK
static PRInt32 allocationCount;

static void EarthShatteringKaBoom(PRInt32 whichOne) {
    long* p = 0;
    *p = 0;
}



static void CheckSegment(GCSeg* sp) {
    PRWord h, tix;
    PRWord *p, *lastp, *np, *limit;

    lastp = p = (PRWord *) sp->base;
    limit = (PRWord *) sp->limit;
    while (p < limit) {
    if (IS_HBIT(sp, p)) {
	    char *cp, i;
	    GCBlockEnd* end;
	    PRWord bytes, requestedBytes;

	    h = p[0];
	    tix = GET_TYPEIX(h);
	    bytes = OBJ_BYTES(h);
	    np = (PRWord *) ((char *)p + bytes);
	    if (tix != FREE_MEMORY_TYPEIX) {
                PRInt32 test;	
		

		end = (GCBlockEnd*)((char*)(p) + bytes - sizeof(GCBlockEnd));
		test = end->check;
		if (test != PR_BLOCK_END) {
		    PR_ASSERT(test == PR_BLOCK_END);
		}
		requestedBytes = end->requestedBytes;
		if (requestedBytes >= bytes) EarthShatteringKaBoom(0);
		cp = (char*)(p + 1) + requestedBytes;
		i = (char) 0xff;
		while (cp < (char*)end) {
            if (*cp != i) EarthShatteringKaBoom(1);
            cp++;
            i--;
        }
        }
        lastp = p;
        p = np;
    } else {
        
        GCFreeChunk *cp = (GCFreeChunk*) p;
        if ((PRInt32)cp->chunkSize < (PRInt32)sizeof(GCFreeChunk)) {
            EarthShatteringKaBoom(3);
        }
        lastp = p;
        p = (PRWord*) ((char*)p + cp->chunkSize);
    }
    }
}

static void CheckHeap(void) {
    GCSeg *sp = segs;
    GCSeg *esp = sp + nsegs;
    while (sp < esp) {
    CheckSegment(sp);
    sp++;
    }
}

#endif 



#ifdef DEBUG
long gc_thrash = -1L;
#endif






PR_IMPLEMENT(PRWord GCPTR *)PR_AllocMemory(
    PRWord requestedBytes, PRInt32 tix, PRWord flags)
{
    PRWord *p;
    CollectorType *ct;
    PRInt32 bytes;
    GCFinal *final = 0;
    GCWeak *weak = 0;
    int dub = flags & PR_ALLOC_DOUBLE;
    PRInt32 objBytes;
#ifdef GC_STATS
    PRInt64 allocTime, ldelta;
#endif

    if (!allocationEnabled) return NULL;

    PR_ASSERT(requestedBytes >= 0);
    PR_ASSERT(_pr_collectorTypes[tix].flags != 0);

#ifdef DEBUG
    if (_pr_do_a_dump) {
    



    PR_GC();
    PR_Sleep(PR_MicrosecondsToInterval(1000000L));
    PR_GC();
    PR_DumpGCHeap(_pr_dump_file, PR_TRUE);
    _pr_do_a_dump = 0;
    }
#endif

#ifdef GC_STATS
    allocTime = PR_Now();
#endif
    bytes = (PRInt32) requestedBytes;

    





    
    if ((MAX_INT - PR_BYTES_PER_WORD) < bytes ) return NULL;
    bytes = (bytes + PR_BYTES_PER_WORD - 1) >> PR_BYTES_PER_WORD_LOG2;
    bytes <<= PR_BYTES_PER_WORD_LOG2;
    
    if ((MAX_INT - sizeof(PRWord)) < bytes ) return NULL;
    bytes += sizeof(PRWord);
    





#ifndef IS_64
    if (dub) {
        
        if ((MAX_INT - PR_BYTES_PER_WORD) < bytes ) return NULL;
        bytes += PR_BYTES_PER_WORD;
    }
#endif

#ifdef GC_CHECK
    if (_pr_gcData.flags & GC_CHECK) {
        

        
        if ((MAX_INT - PR_BYTES_PER_WORD * 3) < bytes ) return NULL;
        bytes += PR_BYTES_PER_WORD * 3;
    }
#endif

#if defined(GC_CHECK) || defined(GC_STATS) || defined(GC_TRACEROOTS)
    if ((MAX_INT - sizeof(GCBlockEnd)) < bytes ) return NULL;
    bytes += sizeof(GCBlockEnd);
#endif

    PR_ASSERT( bytes < MAX_ALLOC_SIZE );
    



    if (bytes >= MAX_ALLOC_SIZE) return NULL;

#ifdef DEBUG
    if (gc_thrash == -1L ? (gc_thrash = (long)PR_GetEnv("GC_THRASH")):gc_thrash) PR_GC();
#endif

    ct = &_pr_collectorTypes[tix];
    if (ct->flags & (_GC_TYPE_FINAL|_GC_TYPE_WEAK)) {
    if (0 != ct->gctype.finalize) {
        



        final = AllocFinalNode();
        if (!final) {
        
		PR_ASSERT(0);
        return 0;
        }
    }
    if (0 != ct->gctype.getWeakLinkOffset) {
        



        weak = AllocWeakNode();
        if (!weak) {
        
        if (0 != final) {
            FreeFinalNode(final);
        }
		PR_ASSERT(0);
        return 0;
        }
    }
    }

    LOCK_GC();
#ifdef GC_CHECK
    if (_pr_gcData.flags & GC_CHECK) CheckHeap();
    allocationCount++;
#endif

    
    if (bytes > MAX_ALLOC) goto lost;

    
    p = ((bytes >= BIG_ALLOC) && (nsegs < MAX_SEGS)) ?
        BigAlloc(tix, bytes, dub) : BinAlloc(tix, bytes, dub);
    if (0 == p) {
#ifdef GC_STATS
        LL_SUB(ldelta, PR_Now(), allocTime);
#endif
        
        _GCTRACE(GC_ALLOC, ("force GC: want %d", bytes));
        dogc();
        PR_ASSERT( GC_IS_LOCKED() );

        




        if ((_pr_gcData.allocMemory < _pr_gcData.maxMemory)
        && ((_pr_gcData.freeMemory <
            ((_pr_gcData.allocMemory * MIN_FREE_THRESHOLD_AFTER_GC) / 100L))
        || (_pr_gcData.freeMemory < bytes))) {
            GrowHeap(PR_MAX(bytes, segmentSize));
        }
#ifdef GC_STATS
        LL_ADD(allocTime, PR_Now(), ldelta);
#endif

        
        p = ((bytes >= BIG_ALLOC) && (nsegs < MAX_SEGS)) ?
            BigAlloc(tix, bytes, dub) : BinAlloc(tix, bytes, dub);
        if (0 == p) {
            
            if (!GrowHeap(PR_MAX(bytes, segmentSize))) goto lost;
            p = BinAlloc(tix, bytes, dub);
            if (0 == p) goto lost;
        }
    }

    


    objBytes = OBJ_BYTES(p[0]);
    if (objBytes > sizeof(PRWord)) p[1] = 0;
    if (objBytes > sizeof(PRWord)*2) p[2] = 0;

    if (final) {
	_GCTRACE(GC_ALLOC, ("alloc 0x%x (%d) final=0x%x",
                p, bytes, final));
    final->object = p;
    PR_APPEND_LINK(&final->links, &_pr_finalizeableObjects);
    } else {
	_GCTRACE(GC_ALLOC, ("alloc 0x%x (%d)", p, bytes));
    }
    if (weak) {
    weak->object = p;
    PR_APPEND_LINK(&weak->links, &_pr_weakLinks);
    }
    METER(meter.allocBytes += bytes);
    METER(meter.wastedBytes += (bytes - requestedBytes));
    UNLOCK_GC();

    if (collectorCleanupNeeded) {
	CollectorCleanup();
    }

#if defined(GC_CHECK) || defined(GC_STATS) || defined(GC_TRACEROOTS)
    {
	GCBlockEnd* end = (GCBlockEnd*)((char*)p + OBJ_BYTES(p[0]) - sizeof(GCBlockEnd));
	end->check = PR_BLOCK_END;
    }
#endif
#ifdef GC_STATS
    {
	PRInt64 now = PR_Now();
	double delta;
	PRInt32 bin;
	GCBlockEnd* end = (GCBlockEnd*)((char*)p + OBJ_BYTES(p[0]) - sizeof(GCBlockEnd));

	end->allocTime = allocTime;
	LL_SUB(ldelta, now, allocTime);
	LL_L2D(delta, ldelta);
	InlineBinNumber(bin, requestedBytes);
	end->bin = bin;
	gcstats[bin].nallocs++;
	gcstats[bin].allocTime += delta;
	gcstats[bin].allocTimeVariance += delta * delta;
    }
#endif
#ifdef GC_CHECK
    if (_pr_gcData.flags & GC_CHECK) {
    

    char* cp = (char*)(p + 1) + requestedBytes;
	GCBlockEnd* end = (GCBlockEnd*)((char*)p + OBJ_BYTES(p[0]) - sizeof(GCBlockEnd));
	char i = (char) 0xff;
	while (cp < (char*)end) {
	    *cp++ = i--;
	}
	end->requestedBytes = requestedBytes;
	CheckHeap();
    }
#endif
    return p + 1;

  lost:
    
    UNLOCK_GC();
    if (final) {
    FreeFinalNode(final);
    }
    if (weak) {
    FreeWeakNode(weak);
    }
    if (collectorCleanupNeeded) {
    CollectorCleanup();
    }
    return 0;
}



PR_IMPLEMENT(PRWord GCPTR *)
PR_AllocSimpleMemory(PRWord requestedBytes, PRInt32 tix)
{
    PRWord *p;
    PRInt32 bytes;
    PRInt32 objBytes;
#ifdef GC_STATS
    PRInt64 allocTime, ldelta;
#endif

    if (!allocationEnabled) return NULL;

    PR_ASSERT(requestedBytes >= 0);
    PR_ASSERT(_pr_collectorTypes[tix].flags != 0);

#ifdef DEBUG
    if (_pr_do_a_dump) {
	



	PR_GC();
	PR_Sleep(PR_MicrosecondsToInterval(1000000L));
	PR_GC();
	PR_DumpGCHeap(_pr_dump_file, PR_TRUE);
	_pr_do_a_dump = 0;
    }
#endif

#ifdef GC_STATS
    allocTime = PR_NowMS();
#endif
    bytes = (PRInt32) requestedBytes;

    





    bytes = (bytes + PR_BYTES_PER_WORD - 1) >> PR_BYTES_PER_WORD_LOG2;
    bytes <<= PR_BYTES_PER_WORD_LOG2;
    bytes += sizeof(PRWord);
    
    





#ifndef IS_64
    bytes += PR_BYTES_PER_WORD;
#endif

#ifdef GC_CHECK
    if (_pr_gcData.flags & GC_CHECK) {
    

    bytes += PR_BYTES_PER_WORD * 2;
    }
#endif

#if defined(GC_CHECK) || defined(GC_STATS) || defined(GC_TRACEROOTS)
    bytes += sizeof(GCBlockEnd);
#endif

    
    





    if (bytes >= MAX_ALLOC_SIZE) {
        return NULL;
    }
#ifdef DEBUG
    if (gc_thrash == -1L
	? (gc_thrash = (long)PR_GetEnv("GC_THRASH"))
	: gc_thrash) {
	PR_GC();
    }
#endif

    LOCK_GC();
#ifdef GC_CHECK
    if (_pr_gcData.flags & GC_CHECK) {
    CheckHeap();
    }
    allocationCount++;
#endif

    
    if ((bytes >= BIG_ALLOC) && (nsegs < MAX_SEGS)) {
    p = BigAlloc(tix, bytes, 1);
    } else {
    p = BinAlloc(tix, bytes, 1);
    }
    if (0 == p) {
#ifdef GC_STATS
      LL_SUB(ldelta, PR_Now(), allocTime);
#endif
      
      _GCTRACE(GC_ALLOC, ("force GC: want %d", bytes));
      dogc();
      PR_ASSERT( GC_IS_LOCKED() );

      




      if ((_pr_gcData.allocMemory < _pr_gcData.maxMemory) &&
      (_pr_gcData.freeMemory <
       ((_pr_gcData.allocMemory * MIN_FREE_THRESHOLD_AFTER_GC) / 100L))) {
    GrowHeap(PR_MAX(bytes, segmentSize));
      }
#ifdef GC_STATS
      LL_ADD(allocTime, PR_Now(), ldelta);
#endif

      
      if ((bytes >= BIG_ALLOC) && (nsegs < MAX_SEGS)) {
    p = BigAlloc(tix, bytes, 1);
      } else {
    p = BinAlloc(tix, bytes, 1);
      }
      if (0 == p) {
    
    if (!GrowHeap(PR_MAX(bytes, segmentSize))) {
      goto lost;
    }
    p = BinAlloc(tix, bytes, 1);
    if (0 == p) goto lost;
      }
    }

    


    objBytes = OBJ_BYTES(p[0]);
    if (objBytes > sizeof(PRWord)) p[1] = 0;
    if (objBytes > sizeof(PRWord)*2) p[2] = 0;

    METER(meter.allocBytes += bytes);
    METER(meter.wastedBytes += (bytes - requestedBytes));
    UNLOCK_GC();

    if (collectorCleanupNeeded) {
	CollectorCleanup();
    }

#if defined(GC_CHECK) || defined(GC_STATS) || defined(GC_TRACEROOTS)
    {
	GCBlockEnd* end = (GCBlockEnd*)((char*)p + OBJ_BYTES(p[0]) - sizeof(GCBlockEnd));
	end->check = PR_BLOCK_END;
    }
#endif
#ifdef GC_STATS
    {
	PRInt64 now = PR_Now();
	double delta;
	PRInt32 bin;
	GCBlockEnd* end = (GCBlockEnd*)((char*)p + OBJ_BYTES(p[0]) - sizeof(GCBlockEnd));

	end->allocTime = allocTime;
	LL_SUB(ldelta, now, allocTime);
	LL_L2D(delta, ldelta);
	InlineBinNumber(bin, requestedBytes);
	end->bin = bin;
	gcstats[bin].nallocs++;
	gcstats[bin].allocTime += delta;
	gcstats[bin].allocTimeVariance += delta * delta;
    }
#endif
#ifdef GC_CHECK
    if (_pr_gcData.flags & GC_CHECK) {
    

    char* cp = (char*)(p + 1) + requestedBytes;
	GCBlockEnd* end = (GCBlockEnd*)((char*)p + OBJ_BYTES(p[0]) - sizeof(GCBlockEnd));
	char i = (char) 0xff;
	while (cp < (char*)end) {
	    *cp++ = i--;
	}
	end->requestedBytes = requestedBytes;
	CheckHeap();
    }
#endif
    return p + 1;

  lost:
    
    UNLOCK_GC();
    if (collectorCleanupNeeded) {
    CollectorCleanup();
    }
    return 0;
}



PR_IMPLEMENT(PRWord) PR_GetObjectHeader(void *ptr) {
    GCSeg *sp;
    PRWord *h;

    if (ptr == 0) return 0;
    sp = InHeap(ptr);
    if (sp == 0) return 0;
    h = (PRWord*)FindObject(sp, (PRWord*)ptr);
    return GC_GET_USER_BITS(h[0]);
}

PR_IMPLEMENT(PRWord) PR_SetObjectHeader(void *ptr, PRWord newUserBits) {
    GCSeg *sp;
    PRWord *h, rv;

    if (ptr == 0) return 0;
    sp = InHeap(ptr);
    if (sp == 0) return 0;
    h = (PRWord*)FindObject(sp, (PRWord*)ptr);
    rv = GC_GET_USER_BITS(h[0]);
    h[0] = (h[0] & ~GC_USER_BITS) |
    ((newUserBits << GC_USER_BITS_SHIFT) & GC_USER_BITS);
    return rv;
}

PR_IMPLEMENT(void) PR_InitGC(
    PRWord flags, PRInt32 initialHeapSize, PRInt32 segSize, PRThreadScope scope)
{
    static char firstTime = 1;

    if (!firstTime) return;
    firstTime = 0;

    _pr_msgc_lm = PR_NewLogModule("msgc");
    _pr_pageShift = PR_GetPageShift();
    _pr_pageSize = PR_GetPageSize();

  
  if (0 != segSize) segmentSize = segSize;
#ifdef DEBUG
    GC = PR_NewLogModule("GC");
    {
    char *ev = PR_GetEnv("GC_SEGMENT_SIZE");
    if (ev && ev[0]) {
      PRInt32 newSegmentSize = atoi(ev);
      if (0 != newSegmentSize) segmentSize = newSegmentSize;
    }
    ev = PR_GetEnv("GC_INITIAL_HEAP_SIZE");
    if (ev && ev[0]) {
      PRInt32 newInitialHeapSize = atoi(ev);
      if (0 != newInitialHeapSize) initialHeapSize = newInitialHeapSize;
    }
    ev = PR_GetEnv("GC_FLAGS");
    if (ev && ev[0]) {
        flags |= atoi(ev);
    }
#ifdef GCMETER
        ev = PR_GetEnv("GC_METER");
        if (ev && ev[0]) {
            _pr_gcMeter = atoi(ev);
        }
#endif
    }
#endif
  if (0 == initialHeapSize) initialHeapSize = segmentSize;
  if (initialHeapSize < segmentSize) initialHeapSize = segmentSize;

  _pr_gcData.maxMemory   = MAX_SEGS * segmentSize;
  _pr_gcData.liveBlock  = ProcessRootBlock;
  _pr_gcData.livePointer = ProcessRootPointer;
  _pr_gcData.processRootBlock  = ProcessRootBlock;
  _pr_gcData.processRootPointer = ProcessRootPointer;
  _pr_gcData.dumpOutput = NULL;

  PR_INIT_CLIST(&_pr_finalizeableObjects);
    PR_INIT_CLIST(&_pr_finalQueue);
    _PR_InitGC(flags);

    
    _PR_CreateFinalizer(scope);

  
  minBin = 31;
  maxBin = 0;
  GrowHeap(initialHeapSize);
    PR_RegisterRootFinder(ScanWeakFreeList, "scan weak free list", 0);
}




#ifdef DEBUG

static int SegmentOverlaps(int i, int j)
{
  return
    (((segs[i].limit > segs[j].base) && (segs[i].base < segs[j].base)) ||
     ((segs[j].limit > segs[i].base) && (segs[j].base < segs[i].base)));
}

static void NoSegmentOverlaps(void)
{
  int i,j;

  for (i = 0; i < nsegs; i++)
    for (j = i+1 ; j < nsegs ; j++)
      PR_ASSERT(!SegmentOverlaps(i,j));
}

static void SegInfoCheck(void)
{
  int i;
  for (i = 0 ; i < nsegs ; i++)
    PR_ASSERT((segs[i].info->hbits) &&
	      (segs[i].info->hbits == segs[i].hbits) &&
	      (segs[i].info->base == segs[i].base) &&
	      (segs[i].info->limit == segs[i].limit));
}

static void SanityCheckGC()
{
  NoSegmentOverlaps();
  SegInfoCheck();
}

#endif

#if defined(DEBUG) && defined(WIN32)

extern void *baseaddr;
extern void *lastaddr;

PR_IMPLEMENT(void)
PR_PrintGCStats(void)
{
    long reportedSegSpace = _pr_gcData.busyMemory + _pr_gcData.freeMemory;
    char* msg;
    long largeCount = 0, largeSize = 0;
    long segCount = 0, segSize = 0;
    long freeCount = 0, freeSize = 0;
    GCSeg *sp, *esp;
    GCSegInfo* si;

    LOCK_GC();

    sp = segs;
    esp = sp + nsegs;
    while (sp < esp) {
    long size = sp->info->limit - sp->info->base;
    segCount++;
    segSize += size;
        if (sp->info->fromMalloc) {
        largeCount++;
        largeSize += size;
    }
        sp++;
    }

    si = freeSegs;
    while (si != NULL) {
    long size = si->limit - si->base;
    freeCount++;
    freeSize += size;
    si = si->next;
    }
    
    msg = PR_smprintf("\
# GC Stats:\n\
#   vm space:\n\
#     range:      %ld - %ld\n\
#     size:       %ld\n\
#   segments:\n\
#     range:      %ld - %ld\n\
#     count:      %ld (reported: %ld)\n\
#     size:       %ld (reported: %ld)\n\
#     free count: %ld\n\
#     free size:  %ld\n\
#     busy objs:  %ld (%ld%%)\n\
#     free objs:  %ld (%ld%%)\n\
#   large blocks:\n\
#     count:      %ld\n\
#     total size: %ld (%ld%%)\n\
#     avg size:   %ld\n\
",
              
              (long)baseaddr, (long)lastaddr,
              (long)lastaddr - (long)baseaddr,
              
              _pr_gcData.lowSeg, _pr_gcData.highSeg,
              segCount, nsegs,
              segSize, reportedSegSpace,
              freeCount,
              freeSize,
              _pr_gcData.busyMemory,
              (_pr_gcData.busyMemory * 100 / reportedSegSpace),
              _pr_gcData.freeMemory,
              (_pr_gcData.freeMemory * 100 / reportedSegSpace),
              
              largeCount,
              largeSize, (largeSize * 100 / reportedSegSpace),
              (largeCount ? largeSize / largeCount : 0)
              );
    UNLOCK_GC();
    fprintf(stderr, msg);
    OutputDebugString(msg);
    PR_smprintf_free(msg);
#ifdef GC_STATS
    PR_PrintGCAllocStats();
#endif
}
#endif

PR_IMPLEMENT(void)
PR_DumpToFile(char* filename, char* msg, PRFileDumper dump, PRBool detailed)
{
    FILE *out;
    OutputDebugString(msg);
    out = fopen(filename, "a");
    if (!out) {
	char buf[64];
	PR_ASSERT(strlen(filename) < sizeof(buf) - 16);
	PR_snprintf(buf, sizeof(buf), "Can't open \"%s\"\n",
		    filename);
	OutputDebugString(buf);
    }
    else
    {
	struct tm *newtime;
	time_t aclock;
	int i;

	time(&aclock);
	newtime = localtime(&aclock);
	fprintf(out, "%s on %s\n", msg, asctime(newtime));  
	dump(out, detailed);
	fprintf(out, "\n\n");
	for (i = 0; i < 80; i++)
	    fprintf(out, "=");
	fprintf(out, "\n\n");
	fclose(out);
    }
    OutputDebugString(" done\n");
}

