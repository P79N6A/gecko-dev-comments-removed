#define USE_MALLOC_LOCK
#define DEFAULT_TRIM_THRESHOLD (256 * 1024)































































































































































































































































#include "xpcom-private.h"










#ifdef WIN32

#include <windows.h>


#define LACKS_UNISTD_H
#define LACKS_SYS_PARAM_H
#define LACKS_SYS_MMAN_H


#define MORECORE sbrk
#define MORECORE_CONTIGUOUS 1
#define MORECORE_FAILURE    ((void*)(-1))


#define HAVE_MMAP 1
#define MUNMAP_FAILURE  (-1)

#define MAP_PRIVATE 1
#define MAP_ANONYMOUS 2
#define PROT_READ 1
#define PROT_WRITE 2




#ifdef USE_MALLOC_LOCK
static int slwait(int *sl);
static int slrelease(int *sl);
#endif

static long getpagesize(void);
static long getregionsize(void);
static void *sbrk(long size);
static void *mmap(void *ptr, long size, long prot, long type, long handle, long arg);
static long munmap(void *ptr, long size);

static void vminfo (unsigned long *free, unsigned long *reserved, unsigned long *committed);
static int cpuinfo (int whole, unsigned long *kernel, unsigned long *user);

#endif









#ifndef __STD_C
#ifdef __STDC__
#define __STD_C     1
#else
#if __cplusplus
#define __STD_C     1
#else
#define __STD_C     0
#endif 
#endif 
#endif 






#ifndef Void_t
#if (__STD_C || defined(WIN32))
#define Void_t      void
#else
#define Void_t      char
#endif
#endif 

#if __STD_C
#include <stddef.h>   
#else
#include <sys/types.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif





#ifndef LACKS_UNISTD_H
#include <unistd.h>
#endif






#include <stdio.h>    
#include <errno.h>    


























#if DEBUG
#include <assert.h>
#else
#define assert(x) ((void)0)
#endif




























#ifndef INTERNAL_SIZE_T
#define INTERNAL_SIZE_T size_t
#endif


#define SIZE_SZ                (sizeof(INTERNAL_SIZE_T))











  

#ifndef MALLOC_ALIGNMENT
#define MALLOC_ALIGNMENT       (2 * SIZE_SZ)
#endif


#define MALLOC_ALIGN_MASK      (MALLOC_ALIGNMENT - 1)









































#ifdef USE_MALLOC_LOCK
#define USE_PUBLIC_MALLOC_WRAPPERS
#else

#endif


















#define HAVE_MEMCPY

#ifndef USE_MEMCPY
#ifdef HAVE_MEMCPY
#define USE_MEMCPY 1
#else
#define USE_MEMCPY 0
#endif
#endif


#if (__STD_C || defined(HAVE_MEMCPY))

#ifdef WIN32
  



#else
#if __STD_C
void* memset(void*, int, size_t);
void* memcpy(void*, const void*, size_t);
void* memmove(void*, const void*, size_t);
#else
Void_t* memset();
Void_t* memcpy();
Void_t* memmove();
#endif
#endif
#endif










#ifndef MALLOC_FAILURE_ACTION
#if __STD_C
#define MALLOC_FAILURE_ACTION \
   errno = ENOMEM;

#else

#define MALLOC_FAILURE_ACTION
#endif
#endif













#ifndef HAVE_MMAP
#define HAVE_MMAP 1
#endif















#ifndef MMAP_AS_MORECORE_SIZE
#define MMAP_AS_MORECORE_SIZE (1024 * 1024)
#endif









#ifndef HAVE_MREMAP
#ifdef linux
#define HAVE_MREMAP 1
#else
#define HAVE_MREMAP 0
#endif

#endif 




























#ifdef HAVE_USR_INCLUDE_MALLOC_H
#include "/usr/include/malloc.h"
#else



struct mallinfo {
  int arena;    
  int ordblks;  
  int smblks;   
  int hblks;    
  int hblkhd;   
  int usmblks;  
  int fsmblks;  
  int uordblks; 
  int fordblks; 
  int keepcost; 
};



#define M_MXFAST  1    /* Set maximum fastbin size */
#define M_NLBLKS  2    /* UNUSED in this malloc */
#define M_GRAIN   3    /* UNUSED in this malloc */
#define M_KEEP    4    /* UNUSED in this malloc */


#endif




#ifndef M_TRIM_THRESHOLD
#define M_TRIM_THRESHOLD    -1
#endif

#ifndef M_TOP_PAD
#define M_TOP_PAD           -2
#endif

#ifndef M_MMAP_THRESHOLD
#define M_MMAP_THRESHOLD    -3
#endif

#ifndef M_MMAP_MAX
#define M_MMAP_MAX          -4
#endif




























#ifndef DEFAULT_MXFAST
#define DEFAULT_MXFAST     64
#endif




























































#ifndef DEFAULT_TRIM_THRESHOLD
#define DEFAULT_TRIM_THRESHOLD (128 * 1024)
#endif































#ifndef DEFAULT_TOP_PAD
#define DEFAULT_TOP_PAD        (0)
#endif







































#ifndef DEFAULT_MMAP_THRESHOLD
#define DEFAULT_MMAP_THRESHOLD (128 * 1024)
#endif




















#ifndef DEFAULT_MMAP_MAX
#if HAVE_MMAP
#define DEFAULT_MMAP_MAX       (256)
#else
#define DEFAULT_MMAP_MAX       (0)
#endif
#endif




















#ifndef TRIM_FASTBINS
#define TRIM_FASTBINS  0
#endif







#ifdef LACKS_UNISTD_H
#if !defined(__FreeBSD__) && !defined(__OpenBSD__) && !defined(__NetBSD__)
#if __STD_C
extern Void_t*     sbrk(ptrdiff_t);
#else
extern Void_t*     sbrk();
#endif
#endif
#endif








#ifndef MORECORE
#define MORECORE sbrk
#endif









#ifndef MORECORE_FAILURE
#define MORECORE_FAILURE (-1)
#endif












#ifndef MORECORE_CONTIGUOUS
#define MORECORE_CONTIGUOUS 1
#endif














#ifndef malloc_getpagesize

#ifndef LACKS_UNISTD_H
#  include <unistd.h>
#endif

#  ifdef _SC_PAGESIZE         
#    ifndef _SC_PAGE_SIZE
#      define _SC_PAGE_SIZE _SC_PAGESIZE
#    endif
#  endif

#  ifdef _SC_PAGE_SIZE
#    define malloc_getpagesize sysconf(_SC_PAGE_SIZE)
#  else
#    if defined(BSD) || defined(DGUX) || defined(HAVE_GETPAGESIZE)
       extern size_t getpagesize();
#      define malloc_getpagesize getpagesize()
#    else
#      ifdef WIN32 
#        define malloc_getpagesize getpagesize() 
#      else
#        ifndef LACKS_SYS_PARAM_H
#          include <sys/param.h>
#        endif
#        ifdef EXEC_PAGESIZE
#          define malloc_getpagesize EXEC_PAGESIZE
#        else
#          ifdef NBPG
#            ifndef CLSIZE
#              define malloc_getpagesize NBPG
#            else
#              define malloc_getpagesize (NBPG * CLSIZE)
#            endif
#          else
#            ifdef NBPC
#              define malloc_getpagesize NBPC
#            else
#              ifdef PAGESIZE
#                define malloc_getpagesize PAGESIZE
#              else 
#                define malloc_getpagesize (4096) 
#              endif
#            endif
#          endif
#        endif
#      endif
#    endif
#  endif
#endif




#ifndef USE_PUBLIC_MALLOC_WRAPPERS
#define cALLOc      public_cALLOc
#define fREe        public_fREe
#define cFREe       public_cFREe
#define mALLOc      public_mALLOc
#define mEMALIGn    public_mEMALIGn
#define rEALLOc     public_rEALLOc
#define vALLOc      public_vALLOc
#define pVALLOc     public_pVALLOc
#define mALLINFo    public_mALLINFo
#define mALLOPt     public_mALLOPt
#define mTRIm       public_mTRIm
#define mSTATs      public_mSTATs
#define mUSABLe     public_mUSABLe
#endif

#ifdef USE_DL_PREFIX
#define public_cALLOc    dlcalloc
#define public_fREe      dlfree
#define public_cFREe     dlcfree
#define public_mALLOc    dlmalloc
#define public_mEMALIGn  dlmemalign
#define public_rEALLOc   dlrealloc
#define public_vALLOc    dlvalloc
#define public_pVALLOc   dlpvalloc
#define public_mALLINFo  dlmallinfo
#define public_mALLOPt   dlmallopt
#define public_mTRIm     dlmalloc_trim
#define public_mSTATs    dlmalloc_stats
#define public_mUSABLe   dlmalloc_usable_size
#else 
#define public_cALLOc    calloc
#define public_fREe      free
#define public_cFREe     cfree
#define public_mALLOc    malloc
#define public_mEMALIGn  memalign
#define public_rEALLOc   realloc
#define public_vALLOc    valloc
#define public_pVALLOc   pvalloc
#define public_mALLINFo  mallinfo
#define public_mALLOPt   mallopt
#define public_mTRIm     malloc_trim
#define public_mSTATs    malloc_stats
#define public_mUSABLe   malloc_usable_size
#endif 

#if __STD_C

Void_t* public_mALLOc(size_t);
void    public_fREe(Void_t*);
Void_t* public_rEALLOc(Void_t*, size_t);
Void_t* public_mEMALIGn(size_t, size_t);
Void_t* public_vALLOc(size_t);
Void_t* public_pVALLOc(size_t);
Void_t* public_cALLOc(size_t, size_t);
void    public_cFREe(Void_t*);
int     public_mTRIm(size_t);
size_t  public_mUSABLe(Void_t*);
void    public_mSTATs();
int     public_mALLOPt(int, int);
struct mallinfo public_mALLINFo(void);
#else
Void_t* public_mALLOc();
void    public_fREe();
Void_t* public_rEALLOc();
Void_t* public_mEMALIGn();
Void_t* public_vALLOc();
Void_t* public_pVALLOc();
Void_t* public_cALLOc();
void    public_cFREe();
int     public_mTRIm();
size_t  public_mUSABLe();
void    public_mSTATs();
int     public_mALLOPt();
struct mallinfo public_mALLINFo();
#endif


#ifdef __cplusplus
};  
#endif











#ifdef USE_PUBLIC_MALLOC_WRAPPERS
#if __STD_C

static Void_t* mALLOc(size_t);
static void    fREe(Void_t*);
static Void_t* rEALLOc(Void_t*, size_t);
static Void_t* mEMALIGn(size_t, size_t);
static Void_t* vALLOc(size_t);
static Void_t* pVALLOc(size_t);
static Void_t* cALLOc(size_t, size_t);
static void    cFREe(Void_t*);
static int     mTRIm(size_t);
static size_t  mUSABLe(Void_t*);
static void    mSTATs();
static int     mALLOPt(int, int);
static struct mallinfo mALLINFo(void);
#else
static Void_t* mALLOc();
static void    fREe();
static Void_t* rEALLOc();
static Void_t* mEMALIGn();
static Void_t* vALLOc();
static Void_t* pVALLOc();
static Void_t* cALLOc();
static void    cFREe();
static int     mTRIm();
static size_t  mUSABLe();
static void    mSTATs();
static int     mALLOPt();
static struct mallinfo mALLINFo();
#endif
#endif





#ifdef USE_PUBLIC_MALLOC_WRAPPERS










#ifdef USE_MALLOC_LOCK

#ifdef WIN32

static int mALLOC_MUTEx;

#define MALLOC_PREACTION   slwait(&mALLOC_MUTEx)
#define MALLOC_POSTACTION  slrelease(&mALLOC_MUTEx)

#else

#include <pthread.h>

static pthread_mutex_t mALLOC_MUTEx = PTHREAD_MUTEX_INITIALIZER;

#define MALLOC_PREACTION   pthread_mutex_lock(&mALLOC_MUTEx)
#define MALLOC_POSTACTION  pthread_mutex_unlock(&mALLOC_MUTEx)

#endif 

#else



#define MALLOC_PREACTION   (0)
#define MALLOC_POSTACTION  (0)

#endif

Void_t* public_mALLOc(size_t bytes) {
  Void_t* m;
  if (MALLOC_PREACTION != 0) {
    return 0;
  }
  m = mALLOc(bytes);
  if (MALLOC_POSTACTION != 0) {
  }
  return m;
}

void public_fREe(Void_t* m) {
  if (MALLOC_PREACTION != 0) {
    return;
  }
  fREe(m);
  if (MALLOC_POSTACTION != 0) {
  }
}

Void_t* public_rEALLOc(Void_t* m, size_t bytes) {
  if (MALLOC_PREACTION != 0) {
    return 0;
  }
  m = rEALLOc(m, bytes);
  if (MALLOC_POSTACTION != 0) {
  }
  return m;
}

Void_t* public_mEMALIGn(size_t alignment, size_t bytes) {
  Void_t* m;
  if (MALLOC_PREACTION != 0) {
    return 0;
  }
  m = mEMALIGn(alignment, bytes);
  if (MALLOC_POSTACTION != 0) {
  }
  return m;
}

Void_t* public_vALLOc(size_t bytes) {
  Void_t* m;
  if (MALLOC_PREACTION != 0) {
    return 0;
  }
  m = vALLOc(bytes);
  if (MALLOC_POSTACTION != 0) {
  }
  return m;
}

Void_t* public_pVALLOc(size_t bytes) {
  Void_t* m;
  if (MALLOC_PREACTION != 0) {
    return 0;
  }
  m = pVALLOc(bytes);
  if (MALLOC_POSTACTION != 0) {
  }
  return m;
}

Void_t* public_cALLOc(size_t n, size_t elem_size) {
  Void_t* m;
  if (MALLOC_PREACTION != 0) {
    return 0;
  }
  m = cALLOc(n, elem_size);
  if (MALLOC_POSTACTION != 0) {
  }
  return m;
}

void public_cFREe(Void_t* m) {
  if (MALLOC_PREACTION != 0) {
    return;
  }
  cFREe(m);
  if (MALLOC_POSTACTION != 0) {
  }
}

int public_mTRIm(size_t s) {
  int result;
  if (MALLOC_PREACTION != 0) {
    return 0;
  }
  result = mTRIm(s);
  if (MALLOC_POSTACTION != 0) {
  }
  return result;
}


size_t public_mUSABLe(Void_t* m) {
  size_t result;
  if (MALLOC_PREACTION != 0) {
    return 0;
  }
  result = mUSABLe(m);
  if (MALLOC_POSTACTION != 0) {
  }
  return result;
}


void public_mSTATs() {
  if (MALLOC_PREACTION != 0) {
    return;
  }
  mSTATs();
  if (MALLOC_POSTACTION != 0) {
  }
}

struct mallinfo public_mALLINFo() {
  struct mallinfo m;
  if (MALLOC_PREACTION != 0) {
    return m;
  }
  m = mALLINFo();
  if (MALLOC_POSTACTION != 0) {
  }
  return m;
}

int public_mALLOPt(int p, int v) {
  int result;
  if (MALLOC_PREACTION != 0) {
    return 0;
  }
  result = mALLOPt(p, v);
  if (MALLOC_POSTACTION != 0) {
  }
  return result;
}

#endif






#if USE_MEMCPY

#define MALLOC_COPY(dest, src, nbytes, overlap) \
 ((overlap) ? memmove(dest, src, nbytes) : memcpy(dest, src, nbytes))
#define MALLOC_ZERO(dest, nbytes)       memset(dest, 0,   nbytes)

#else 



#define MALLOC_ZERO(charp, nbytes)                                            \
do {                                                                          \
  INTERNAL_SIZE_T* mzp = (INTERNAL_SIZE_T*)(charp);                           \
  long mctmp = (nbytes)/sizeof(INTERNAL_SIZE_T), mcn;                         \
  if (mctmp < 8) mcn = 0; else { mcn = (mctmp-1)/8; mctmp %= 8; }             \
  switch (mctmp) {                                                            \
    case 0: for(;;) { *mzp++ = 0;                                             \
    case 7:           *mzp++ = 0;                                             \
    case 6:           *mzp++ = 0;                                             \
    case 5:           *mzp++ = 0;                                             \
    case 4:           *mzp++ = 0;                                             \
    case 3:           *mzp++ = 0;                                             \
    case 2:           *mzp++ = 0;                                             \
    case 1:           *mzp++ = 0; if(mcn <= 0) break; mcn--; }                \
  }                                                                           \
} while(0)



#define MALLOC_COPY(dest,src,nbytes,overlap)                                  \
do {                                                                          \
  INTERNAL_SIZE_T* mcsrc = (INTERNAL_SIZE_T*) src;                            \
  INTERNAL_SIZE_T* mcdst = (INTERNAL_SIZE_T*) dest;                           \
  long mctmp = (nbytes)/sizeof(INTERNAL_SIZE_T), mcn;                         \
  if (mctmp < 8) mcn = 0; else { mcn = (mctmp-1)/8; mctmp %= 8; }             \
  switch (mctmp) {                                                            \
    case 0: for(;;) { *mcdst++ = *mcsrc++;                                    \
    case 7:           *mcdst++ = *mcsrc++;                                    \
    case 6:           *mcdst++ = *mcsrc++;                                    \
    case 5:           *mcdst++ = *mcsrc++;                                    \
    case 4:           *mcdst++ = *mcsrc++;                                    \
    case 3:           *mcdst++ = *mcsrc++;                                    \
    case 2:           *mcdst++ = *mcsrc++;                                    \
    case 1:           *mcdst++ = *mcsrc++; if(mcn <= 0) break; mcn--; }       \
  }                                                                           \
} while(0)

#endif




#if HAVE_MMAP

#include <fcntl.h>
#ifndef LACKS_SYS_MMAN_H
#include <sys/mman.h>
#endif

#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#define MAP_ANONYMOUS MAP_ANON
#endif








#ifndef MAP_ANONYMOUS

static int dev_zero_fd = -1; 

#define MMAP(addr, size, prot, flags) ((dev_zero_fd < 0) ? \
 (dev_zero_fd = open("/dev/zero", O_RDWR), \
  mmap((addr), (size), (prot), (flags), dev_zero_fd, 0)) : \
   mmap((addr), (size), (prot), (flags), dev_zero_fd, 0))

#else

#define MMAP(addr, size, prot, flags) \
 (mmap((addr), (size), (prot), (flags)|MAP_ANONYMOUS, -1, 0))

#endif

#endif 























































































































































struct malloc_chunk {

  INTERNAL_SIZE_T      prev_size;  
  INTERNAL_SIZE_T      size;       

  struct malloc_chunk* fd;         
  struct malloc_chunk* bk;
};


typedef struct malloc_chunk* mchunkptr;
































































































#define chunk2mem(p)   ((Void_t*)((char*)(p) + 2*SIZE_SZ))
#define mem2chunk(mem) ((mchunkptr)((char*)(mem) - 2*SIZE_SZ))


#define MIN_CHUNK_SIZE        (sizeof(struct malloc_chunk))



#define MINSIZE   ((MIN_CHUNK_SIZE+MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK)



#define aligned_OK(m)  (((unsigned long)((m)) & (MALLOC_ALIGN_MASK)) == 0)







#define IS_NEGATIVE(x) \
  ((unsigned long)x >= \
   (unsigned long)((((INTERNAL_SIZE_T)(1)) << ((SIZE_SZ)*8 - 1))))




#define request2size(req)                                        \
  (((req) + SIZE_SZ + MALLOC_ALIGN_MASK < MINSIZE)  ?            \
   MINSIZE :                                                     \
   ((req) + SIZE_SZ + MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK)









#define checked_request2size(req, sz)                                    \
   if (IS_NEGATIVE(req)) {                                               \
     MALLOC_FAILURE_ACTION;                                              \
     return 0;                                                           \
   }                                                                     \
   (sz) = request2size(req);











#define PREV_INUSE 0x1




#define IS_MMAPPED 0x2




#define SIZE_BITS (PREV_INUSE|IS_MMAPPED)




#define next_chunk(p) ((mchunkptr)( ((char*)(p)) + ((p)->size & ~PREV_INUSE) ))




#define prev_chunk(p) ((mchunkptr)( ((char*)(p)) - ((p)->prev_size) ))




#define chunk_at_offset(p, s)  ((mchunkptr)(((char*)(p)) + (s)))

















#define inuse(p)\
((((mchunkptr)(((char*)(p))+((p)->size & ~PREV_INUSE)))->size) & PREV_INUSE)




#define prev_inuse(p)       ((p)->size & PREV_INUSE)




#define chunk_is_mmapped(p) ((p)->size & IS_MMAPPED)




#define set_inuse(p)\
((mchunkptr)(((char*)(p)) + ((p)->size & ~PREV_INUSE)))->size |= PREV_INUSE

#define clear_inuse(p)\
((mchunkptr)(((char*)(p)) + ((p)->size & ~PREV_INUSE)))->size &= ~(PREV_INUSE)




#define inuse_bit_at_offset(p, s)\
 (((mchunkptr)(((char*)(p)) + (s)))->size & PREV_INUSE)

#define set_inuse_bit_at_offset(p, s)\
 (((mchunkptr)(((char*)(p)) + (s)))->size |= PREV_INUSE)

#define clear_inuse_bit_at_offset(p, s)\
 (((mchunkptr)(((char*)(p)) + (s)))->size &= ~(PREV_INUSE))










#define chunksize(p)         ((p)->size & ~(SIZE_BITS))



#define set_head_size(p, s)  ((p)->size = (((p)->size & PREV_INUSE) | (s)))



#define set_head(p, s)       ((p)->size = (s))



#define set_foot(p, s)       (((mchunkptr)((char*)(p) + (s)))->prev_size = (s))

























































typedef struct malloc_chunk* mbinptr;

#define NBINS        128




#define bin_at(m, i) ((mbinptr)((char*)&((m)->bins[(i)<<1]) - (SIZE_SZ<<1)))




#define next_bin(b)  ((mbinptr)((char*)(b) + (sizeof(mchunkptr)<<1)))




#define first(b)     ((b)->fd)
#define last(b)      ((b)->bk)





#define unlink(P, BK, FD) {                                            \
  FD = P->fd;                                                          \
  BK = P->bk;                                                          \
  FD->bk = BK;                                                         \
  BK->fd = FD;                                                         \
}
























#define NSMALLBINS         64
#define SMALLBIN_WIDTH      8
#define MIN_LARGE_SIZE    512

#define in_smallbin_range(sz)  ((sz) <  MIN_LARGE_SIZE)

#define smallbin_index(sz)     (((unsigned)(sz)) >> 3)

#define largebin_index(sz)                                                   \
(((((unsigned long)(sz)) >>  6) <= 32)?  56 + (((unsigned long)(sz)) >>  6): \
 ((((unsigned long)(sz)) >>  9) <= 20)?  91 + (((unsigned long)(sz)) >>  9): \
 ((((unsigned long)(sz)) >> 12) <= 10)? 110 + (((unsigned long)(sz)) >> 12): \
 ((((unsigned long)(sz)) >> 15) <=  4)? 119 + (((unsigned long)(sz)) >> 15): \
 ((((unsigned long)(sz)) >> 18) <=  2)? 124 + (((unsigned long)(sz)) >> 18): \
                                        126)

#define bin_index(sz) \
 ((in_smallbin_range(sz)) ? smallbin_index(sz) : largebin_index(sz))

















#define unsorted_chunks(M)          (bin_at(M, 1))























#define initial_top(M)              (unsorted_chunks(M))














#define BINMAPSHIFT      5
#define BITSPERMAP       (1U << BINMAPSHIFT)
#define BINMAPSIZE       (NBINS / BITSPERMAP)

#define idx2block(i)     ((i) >> BINMAPSHIFT)
#define idx2bit(i)       ((1U << ((i) & ((1U << BINMAPSHIFT)-1))))

#define mark_bin(m,i)    ((m)->binmap[idx2block(i)] |=  idx2bit(i))
#define unmark_bin(m,i)  ((m)->binmap[idx2block(i)] &= ~(idx2bit(i)))
#define get_binmap(m,i)  ((m)->binmap[idx2block(i)] &   idx2bit(i))
















typedef struct malloc_chunk* mfastbinptr;


#define fastbin_index(sz)        ((((unsigned int)(sz)) >> 3) - 2)


#define MAX_FAST_SIZE     80

#define NFASTBINS  (fastbin_index(request2size(MAX_FAST_SIZE))+1)













#define have_fastchunks(M)    (((M)->max_fast &  1U) == 0)
#define clear_fastchunks(M)   ((M)->max_fast |=  1U)
#define set_fastchunks(M)     ((M)->max_fast &= ~1U)






#define req2max_fast(s) (((((s) == 0)? SMALLBIN_WIDTH: request2size(s))) | 1U)


















#define NONCONTIGUOUS_REGIONS ((char*)(-3))







struct malloc_state {

  
  INTERNAL_SIZE_T  max_fast;   

  
  mchunkptr        top;

  
  mchunkptr        last_remainder;

  
  mfastbinptr      fastbins[NFASTBINS];

  
  mchunkptr        bins[NBINS * 2];

  
  unsigned int     binmap[BINMAPSIZE];

  
  unsigned long    trim_threshold;
  INTERNAL_SIZE_T  top_pad;
  INTERNAL_SIZE_T  mmap_threshold;

  
  int              n_mmaps;
  int              n_mmaps_max;
  int              max_n_mmaps;

  
  unsigned int     pagesize;    
  char*            sbrk_base;   

  

  INTERNAL_SIZE_T  mmapped_mem;
  INTERNAL_SIZE_T  sbrked_mem;

  INTERNAL_SIZE_T  max_sbrked_mem;
  INTERNAL_SIZE_T  max_mmapped_mem;
  INTERNAL_SIZE_T  max_total_mem;
};

typedef struct malloc_state *mstate;











static struct malloc_state av_;  











#define get_malloc_state() (&(av_))











#if __STD_C
static void malloc_init_state(mstate av)
#else
static void malloc_init_state(av) mstate av;
#endif
{
  int     i;
  mbinptr bin;

  
  
  

  
  for (i = 1; i < NBINS; ++i) { 
    bin = bin_at(av,i);
    bin->fd = bin->bk = bin;
  }

  av->max_fast       = req2max_fast(DEFAULT_MXFAST);

  av->top_pad        = DEFAULT_TOP_PAD;
  av->n_mmaps_max    = DEFAULT_MMAP_MAX;
  av->mmap_threshold = DEFAULT_MMAP_THRESHOLD;

#if MORECORE_CONTIGUOUS
  av->trim_threshold = DEFAULT_TRIM_THRESHOLD;
  av->sbrk_base      = (char*)MORECORE_FAILURE;
#else
  av->trim_threshold = (unsigned long)(-1);
  av->sbrk_base      = NONCONTIGUOUS_REGIONS;
#endif

  av->top            = initial_top(av);
  av->pagesize       = malloc_getpagesize;
}





#if __STD_C
static Void_t* sYSMALLOc(INTERNAL_SIZE_T, mstate);
static int  sYSTRIm(size_t, mstate);
static void malloc_consolidate(mstate);
#else
static Void_t* sYSMALLOc();
static int  sYSTRIm();
static void malloc_consolidate();
#endif













#if ! DEBUG

#define check_chunk(P)
#define check_free_chunk(P)
#define check_inuse_chunk(P)
#define check_remalloced_chunk(P,N)
#define check_malloced_chunk(P,N)
#define check_malloc_state()

#else
#define check_chunk(P)              do_check_chunk(P)
#define check_free_chunk(P)         do_check_free_chunk(P)
#define check_inuse_chunk(P)        do_check_inuse_chunk(P)
#define check_remalloced_chunk(P,N) do_check_remalloced_chunk(P,N)
#define check_malloced_chunk(P,N)   do_check_malloced_chunk(P,N)
#define check_malloc_state()        do_check_malloc_state()






#if __STD_C
static void do_check_chunk(mchunkptr p)
#else
static void do_check_chunk(p) mchunkptr p;
#endif
{

  mstate av = get_malloc_state();
  unsigned long sz = chunksize(p);

  if (!chunk_is_mmapped(p)) {
    
    
    if (av->sbrk_base != NONCONTIGUOUS_REGIONS) {
      assert(((char*)p) >= ((char*)(av->sbrk_base)));
    }

    if (p != av->top) {
      if (av->sbrk_base != NONCONTIGUOUS_REGIONS) {
        assert(((char*)p + sz) <= ((char*)(av->top)));
      }
    }
    else {
      if (av->sbrk_base != NONCONTIGUOUS_REGIONS) {
        assert(((char*)p + sz) <= ((char*)(av->sbrk_base) + av->sbrked_mem));
      }
      
      assert((long)(sz) >= (long)(MINSIZE));
      
      assert(prev_inuse(p));
    }
      
  }
  else {
#if HAVE_MMAP
    
    
    if (av->sbrk_base != NONCONTIGUOUS_REGIONS) { 
      assert(! (((char*)p) >= ((char*)av->sbrk_base) &&
                ((char*)p) <  ((char*)(av->sbrk_base) + av->sbrked_mem)));
    }
    
    assert(((p->prev_size + sz) & (av->pagesize-1)) == 0);
    
    assert(aligned_OK(chunk2mem(p)));
#else
    
    assert(!chunk_is_mmapped(p));
#endif
  }

}






#if __STD_C
static void do_check_free_chunk(mchunkptr p)
#else
static void do_check_free_chunk(p) mchunkptr p;
#endif
{
  mstate av = get_malloc_state();

  INTERNAL_SIZE_T sz = p->size & ~PREV_INUSE;
  mchunkptr next = chunk_at_offset(p, sz);

  do_check_chunk(p);

  
  assert(!inuse(p));
  assert (!chunk_is_mmapped(p));

  
  if ((unsigned long)sz >= (unsigned long)MINSIZE)
  {
    assert((sz & MALLOC_ALIGN_MASK) == 0);
    assert(aligned_OK(chunk2mem(p)));
    
    assert(next->prev_size == sz);
    
    assert(prev_inuse(p));
    assert (next == av->top || inuse(next));

    
    assert(p->fd->bk == p);
    assert(p->bk->fd == p);
  }
  else 
    assert(sz == SIZE_SZ);
}






#if __STD_C
static void do_check_inuse_chunk(mchunkptr p)
#else
static void do_check_inuse_chunk(p) mchunkptr p;
#endif
{
  mstate av = get_malloc_state();
  mchunkptr next;
  do_check_chunk(p);

  if (chunk_is_mmapped(p))
    return; 

  
  assert(inuse(p));

  next = next_chunk(p);

  



  if (!prev_inuse(p))  {
    
    mchunkptr prv = prev_chunk(p);
    assert(next_chunk(prv) == p);
    do_check_free_chunk(prv);
  }

  if (next == av->top) {
    assert(prev_inuse(next));
    assert(chunksize(next) >= MINSIZE);
  }
  else if (!inuse(next))
    do_check_free_chunk(next);

}





#if __STD_C
static void do_check_remalloced_chunk(mchunkptr p, INTERNAL_SIZE_T s)
#else
static void do_check_remalloced_chunk(p, s) mchunkptr p; INTERNAL_SIZE_T s;
#endif
{

  INTERNAL_SIZE_T sz = p->size & ~PREV_INUSE;

  do_check_inuse_chunk(p);

  
  assert((sz & MALLOC_ALIGN_MASK) == 0);
  assert((long)sz - (long)MINSIZE >= 0);
  assert((long)sz - (long)s >= 0);
  assert((long)sz - (long)(s + MINSIZE) < 0);

  
  assert(aligned_OK(chunk2mem(p)));

}





#if __STD_C
static void do_check_malloced_chunk(mchunkptr p, INTERNAL_SIZE_T s)
#else
static void do_check_malloced_chunk(p, s) mchunkptr p; INTERNAL_SIZE_T s;
#endif
{
  
  do_check_remalloced_chunk(p, s);

  









  assert(prev_inuse(p));
}









static void do_check_malloc_state()
{
  mstate av = get_malloc_state();
  int i;
  mchunkptr p;
  mchunkptr q;
  mbinptr b;
  unsigned int biton;
  int empty;
  unsigned int idx;
  INTERNAL_SIZE_T size;
  unsigned long total = 0;
  int max_fast_bin;


  
  assert(sizeof(INTERNAL_SIZE_T) <= sizeof(char*));

  
  assert((MALLOC_ALIGNMENT & (MALLOC_ALIGNMENT-1)) == 0);

  
  if (av->top == 0 || av->top == initial_top(av))
    return;

  
  assert((av->pagesize & (av->pagesize-1)) == 0);

  

  

  assert((av->max_fast & ~1) <= request2size(MAX_FAST_SIZE));

  max_fast_bin = fastbin_index(av->max_fast);

  for (i = 0; i < NFASTBINS; ++i) {
    p = av->fastbins[i];

    
    if (i > max_fast_bin)
      assert(p == 0);

    while (p != 0) {
      
      do_check_inuse_chunk(p);
      total += chunksize(p);
      
      assert(fastbin_index(chunksize(p)) == i);
      p = p->fd;
    }
  }

  if (total != 0)
    assert(have_fastchunks(av));

  
  for (i = 1; i < NBINS; ++i) {
    b = bin_at(av,i);

    
    if (i >= 2) {
      biton = get_binmap(av,i);
      empty = last(b) == b;
      if (!biton)
        assert(empty);
      else if (!empty)
        assert(biton);
    }

    for (p = last(b); p != b; p = p->bk) {
      
      do_check_free_chunk(p);
      size = chunksize(p);
      total += size;
      if (i >= 2) {
        
        idx = bin_index(size);
        assert(idx == i);
        
        assert(p->bk == b || chunksize(p->bk) >= chunksize(p));
      }
      
      for (q = next_chunk(p);
           q != av->top && inuse(q) && (long)(chunksize(q)) >= (long)MINSIZE;
           q = next_chunk(q))
        do_check_inuse_chunk(q);

    }
  }

  
  check_chunk(av->top);

  

  assert(total <= (unsigned long)(av->max_total_mem));
  assert(av->n_mmaps >= 0);
  assert(av->n_mmaps <= av->n_mmaps_max);
  assert(av->n_mmaps <= av->max_n_mmaps);
  assert(av->max_n_mmaps <= av->n_mmaps_max);

  assert((unsigned long)(av->sbrked_mem) <=
         (unsigned long)(av->max_sbrked_mem));

  assert((unsigned long)(av->mmapped_mem) <=
         (unsigned long)(av->max_mmapped_mem));

  assert((unsigned long)(av->max_total_mem) >=
         (unsigned long)(av->mmapped_mem) + (unsigned long)(av->sbrked_mem));

}


#endif

















#if __STD_C
static Void_t* sYSMALLOc(INTERNAL_SIZE_T nb, mstate av)
#else
static Void_t* sYSMALLOc(nb, av) INTERNAL_SIZE_T nb; mstate av;
#endif
{
  mchunkptr       old_top;        
  INTERNAL_SIZE_T old_size;       
  char*           old_end;        

  long            size;           
  char*           brk;            
  char*           mm;             

  long            correction;     
  char*           snd_brk;        

  INTERNAL_SIZE_T front_misalign; 
  INTERNAL_SIZE_T end_misalign;   
  char*           aligned_brk;    

  mchunkptr       p;              
  mchunkptr       remainder;      
  long            remainder_size; 

  unsigned long   sum;            

  size_t          pagemask  = av->pagesize - 1;

  






#if HAVE_MMAP
  if ((unsigned long)nb >= (unsigned long)(av->mmap_threshold) &&
      (av->n_mmaps < av->n_mmaps_max)) {

    




    size = (nb + SIZE_SZ + MALLOC_ALIGN_MASK + pagemask) & ~pagemask;
    
    mm = (char*)(MMAP(0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE));

    if (mm != (char*)(MORECORE_FAILURE)) {

      







      front_misalign = (INTERNAL_SIZE_T)chunk2mem(mm) & MALLOC_ALIGN_MASK;
      if (front_misalign > 0) {
        correction = MALLOC_ALIGNMENT - front_misalign;
        p = (mchunkptr)(mm + correction);
        p->prev_size = correction;
        set_head(p, (size - correction) |IS_MMAPPED);
      }
      else {
        p = (mchunkptr)mm;
        set_head(p, size|IS_MMAPPED);
      }

      check_chunk(p);
      
      

      if (++av->n_mmaps > av->max_n_mmaps) 
        av->max_n_mmaps = av->n_mmaps;

      sum = av->mmapped_mem += size;
      if (sum > (unsigned long)(av->max_mmapped_mem)) 
        av->max_mmapped_mem = sum;
      sum += av->sbrked_mem;
      if (sum > (unsigned long)(av->max_total_mem)) 
        av->max_total_mem = sum;
      
      return chunk2mem(p);
    }
  }
#endif

  

  old_top  = av->top;
  old_size = chunksize(old_top);
  old_end  = (char*)(chunk_at_offset(old_top, old_size));

  brk = snd_brk = (char*)(MORECORE_FAILURE); 

  




  assert(old_top == initial_top(av) || 
         ((unsigned long) (old_size) >= (unsigned long)(MINSIZE) &&
          prev_inuse(old_top)));


  

  size = nb + av->top_pad + MINSIZE;

  





  if (av->sbrk_base != NONCONTIGUOUS_REGIONS)
    size -= old_size;

  







  size = (size + pagemask) & ~pagemask;

  





  if (size > 0) 
    brk = (char*)(MORECORE(size));

  







#if HAVE_MMAP
  if (brk == (char*)(MORECORE_FAILURE)) {

    

    if (av->sbrk_base != NONCONTIGUOUS_REGIONS)
      size = (size + old_size + pagemask) & ~pagemask;

    

    if ((unsigned long)size < (unsigned long)MMAP_AS_MORECORE_SIZE)
      size = MMAP_AS_MORECORE_SIZE;

    brk = (char*)(MMAP(0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE));

    if (brk != (char*)(MORECORE_FAILURE)) {

      
      snd_brk = brk + size;

      




      av->sbrk_base = NONCONTIGUOUS_REGIONS; 
    }
  }
#endif

  if (brk != (char*)(MORECORE_FAILURE)) {

    av->sbrked_mem += size;

    


    
    if (brk == old_end && snd_brk == (char*)(MORECORE_FAILURE)) {
      set_head(old_top, (size + old_size) | PREV_INUSE);
    }
    
    



















    
    else {
      front_misalign = 0;
      end_misalign = 0;
      correction = 0;
      aligned_brk = brk;
      
      
      if (av->sbrk_base != NONCONTIGUOUS_REGIONS) { 
        
        

        front_misalign = (INTERNAL_SIZE_T)chunk2mem(brk) & MALLOC_ALIGN_MASK;
        if (front_misalign > 0) {

          







          correction = MALLOC_ALIGNMENT - front_misalign;
          aligned_brk += correction;
        }
        
        



        
        correction += old_size;
        
        

        end_misalign = (INTERNAL_SIZE_T)(brk + size + correction);
        correction += ((end_misalign + pagemask) & ~pagemask) - end_misalign;
        
        assert(correction >= 0);
        
        snd_brk = (char*)(MORECORE(correction));
        
        








        
        if (snd_brk == (char*)(MORECORE_FAILURE)) {
          correction = 0;
          snd_brk = (char*)(MORECORE(0));
        }
      }
      
      
      else { 
        
        
        assert(((unsigned long)chunk2mem(brk) & MALLOC_ALIGN_MASK) == 0);
        
        
        if (snd_brk == (char*)(MORECORE_FAILURE)) {
          snd_brk = (char*)(MORECORE(0));
        }
        
        
        if (snd_brk != (char*)(MORECORE_FAILURE)) {
          assert(((INTERNAL_SIZE_T)(snd_brk) & pagemask) == 0);
        }
      }
      
      
      if (snd_brk != (char*)(MORECORE_FAILURE)) {
       
        av->top = (mchunkptr)aligned_brk;
        set_head(av->top, (snd_brk - aligned_brk + correction) | PREV_INUSE);
        
        av->sbrked_mem += correction;
        
        
        if (old_top == initial_top(av)) {
          if (av->sbrk_base == (char*)(MORECORE_FAILURE)) 
            av->sbrk_base = brk;
        }
        
        








        else {
          
          



          old_size = (old_size - 3*SIZE_SZ) & ~MALLOC_ALIGN_MASK;
          set_head(old_top, old_size | PREV_INUSE);
          
          




          chunk_at_offset(old_top, old_size          )->size =
            SIZE_SZ|PREV_INUSE;

          chunk_at_offset(old_top, old_size + SIZE_SZ)->size =
            SIZE_SZ|PREV_INUSE;
          
          
          if (old_size >= MINSIZE) 
            fREe(chunk2mem(old_top));

        }
      }
    }
    
    
    
    sum = av->sbrked_mem;
    if (sum > (unsigned long)(av->max_sbrked_mem))
      av->max_sbrked_mem = sum;
    
    sum += av->mmapped_mem;
    if (sum > (unsigned long)(av->max_total_mem))
      av->max_total_mem = sum;

    check_malloc_state();
    
    
    
    p = av->top;
    size = chunksize(p);
    remainder_size = (long)size - (long)nb;
    
    
    if (remainder_size >= (long)MINSIZE) {
      remainder = chunk_at_offset(p, nb);
      av->top = remainder;
      set_head(p, nb | PREV_INUSE);
      set_head(remainder, remainder_size | PREV_INUSE);
      
      check_malloced_chunk(p, nb);
      return chunk2mem(p);
    }
  }

  
  MALLOC_FAILURE_ACTION;
  return 0;
}











#if __STD_C
static int sYSTRIm(size_t pad, mstate av)
#else
static int sYSTRIm(pad, av) size_t pad; mstate av;
#endif
{
  long  top_size;        
  long  extra;           
  long  released;        
  char* current_brk;     
  char* new_brk;         
  size_t pagesz;

  
  if (av->sbrk_base != NONCONTIGUOUS_REGIONS) {

    pagesz = av->pagesize;
    top_size = chunksize(av->top);
    
    
    extra = ((top_size - pad - MINSIZE + (pagesz-1)) / pagesz - 1) * pagesz;
    
    if (extra > 0) {
      
      



      current_brk = (char*)(MORECORE(0));
      if (current_brk == (char*)(av->top) + top_size) {
        
        








        
        MORECORE(-extra);
        new_brk = (char*)(MORECORE(0));
        
        if (new_brk != (char*)MORECORE_FAILURE) {
          released = (long)(current_brk - new_brk);

          if (released != 0) {
            
            av->sbrked_mem -= released;
            set_head(av->top, (top_size - released) | PREV_INUSE);
            check_malloc_state();
            return 1;
          }
        }
      }
    }
  }

  return 0;
}








#if __STD_C
Void_t* mALLOc(size_t bytes)
#else
  Void_t* mALLOc(bytes) size_t bytes;
#endif
{
  mstate av = get_malloc_state();

  INTERNAL_SIZE_T nb;               
  unsigned int    idx;              
  mbinptr         bin;              
  mfastbinptr*    fb;               

  mchunkptr       victim;           
  INTERNAL_SIZE_T size;             
  int             victim_index;     

  mchunkptr       remainder;        
  long            remainder_size;   

  unsigned int    block;            
  unsigned int    bit;              
  unsigned int    map;              

  mchunkptr       fwd;              
  mchunkptr       bck;              


  









  checked_request2size(bytes, nb);

  





  if (nb <= av->max_fast) { 
    fb = &(av->fastbins[(fastbin_index(nb))]);
    if ( (victim = *fb) != 0) {
      *fb = victim->fd;
      check_remalloced_chunk(victim, nb);
      return chunk2mem(victim);
    }
  }

  








  if (in_smallbin_range(nb)) {
    idx = smallbin_index(nb);
    bin = bin_at(av,idx);

    if ( (victim = last(bin)) != bin) {
      if (victim == 0) 
        malloc_consolidate(av);
      else {
        bck = victim->bk;
        set_inuse_bit_at_offset(victim, nb);
        bin->bk = bck;
        bck->fd = bin;
        
        check_malloced_chunk(victim, nb);
        return chunk2mem(victim);
      }
    }
  }

  









  else {
    idx = largebin_index(nb);
    if (have_fastchunks(av)) 
      malloc_consolidate(av);
  }


  












    
  for(;;) {    
    
    while ( (victim = unsorted_chunks(av)->bk) != unsorted_chunks(av)) {
      bck = victim->bk;
      size = chunksize(victim);

      






      if (in_smallbin_range(nb) && 
          victim == av->last_remainder &&
          bck == unsorted_chunks(av) &&
          (remainder_size = (long)size - (long)nb) >= (long)MINSIZE) {

        
        remainder = chunk_at_offset(victim, nb);
        unsorted_chunks(av)->bk = unsorted_chunks(av)->fd = remainder;
        av->last_remainder = remainder; 
        remainder->bk = remainder->fd = unsorted_chunks(av);
        
        set_head(victim, nb | PREV_INUSE);
        set_head(remainder, remainder_size | PREV_INUSE);
        set_foot(remainder, remainder_size);
        
        check_malloced_chunk(victim, nb);
        return chunk2mem(victim);
      }
      
      
      unsorted_chunks(av)->bk = bck;
      bck->fd = unsorted_chunks(av);
      
      
      
      if (size == nb) {
        set_inuse_bit_at_offset(victim, size);
        check_malloced_chunk(victim, nb);
        return chunk2mem(victim);
      }
      
      
      
      if (in_smallbin_range(size)) {
        victim_index = smallbin_index(size);
        bck = bin_at(av, victim_index);
        fwd = bck->fd;
      }
      else {
        victim_index = largebin_index(size);
        bck = bin_at(av, victim_index);
        fwd = bck->fd;

        
        if (fwd != bck) {
          
          if ((unsigned long)size <= 
              (unsigned long)(chunksize(bck->bk))) {
            fwd = bck;
            bck = bck->bk;
          }
          else {
            while (fwd != bck && 
                   (unsigned long)size < (unsigned long)(chunksize(fwd))) {
              fwd = fwd->fd;
            }
            bck = fwd->bk;
          }
        }
      }
      
      mark_bin(av, victim_index);
      victim->bk = bck;
      victim->fd = fwd;
      fwd->bk = victim;
      bck->fd = victim;
    }
   
    






      
    if (!in_smallbin_range(nb)) {
      bin = bin_at(av, idx);

      
      if ((victim = last(bin)) != bin &&
          (long)(chunksize(first(bin))) - (long)(nb) >= 0) {
        do {
          size = chunksize(victim);
          remainder_size = (long)size - (long)nb;
          
          if (remainder_size >= 0)  {
            unlink(victim, bck, fwd);
            
            
            if (remainder_size < (long)MINSIZE)  {
              set_inuse_bit_at_offset(victim, size);
              check_malloced_chunk(victim, nb);
              return chunk2mem(victim);
            }
            
            else {
              remainder = chunk_at_offset(victim, nb);
              unsorted_chunks(av)->bk = unsorted_chunks(av)->fd = remainder;
              remainder->bk = remainder->fd = unsorted_chunks(av);
              set_head(victim, nb | PREV_INUSE);
              set_head(remainder, remainder_size | PREV_INUSE);
              set_foot(remainder, remainder_size);
              check_malloced_chunk(victim, nb);
              return chunk2mem(victim);
            }
          }
        } while ( (victim = victim->bk) != bin);
      }
    }    

    









    
    ++idx;
    bin = bin_at(av,idx);
    block = idx2block(idx);
    map = av->binmap[block];
    bit = idx2bit(idx);
    
    for (;;) {
      


      
      if (bit > map || bit == 0) {
        for (;;) {
          if (++block >= BINMAPSIZE)  
            break;

          else if ( (map = av->binmap[block]) != 0) {
            bin = bin_at(av, (block << BINMAPSHIFT));
            bit = 1;
            break;
          }
        }
        
        if (block >= BINMAPSIZE) 
          break;
      }
      
      
      while ((bit & map) == 0) {
        bin = next_bin(bin);
        bit <<= 1;
      }
      
      victim = last(bin);
      
      
      if (victim == bin) {
        av->binmap[block] = map &= ~bit; 
        bin = next_bin(bin);
        bit <<= 1;
      }
      
      
      else {
        size = chunksize(victim);
        remainder_size = (long)size - (long)nb;
        
        assert(remainder_size >= 0);
        
        
        bck = victim->bk;
        bin->bk = bck;
        bck->fd = bin;
        
        
        
        if (remainder_size < (long)MINSIZE) {
          set_inuse_bit_at_offset(victim, size);
          check_malloced_chunk(victim, nb);
          return chunk2mem(victim);
        }
        
        
        else {
          remainder = chunk_at_offset(victim, nb);
          
          unsorted_chunks(av)->bk = unsorted_chunks(av)->fd = remainder;
          remainder->bk = remainder->fd = unsorted_chunks(av);
          
          if (in_smallbin_range(nb)) 
            av->last_remainder = remainder; 
          
          set_head(victim, nb | PREV_INUSE);
          set_head(remainder, remainder_size | PREV_INUSE);
          set_foot(remainder, remainder_size);
          check_malloced_chunk(victim, nb);
          return chunk2mem(victim);
        }
      }
    }
    
    













    victim = av->top;
    size = chunksize(victim);
    remainder_size = (long)size - (long)nb;
   
    if (remainder_size >= (long)MINSIZE) {
      remainder = chunk_at_offset(victim, nb);
      av->top = remainder;
      set_head(victim, nb | PREV_INUSE);
      set_head(remainder, remainder_size | PREV_INUSE);
      
      check_malloced_chunk(victim, nb);
      return chunk2mem(victim);
    }
    
    





    else if (have_fastchunks(av)) {
      assert(in_smallbin_range(nb));
      idx = smallbin_index(nb); 
      malloc_consolidate(av);
    }

    


    else 
      return sYSMALLOc(nb, av);    
  }
}







#if __STD_C
void fREe(Void_t* mem)
#else
void fREe(mem) Void_t* mem;
#endif
{
  mstate av = get_malloc_state();

  mchunkptr       p;           
  INTERNAL_SIZE_T size;        
  mfastbinptr*    fb;          
  mchunkptr       nextchunk;   
  INTERNAL_SIZE_T nextsize;    
  int             nextinuse;   
  INTERNAL_SIZE_T prevsize;    
  mchunkptr       bck;         
  mchunkptr       fwd;         


  
  if (mem != 0) {

    p = mem2chunk(mem);
    check_inuse_chunk(p);

    size = chunksize(p);

    




    if ((unsigned long)size <= (unsigned long)av->max_fast

#if TRIM_FASTBINS
        



        && (chunk_at_offset(p, size) != av->top)
#endif
        ) {

      set_fastchunks(av);
      fb = &(av->fastbins[fastbin_index(size)]);
      p->fd = *fb;
      *fb = p;
    }

    



    else if (!chunk_is_mmapped(p)) {

      nextchunk = chunk_at_offset(p, size);

      
      if (!prev_inuse(p)) {
        prevsize = p->prev_size;
        size += prevsize;
        p = chunk_at_offset(p, -((long) prevsize));
        unlink(p, bck, fwd);
      }

      nextsize = chunksize(nextchunk);

      if (nextchunk != av->top) {

        
        nextinuse = inuse_bit_at_offset(nextchunk, nextsize);
        set_head(nextchunk, nextsize);

        
        if (!nextinuse) {
          unlink(nextchunk, bck, fwd);
          size += nextsize;
        }

        





        bck = unsorted_chunks(av);
        fwd = bck->fd;
        p->bk = bck;
        p->fd = fwd;
        bck->fd = p;
        fwd->bk = p;

        set_head(p, size | PREV_INUSE);
        set_foot(p, size);
      }

      




      else {
        size += nextsize;
        set_head(p, size | PREV_INUSE);
        av->top = p;

        












        if ((unsigned long)(size) > (unsigned long)(av->trim_threshold / 2)) {
          if (have_fastchunks(av)) {
            malloc_consolidate(av);
            size = chunksize(av->top);
          }

          if ((unsigned long)(size) > (unsigned long)(av->trim_threshold)) 
            sYSTRIm(av->top_pad, av);
        }
      }
    }

    







    else {
#if HAVE_MMAP
      int ret;
      INTERNAL_SIZE_T offset = p->prev_size;
      av->n_mmaps--;
      av->mmapped_mem -= (size + offset);
      ret = munmap((char*)p - offset, size + offset);
      
      assert(ret == 0);
#endif
    }
  }
}















#if __STD_C
static void malloc_consolidate(mstate av)
#else
static void malloc_consolidate(av) mstate av;
#endif
{
  mfastbinptr*    fb;
  mfastbinptr*    maxfb;
  mchunkptr       p;
  mchunkptr       nextp;
  mchunkptr       unsorted_bin;
  mchunkptr       first_unsorted;

  
  mchunkptr       nextchunk;
  INTERNAL_SIZE_T size;
  INTERNAL_SIZE_T nextsize;
  INTERNAL_SIZE_T prevsize;
  int             nextinuse;
  mchunkptr       bck;
  mchunkptr       fwd;

  




  if (av->max_fast == 0) {
    malloc_init_state(av);
    check_malloc_state();
  }
  else if (have_fastchunks(av)) {
    clear_fastchunks(av);
    
    unsorted_bin = unsorted_chunks(av);
    
    






    
    maxfb = &(av->fastbins[fastbin_index(av->max_fast)]);
    fb = &(av->fastbins[0]);
    do {
      if ( (p = *fb) != 0) {
        *fb = 0;
        
        do {
          check_inuse_chunk(p);
          nextp = p->fd;
          
          
          size = p->size & ~PREV_INUSE;
          nextchunk = chunk_at_offset(p, size);
          
          if (!prev_inuse(p)) {
            prevsize = p->prev_size;
            size += prevsize;
            p = chunk_at_offset(p, -((long) prevsize));
            unlink(p, bck, fwd);
          }
          
          nextsize = chunksize(nextchunk);
          
          if (nextchunk != av->top) {
            
            nextinuse = inuse_bit_at_offset(nextchunk, nextsize);
            set_head(nextchunk, nextsize);
            
            if (!nextinuse) {
              size += nextsize;
              unlink(nextchunk, bck, fwd);
            }
            
            first_unsorted = unsorted_bin->fd;
            unsorted_bin->fd = p;
            first_unsorted->bk = p;
            
            set_head(p, size | PREV_INUSE);
            p->bk = unsorted_bin;
            p->fd = first_unsorted;
            set_foot(p, size);
          }
          
          else {
            size += nextsize;
            set_head(p, size | PREV_INUSE);
            av->top = p;
          }
          
        } while ( (p = nextp) != 0);
        
      }
    } while (fb++ != maxfb);
  }
}






































#if __STD_C
Void_t* rEALLOc(Void_t* oldmem, size_t bytes)
#else
Void_t* rEALLOc(oldmem, bytes) Void_t* oldmem; size_t bytes;
#endif
{
  mstate av = get_malloc_state();

  INTERNAL_SIZE_T  nb;              

  mchunkptr        oldp;            
  INTERNAL_SIZE_T  oldsize;         

  mchunkptr        newp;            
  INTERNAL_SIZE_T  newsize;         
  Void_t*          newmem;          

  mchunkptr        next;            
  mchunkptr        prev;            

  mchunkptr        remainder;       
  long             remainder_size;  

  mchunkptr        bck;             
  mchunkptr        fwd;             

  INTERNAL_SIZE_T  copysize;        
  int              ncopies;         
  INTERNAL_SIZE_T* s;                
  INTERNAL_SIZE_T* d;               


#ifdef REALLOC_ZERO_BYTES_FREES
  if (bytes == 0) {
    fREe(oldmem);
    return 0;
  }
#endif

  
  if (oldmem == 0) return mALLOc(bytes);

  checked_request2size(bytes, nb);

  oldp    = mem2chunk(oldmem);
  oldsize = chunksize(oldp);

  check_inuse_chunk(oldp);

  if (!chunk_is_mmapped(oldp)) {

    if ((unsigned long)(oldsize) >= (unsigned long)(nb)) {
      
      newp = oldp;
      newsize = oldsize;
    }

    else {
      newp = 0;
      newsize = 0;

      next = chunk_at_offset(oldp, oldsize);

      if (next == av->top) {            
        newsize = oldsize + chunksize(next);

        if ((unsigned long)(newsize) >= (unsigned long)(nb + MINSIZE)) {
          set_head_size(oldp, nb);
          av->top = chunk_at_offset(oldp, nb);
          set_head(av->top, (newsize - nb) | PREV_INUSE);
          return chunk2mem(oldp);
        }

        else if (!prev_inuse(oldp)) {   
          prev = prev_chunk(oldp);
          newsize += chunksize(prev);

          if ((unsigned long)(newsize) >= (unsigned long)(nb + MINSIZE)) {
            newp = prev;
            unlink(prev, bck, fwd);
            av->top = chunk_at_offset(newp, nb);
            set_head(av->top, (newsize - nb) | PREV_INUSE);
            newsize = nb; 
          }
        }
      }

      else if (!inuse(next)) {          
        newsize = oldsize + chunksize(next);
        
        if (((unsigned long)(newsize) >= (unsigned long)(nb))) {
          newp = oldp;
          unlink(next, bck, fwd);
        }
        
        else if (!prev_inuse(oldp)) {   
          prev = prev_chunk(oldp);
          newsize += chunksize(prev);
          
          if (((unsigned long)(newsize) >= (unsigned long)(nb))) {
            newp = prev;
            unlink(prev, bck, fwd);
            unlink(next, bck, fwd);
          }
        }
      }
      
      else if (!prev_inuse(oldp)) {     
        prev = prev_chunk(oldp);
        newsize = oldsize + chunksize(prev);
        
        if ((unsigned long)(newsize) >= (unsigned long)(nb)) {
          newp = prev;
          unlink(prev, bck, fwd);
        }
      }
      
      if (newp != 0) {
        if (newp != oldp) {
          
          MALLOC_COPY(chunk2mem(newp), oldmem, oldsize - SIZE_SZ, 1);
        }
      }

      
      else {                  
        newmem = mALLOc(nb - MALLOC_ALIGN_MASK);
        if (newmem == 0)
          return 0; 

        newp = mem2chunk(newmem);
        newsize = chunksize(newp);

        


        if (newp == next) {
          newsize += oldsize;
          newp = oldp;
        }
        else {

          




          
          copysize = oldsize - SIZE_SZ;
          s = (INTERNAL_SIZE_T*)oldmem;
          d = (INTERNAL_SIZE_T*)(chunk2mem(newp));
          ncopies = copysize / sizeof(INTERNAL_SIZE_T);
          assert(ncopies >= 3);
          
          if (ncopies > 9)
            MALLOC_COPY(d, s, copysize, 0);
          
          else {
            *(d+0) = *(s+0);
            *(d+1) = *(s+1);
            *(d+2) = *(s+2);
            if (ncopies > 4) {
              *(d+3) = *(s+3);
              *(d+4) = *(s+4);
              if (ncopies > 6) {
                *(d+5) = *(s+5);
                *(d+6) = *(s+6);
                if (ncopies > 8) {
                  *(d+7) = *(s+7);
                  *(d+8) = *(s+8);
                }
              }
            }
          }

          fREe(oldmem);
          check_inuse_chunk(newp);
          return chunk2mem(newp);
        }
      }
    }


    

    remainder_size = (long)newsize - (long)nb;
    assert(remainder_size >= 0);

    if (remainder_size >= (long)MINSIZE) { 
      remainder = chunk_at_offset(newp, nb);
      set_head_size(newp, nb);
      set_head(remainder, remainder_size | PREV_INUSE);
      
      set_inuse_bit_at_offset(remainder, remainder_size);
      fREe(chunk2mem(remainder)); 
    }

    else { 
      set_head_size(newp, newsize);
      set_inuse_bit_at_offset(newp, newsize);
    }

    check_inuse_chunk(newp);
    return chunk2mem(newp);
  }

  



  else {
#if HAVE_MMAP

#if HAVE_MREMAP
    INTERNAL_SIZE_T offset = oldp->prev_size;
    size_t pagemask = av->pagesize - 1;
    char *cp;
    unsigned long sum;
    
    
    newsize = (nb + offset + SIZE_SZ + pagemask) & ~pagemask;

    
    if (oldsize == newsize - offset) 
      return oldmem;

    cp = (char*)mremap((char*)oldp - offset, oldsize + offset, newsize, 1);
    
    if (cp != (char*)MORECORE_FAILURE) {

      newp = (mchunkptr)(cp + offset);
      set_head(newp, (newsize - offset)|IS_MMAPPED);
      
      assert(aligned_OK(chunk2mem(newp)));
      assert((newp->prev_size == offset));
      
      
      sum = av->mmapped_mem += newsize - oldsize;
      if (sum > (unsigned long)(av->max_mmapped_mem)) 
        av->max_mmapped_mem = sum;
      sum += av->sbrked_mem;
      if (sum > (unsigned long)(av->max_total_mem)) 
        av->max_total_mem = sum;
      
      return chunk2mem(newp);
    }

#endif

    
    if ((long)oldsize - (long)SIZE_SZ >= (long)nb)
      newmem = oldmem; 
    else {
      
      newmem = mALLOc(nb - MALLOC_ALIGN_MASK);
      if (newmem != 0) {
        MALLOC_COPY(newmem, oldmem, oldsize - 2*SIZE_SZ, 0);
        fREe(oldmem);
      }
    }
    return newmem;

#else 
    
    check_malloc_state();
    MALLOC_FAILURE_ACTION;
    return 0;
#endif
  }

}


















#if __STD_C
Void_t* mEMALIGn(size_t alignment, size_t bytes)
#else
Void_t* mEMALIGn(alignment, bytes) size_t alignment; size_t bytes;
#endif
{
  INTERNAL_SIZE_T nb;             
  char*           m;              
  mchunkptr       p;              
  char*           brk;            
  mchunkptr       newp;           
  INTERNAL_SIZE_T newsize;        
  INTERNAL_SIZE_T leadsize;       
  mchunkptr       remainder;      
  long            remainder_size; 


  

  if (alignment <= MALLOC_ALIGNMENT) return mALLOc(bytes);

  

  if (alignment <  MINSIZE) alignment = MINSIZE;

  
  if ((alignment & (alignment - 1)) != 0) {
    size_t a = MALLOC_ALIGNMENT * 2;
    while ((unsigned long)a < (unsigned long)alignment) a <<= 1;
    alignment = a;
  }

  checked_request2size(bytes, nb);

  

  m  = (char*)(mALLOc(nb + alignment + MINSIZE));

  if (m == 0) return 0; 

  p = mem2chunk(m);

  if ((((unsigned long)(m)) % alignment) != 0) { 

    







    brk = (char*)mem2chunk(((unsigned long)(m + alignment - 1)) &
                           -((signed long) alignment));
    if ((long)(brk - (char*)(p)) < (long)MINSIZE)
      brk = brk + alignment;

    newp = (mchunkptr)brk;
    leadsize = brk - (char*)(p);
    newsize = chunksize(p) - leadsize;

    
    if (chunk_is_mmapped(p)) {
      newp->prev_size = p->prev_size + leadsize;
      set_head(newp, newsize|IS_MMAPPED);
      return chunk2mem(newp);
    }

    

    set_head(newp, newsize | PREV_INUSE);
    set_inuse_bit_at_offset(newp, newsize);
    set_head_size(p, leadsize);
    fREe(chunk2mem(p));
    p = newp;

    assert (newsize >= nb &&
            (((unsigned long)(chunk2mem(p))) % alignment) == 0);
  }

  
  if (!chunk_is_mmapped(p)) {

    remainder_size = (long)(chunksize(p)) - (long)nb;

    if (remainder_size >= (long)MINSIZE) {
      remainder = chunk_at_offset(p, nb);
      set_head(remainder, remainder_size | PREV_INUSE);
      set_head_size(p, nb);
      fREe(chunk2mem(remainder));
    }
  }

  check_inuse_chunk(p);
  return chunk2mem(p);

}








#if __STD_C
Void_t* cALLOc(size_t n_elements, size_t elem_size)
#else
Void_t* cALLOc(n_elements, elem_size) size_t n_elements; size_t elem_size;
#endif
{
  mchunkptr p;
  INTERNAL_SIZE_T clearsize;
  int nclears;
  INTERNAL_SIZE_T* d;

  Void_t* mem = mALLOc(n_elements * elem_size);

  if (mem != 0) {
    p = mem2chunk(mem);
    if (!chunk_is_mmapped(p)) {  

      





      d = (INTERNAL_SIZE_T*)mem;
      clearsize = chunksize(p) - SIZE_SZ;
      nclears = clearsize / sizeof(INTERNAL_SIZE_T);
      assert(nclears >= 3);

      if (nclears > 9)
        MALLOC_ZERO(d, clearsize);

      else {
        *(d+0) = 0;
        *(d+1) = 0;
        *(d+2) = 0;
        if (nclears > 4) {
          *(d+3) = 0;
          *(d+4) = 0;
          if (nclears > 6) {
            *(d+5) = 0;
            *(d+6) = 0;
            if (nclears > 8) {
              *(d+7) = 0;
              *(d+8) = 0;
            }
          }
        }
      }
    }
  }
  return mem;
}








#if __STD_C
void cFREe(Void_t *mem)
#else
void cFREe(mem) Void_t *mem;
#endif
{
  fREe(mem);
}











#if __STD_C
Void_t* vALLOc(size_t bytes)
#else
Void_t* vALLOc(bytes) size_t bytes;
#endif
{
  
  mstate av = get_malloc_state();
  malloc_consolidate(av);
  return mEMALIGn(av->pagesize, bytes);
}







#if __STD_C
Void_t* pVALLOc(size_t bytes)
#else
Void_t* pVALLOc(bytes) size_t bytes;
#endif
{
  mstate av = get_malloc_state();
  size_t pagesz;

  
  malloc_consolidate(av);

  pagesz = av->pagesize;
  return mEMALIGn(pagesz, (bytes + pagesz - 1) & ~(pagesz - 1));
}























#if __STD_C
int mTRIm(size_t pad)
#else
int mTRIm(pad) size_t pad;
#endif
{
  mstate av = get_malloc_state();
  
  malloc_consolidate(av);

  return sYSTRIm(pad, av);
}









#if __STD_C
size_t mUSABLe(Void_t* mem)
#else
size_t mUSABLe(mem) Void_t* mem;
#endif
{
  mchunkptr p;
  if (mem != 0) {
    p = mem2chunk(mem);
    if (chunk_is_mmapped(p))
      return chunksize(p) - 2*SIZE_SZ;
    else if (inuse(p))
      return chunksize(p) - SIZE_SZ;
  }
  return 0;
}








struct mallinfo mALLINFo()
{
  mstate av = get_malloc_state();
  struct mallinfo mi;
  int i;
  mbinptr b;
  mchunkptr p;
  INTERNAL_SIZE_T avail;
  int navail;
  int nfastblocks;
  int fastbytes;

  
  if (av->top == 0)  malloc_consolidate(av);

  check_malloc_state();

  
  avail = chunksize(av->top);
  navail = 1;  

  
  nfastblocks = 0;
  fastbytes = 0;

  for (i = 0; i < NFASTBINS; ++i) {
    for (p = av->fastbins[i]; p != 0; p = p->fd) {
      ++nfastblocks;
      fastbytes += chunksize(p);
    }
  }

  avail += fastbytes;

  
  for (i = 1; i < NBINS; ++i) {
    b = bin_at(av, i);
    for (p = last(b); p != b; p = p->bk) {
      avail += chunksize(p);
      navail++;
    }
  }

  mi.smblks = nfastblocks;
  mi.ordblks = navail;
  mi.fordblks = avail;
  mi.uordblks = av->sbrked_mem - avail;
  mi.arena = av->sbrked_mem;
  mi.hblks = av->n_mmaps;
  mi.hblkhd = av->mmapped_mem;
  mi.fsmblks = fastbytes;
  mi.keepcost = chunksize(av->top);
  mi.usmblks = av->max_total_mem;
  return mi;
}






















void mSTATs()
{
  struct mallinfo mi = mALLINFo();

#ifdef WIN32
  {
    unsigned long free, reserved, committed;
    vminfo (&free, &reserved, &committed);
    fprintf(stderr, "free bytes       = %10lu\n", 
            free);
    fprintf(stderr, "reserved bytes   = %10lu\n", 
            reserved);
    fprintf(stderr, "committed bytes  = %10lu\n", 
            committed);
  }
#endif


  fprintf(stderr, "max system bytes = %10lu\n",
          (unsigned long)(mi.usmblks));
  fprintf(stderr, "system bytes     = %10lu\n",
          (unsigned long)(mi.arena + mi.hblkhd));
  fprintf(stderr, "in use bytes     = %10lu\n",
          (unsigned long)(mi.uordblks + mi.hblkhd));

#ifdef WIN32 
  {
    unsigned long kernel, user;
    if (cpuinfo (TRUE, &kernel, &user)) {
      fprintf(stderr, "kernel ms        = %10lu\n", 
              kernel);
      fprintf(stderr, "user ms          = %10lu\n", 
              user);
    }
  }
#endif
}












#if __STD_C
int mALLOPt(int param_number, int value)
#else
int mALLOPt(param_number, value) int param_number; int value;
#endif
{
  mstate av = get_malloc_state();
  
  malloc_consolidate(av);

  switch(param_number) {
  case M_MXFAST:
    if (value >= 0 && value <= MAX_FAST_SIZE) {
      av->max_fast = req2max_fast(value);
      return 1;
    }
    else
      return 0;

  case M_TRIM_THRESHOLD:
    av->trim_threshold = value;
    return 1;

  case M_TOP_PAD:
    av->top_pad = value;
    return 1;

  case M_MMAP_THRESHOLD:
    av->mmap_threshold = value;
    return 1;

  case M_MMAP_MAX:
#if HAVE_MMAP
    av->n_mmaps_max = value;
    return 1;
#else
    if (value != 0)
      return 0;
    else {
      av->n_mmaps_max = value;
      return 1;
    }
#endif

  default:
    return 0;
  }
}













#ifdef WIN32

#ifdef _DEBUG

#endif


#ifdef USE_MALLOC_LOCK


static int slwait (int *sl) {
    while (InterlockedCompareExchange ((void **) sl, (void *) 1, (void *) 0) != 0) 
	    Sleep (0);
    return 0;
}


static int slrelease (int *sl) {
    InterlockedExchange (sl, 0);
    return 0;
}

#ifdef NEEDED

static int g_sl;
#endif

#endif 


static long getpagesize (void) {
    static long g_pagesize = 0;
    if (! g_pagesize) {
        SYSTEM_INFO system_info;
        GetSystemInfo (&system_info);
        g_pagesize = system_info.dwPageSize;
    }
    return g_pagesize;
}
static long getregionsize (void) {
    static long g_regionsize = 0;
    if (! g_regionsize) {
        SYSTEM_INFO system_info;
        GetSystemInfo (&system_info);
        g_regionsize = system_info.dwAllocationGranularity;
    }
    return g_regionsize;
}


typedef struct _region_list_entry {
    void *top_allocated;
    void *top_committed;
    void *top_reserved;
    long reserve_size;
    struct _region_list_entry *previous;
} region_list_entry;


static int region_list_append (region_list_entry **last, void *base_reserved, long reserve_size) {
    region_list_entry *next = HeapAlloc (GetProcessHeap (), 0, sizeof (region_list_entry));
    if (! next)
        return FALSE;
    next->top_allocated = (char *) base_reserved;
    next->top_committed = (char *) base_reserved;
    next->top_reserved = (char *) base_reserved + reserve_size;
    next->reserve_size = reserve_size;
    next->previous = *last;
    *last = next;
    return TRUE;
}

static int region_list_remove (region_list_entry **last) {
    region_list_entry *previous = (*last)->previous;
    if (! HeapFree (GetProcessHeap (), sizeof (region_list_entry), *last))
        return FALSE;
    *last = previous;
    return TRUE;
}

#define CEIL(size,to)	(((size)+(to)-1)&~((to)-1))
#define FLOOR(size,to)	((size)&~((to)-1))

#define SBRK_SCALE  0





static void *sbrk (long size) {
    static long g_pagesize, g_my_pagesize;
    static long g_regionsize, g_my_regionsize;
    static region_list_entry *g_last;
    void *result = (void *) MORECORE_FAILURE;
#ifdef TRACE
    printf ("sbrk %d\n", size);
#endif
#if defined (USE_MALLOC_LOCK) && defined (NEEDED)
    
    slwait (&g_sl);
#endif
    
    if (! g_pagesize) {
        g_pagesize = getpagesize ();
        g_my_pagesize = g_pagesize << SBRK_SCALE;
    }
    if (! g_regionsize) {
        g_regionsize = getregionsize ();
        g_my_regionsize = g_regionsize << SBRK_SCALE;
    }
    if (! g_last) {
        if (! region_list_append (&g_last, 0, 0)) 
           goto sbrk_exit;
    }
    
    assert (g_last);
    assert ((char *) g_last->top_reserved - g_last->reserve_size <= (char *) g_last->top_allocated &&
            g_last->top_allocated <= g_last->top_committed);
    assert ((char *) g_last->top_reserved - g_last->reserve_size <= (char *) g_last->top_committed &&
            g_last->top_committed <= g_last->top_reserved &&
            (unsigned) g_last->top_committed % g_pagesize == 0);
    assert ((unsigned) g_last->top_reserved % g_regionsize == 0);
    assert ((unsigned) g_last->reserve_size % g_regionsize == 0);
    
    if (size >= 0) {
        
        long allocate_size = size;
        
        long to_commit = (char *) g_last->top_allocated + allocate_size - (char *) g_last->top_committed;
        
        if (to_commit > 0) {
            
            long commit_size = CEIL (to_commit, g_my_pagesize);
            
            long to_reserve = (char *) g_last->top_committed + commit_size - (char *) g_last->top_reserved;
            
            if (to_reserve > 0) {
                
                long remaining_commit_size = (char *) g_last->top_reserved - (char *) g_last->top_committed;
                if (remaining_commit_size > 0) {
                    
                    assert ((unsigned) g_last->top_committed % g_pagesize == 0);
                    assert (0 < remaining_commit_size && remaining_commit_size % g_pagesize == 0); {
                        
                        void *base_committed = VirtualAlloc (g_last->top_committed, remaining_commit_size,
							                                 MEM_COMMIT, PAGE_READWRITE);
                        
                        if (base_committed != g_last->top_committed)
                            goto sbrk_exit;
                        
                        assert ((unsigned) base_committed % g_pagesize == 0);
#ifdef TRACE
                        printf ("Commit %p %d\n", base_committed, remaining_commit_size);
#endif
                        
                        g_last->top_committed = (char *) base_committed + remaining_commit_size;
                    }
                } {
                    
                    int contiguous = -1;
                    int found = FALSE;
                    MEMORY_BASIC_INFORMATION memory_info;
                    void *base_reserved;
                    long reserve_size;
                    do {
                        
                        contiguous = TRUE;
                        
                        reserve_size = CEIL (to_reserve, g_my_regionsize);
                        
                        memory_info.BaseAddress = g_last->top_reserved;
                        
                        assert ((unsigned) memory_info.BaseAddress % g_pagesize == 0);
                        assert (0 < reserve_size && reserve_size % g_regionsize == 0);
                        while (VirtualQuery (memory_info.BaseAddress, &memory_info, sizeof (memory_info))) {
                            
                            assert ((unsigned) memory_info.BaseAddress % g_pagesize == 0);
#ifdef TRACE
                            printf ("Query %p %d %s\n", memory_info.BaseAddress, memory_info.RegionSize, 
                                    memory_info.State == MEM_FREE ? "FREE": 
                                    (memory_info.State == MEM_RESERVE ? "RESERVED":
                                     (memory_info.State == MEM_COMMIT ? "COMMITTED": "?")));
#endif
                            
                            if (memory_info.State == MEM_FREE &&
                                (unsigned) memory_info.BaseAddress % g_regionsize == 0 &&
                                memory_info.RegionSize >= (unsigned) reserve_size) {
                                found = TRUE;
                                break;
                            }
                            
                            contiguous = FALSE;
                            
                            reserve_size = CEIL (allocate_size, g_my_regionsize);
                            memory_info.BaseAddress = (char *) memory_info.BaseAddress + memory_info.RegionSize;
                            
                            assert ((unsigned) memory_info.BaseAddress % g_pagesize == 0);
                            assert (0 < reserve_size && reserve_size % g_regionsize == 0);
                        }
                        
                        if (! found) 
                            goto sbrk_exit;
                        
                        assert ((unsigned) memory_info.BaseAddress % g_regionsize == 0);
                        assert (0 < reserve_size && reserve_size % g_regionsize == 0);
                        
                        base_reserved = VirtualAlloc (memory_info.BaseAddress, reserve_size, 
					                                  MEM_RESERVE, PAGE_NOACCESS);
                        if (! base_reserved) {
                            int rc = GetLastError ();
                            if (rc != ERROR_INVALID_ADDRESS) 
                                goto sbrk_exit;
                        }
                        
                        
                    } while (! base_reserved);
                    
                    if (memory_info.BaseAddress && base_reserved != memory_info.BaseAddress)
                        goto sbrk_exit;
                    
                    assert ((unsigned) base_reserved % g_regionsize == 0);
#ifdef TRACE
                    printf ("Reserve %p %d\n", base_reserved, reserve_size);
#endif
                    
                    if (contiguous) {
                        long start_size = (char *) g_last->top_committed - (char *) g_last->top_allocated;
                        
                        allocate_size -= start_size;
                        
                        g_last->top_allocated = g_last->top_committed;
                        
                        to_commit = (char *) g_last->top_allocated + allocate_size - (char *) g_last->top_committed;
                        
                        commit_size = CEIL (to_commit, g_my_pagesize);
                    } 
                    
                    if (! region_list_append (&g_last, base_reserved, reserve_size))
                        goto sbrk_exit;
                    
                    if (! contiguous) {
                        
                        to_commit = (char *) g_last->top_allocated + allocate_size - (char *) g_last->top_committed;
                        
                        commit_size = CEIL (to_commit, g_my_pagesize);
                    }
                }
            } 
            
            assert ((unsigned) g_last->top_committed % g_pagesize == 0);
            assert (0 < commit_size && commit_size % g_pagesize == 0); {
                
                void *base_committed = VirtualAlloc (g_last->top_committed, commit_size, 
				    			                     MEM_COMMIT, PAGE_READWRITE);
                
                if (base_committed != g_last->top_committed)
                    goto sbrk_exit;
                
                assert ((unsigned) base_committed % g_pagesize == 0);
#ifdef TRACE
                printf ("Commit %p %d\n", base_committed, commit_size);
#endif
                
                g_last->top_committed = (char *) base_committed + commit_size;
            }
        } 
        
        g_last->top_allocated = (char *) g_last->top_allocated + allocate_size;
        result = (char *) g_last->top_allocated - size;
    
    } else if (size < 0) {
        long deallocate_size = - size;
        
        while ((char *) g_last->top_allocated - deallocate_size < (char *) g_last->top_reserved - g_last->reserve_size) {
            
            long release_size = g_last->reserve_size;
            
            void *base_reserved = (char *) g_last->top_reserved - release_size;
            
            assert ((unsigned) base_reserved % g_regionsize == 0); 
            assert (0 < release_size && release_size % g_regionsize == 0); {
                
                int rc = VirtualFree (base_reserved, 0, 
                                      MEM_RELEASE);
                
                if (! rc)
                    goto sbrk_exit;
#ifdef TRACE
                printf ("Release %p %d\n", base_reserved, release_size);
#endif
            }
            
            deallocate_size -= (char *) g_last->top_allocated - (char *) base_reserved;
            
            if (! region_list_remove (&g_last))
                goto sbrk_exit;
        } {
            
            long to_decommit = (char *) g_last->top_committed - ((char *) g_last->top_allocated - deallocate_size);
            if (to_decommit >= g_my_pagesize) {
                
                long decommit_size = FLOOR (to_decommit, g_my_pagesize);
                
                void *base_committed = (char *) g_last->top_committed - decommit_size;
                
                assert ((unsigned) base_committed % g_pagesize == 0);
                assert (0 < decommit_size && decommit_size % g_pagesize == 0); {
                    
                    int rc = VirtualFree ((char *) base_committed, decommit_size, 
                                          MEM_DECOMMIT);
                    
                    if (! rc)
                        goto sbrk_exit;
#ifdef TRACE
                    printf ("Decommit %p %d\n", base_committed, decommit_size);
#endif
                }
                
                deallocate_size -= (char *) g_last->top_allocated - (char *) base_committed;
                g_last->top_committed = base_committed;
                g_last->top_allocated = base_committed;
            }
        }
        
        g_last->top_allocated = (char *) g_last->top_allocated - deallocate_size;
        
        if ((char *) g_last->top_reserved - g_last->reserve_size > (char *) g_last->top_allocated ||
            g_last->top_allocated > g_last->top_committed) {
            
            g_last->top_allocated = (char *) g_last->top_reserved - g_last->reserve_size;
            goto sbrk_exit;
        }
        result = g_last->top_allocated;
    }
    
    assert (g_last);
    assert ((char *) g_last->top_reserved - g_last->reserve_size <= (char *) g_last->top_allocated &&
            g_last->top_allocated <= g_last->top_committed);
    assert ((char *) g_last->top_reserved - g_last->reserve_size <= (char *) g_last->top_committed &&
            g_last->top_committed <= g_last->top_reserved &&
            (unsigned) g_last->top_committed % g_pagesize == 0);
    assert ((unsigned) g_last->top_reserved % g_regionsize == 0);
    assert ((unsigned) g_last->reserve_size % g_regionsize == 0);

sbrk_exit:
#if defined (USE_MALLOC_LOCK) && defined (NEEDED)
    
    slrelease (&g_sl);
#endif
    return result;
}


static void *mmap (void *ptr, long size, long prot, long type, long handle, long arg) {
    static long g_pagesize;
    static long g_regionsize;
#ifdef TRACE
    printf ("mmap %d\n", size);
#endif
#if defined (USE_MALLOC_LOCK) && defined (NEEDED)
    
    slwait (&g_sl);
#endif
    
    if (! g_pagesize) 
        g_pagesize = getpagesize ();
    if (! g_regionsize) 
        g_regionsize = getregionsize ();
    
    assert ((unsigned) ptr % g_regionsize == 0);
    assert (size % g_pagesize == 0);
    
    ptr = VirtualAlloc (ptr, size,
					    MEM_RESERVE | MEM_COMMIT | MEM_TOP_DOWN, PAGE_READWRITE);
    if (! ptr) {
        ptr = (void *) MORECORE_FAILURE;
        goto mmap_exit;
    }
    
    assert ((unsigned) ptr % g_regionsize == 0);
#ifdef TRACE
    printf ("Commit %p %d\n", ptr, size);
#endif
mmap_exit:
#if defined (USE_MALLOC_LOCK) && defined (NEEDED)
    
    slrelease (&g_sl);
#endif
    return ptr;
}


static long munmap (void *ptr, long size) {
    static long g_pagesize;
    static long g_regionsize;
    int rc = MUNMAP_FAILURE;
#ifdef TRACE
    printf ("munmap %p %d\n", ptr, size);
#endif
#if defined (USE_MALLOC_LOCK) && defined (NEEDED)
    
    slwait (&g_sl);
#endif
    
    if (! g_pagesize) 
        g_pagesize = getpagesize ();
    if (! g_regionsize) 
        g_regionsize = getregionsize ();
    
    assert ((unsigned) ptr % g_regionsize == 0);
    assert (size % g_pagesize == 0);
    
    if (! VirtualFree (ptr, 0, 
                       MEM_RELEASE))
        goto munmap_exit;
    rc = 0;
#ifdef TRACE
    printf ("Release %p %d\n", ptr, size);
#endif
munmap_exit:
#if defined (USE_MALLOC_LOCK) && defined (NEEDED)
    
    slrelease (&g_sl);
#endif
    return rc;
}

static void vminfo (unsigned long *free, unsigned long *reserved, unsigned long *committed) {
    MEMORY_BASIC_INFORMATION memory_info;
    memory_info.BaseAddress = 0;
    *free = *reserved = *committed = 0;
    while (VirtualQuery (memory_info.BaseAddress, &memory_info, sizeof (memory_info))) {
        switch (memory_info.State) {
        case MEM_FREE:
            *free += memory_info.RegionSize;
            break;
        case MEM_RESERVE:
            *reserved += memory_info.RegionSize;
            break;
        case MEM_COMMIT:
            *committed += memory_info.RegionSize;
            break;
        }
        memory_info.BaseAddress = (char *) memory_info.BaseAddress + memory_info.RegionSize;
    }
}

static int cpuinfo (int whole, unsigned long *kernel, unsigned long *user) {
    if (whole) {
        __int64 creation64, exit64, kernel64, user64;
        int rc = GetProcessTimes (GetCurrentProcess (), 
                                  (FILETIME *) &creation64,  
                                  (FILETIME *) &exit64, 
                                  (FILETIME *) &kernel64, 
                                  (FILETIME *) &user64);
        if (! rc) {
            *kernel = 0;
            *user = 0;
            return FALSE;
        } 
        *kernel = (unsigned long) (kernel64 / 10000);
        *user = (unsigned long) (user64 / 10000);
        return TRUE;
    } else {
        __int64 creation64, exit64, kernel64, user64;
        int rc = GetThreadTimes (GetCurrentThread (), 
                                 (FILETIME *) &creation64,  
                                 (FILETIME *) &exit64, 
                                 (FILETIME *) &kernel64, 
                                 (FILETIME *) &user64);
        if (! rc) {
            *kernel = 0;
            *user = 0;
            return FALSE;
        } 
        *kernel = (unsigned long) (kernel64 / 10000);
        *user = (unsigned long) (user64 / 10000);
        return TRUE;
    }
}

#endif 




































































































































