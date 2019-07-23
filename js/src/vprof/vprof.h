











































































#ifndef __VPROF__
#define __VPROF__






#define THREADED 0
#define THREAD_SAFE 0

#ifdef _MSC_VER
typedef __int8             int8_t;
typedef __int16            int16_t;
typedef __int32            int32_t;
typedef __int64            int64_t;
typedef unsigned __int8    uint8_t;
typedef unsigned __int16   uint16_t;
typedef unsigned __int32   uint32_t;
typedef unsigned __int64   uint64_t;
#else
#include <inttypes.h>
#endif


#if defined(_MSC_VER)
	#define vprof_align8(t) __declspec(align(8)) t
#elif defined(__GNUC__)
	#define vprof_align8(t) t __attribute__ ((aligned (8)))
#endif

#ifdef __cplusplus
extern "C" {
#endif

int profileValue (void** id, char* file, int line, int64_t value, ...);
int _profileEntryValue (void* id, int64_t value);
int histValue(void** id, char* file, int line, int64_t value, int nbins, ...);
int _histEntryValue (void* id, int64_t value);

#ifdef __cplusplus
}
#endif 

#ifndef DOPROF
#define _vprof(v)
#define _nvprof(n,v)
#define _hprof(h)
#define _nhprof(n,h)
#else

#define _vprof(v,...) \
{ \
    static void* id = 0; \
    (id != 0) ? \
        _profileEntryValue (id, (int64_t) (v)) \
    : \
        profileValue (&id, __FILE__, __LINE__, (int64_t) (v), ##__VA_ARGS__, NULL) \
    ;\
}

#define _nvprof(e,v,...) \
{ \
    static void* id = 0; \
    (id != 0) ? \
        _profileEntryValue (id, (int64_t) (v)) \
    : \
        profileValue (&id, (char*) (e), -1, (int64_t) (v), ##__VA_ARGS__, NULL) \
    ; \
}

#define _hprof(v,n,...) \
{ \
    static void* id = 0; \
    (id != 0) ? \
        _histEntryValue (id, (int64_t) (v)) \
    : \
        histValue (&id, __FILE__, __LINE__, (int64_t) (v), (int) (n), ##__VA_ARGS__) \
    ; \
}

#define _nhprof(e,v,n,...) \
{ \
    static void* id = 0; \
    (id != 0) ? \
        _histEntryValue (id, (int64_t) (v)) \
    : \
        histValue (&id, (char*) (e), -1, (int64_t) (v), (int) (n), ##__VA_ARGS__) \
    ; \
}
#endif

#define NUM_EVARS 4

typedef enum {
    LOCK_IS_FREE = 0, 
    LOCK_IS_TAKEN = 1
};

extern
#ifdef __cplusplus
"C" 
#endif
long _InterlockedCompareExchange (
   long volatile * Destination,
   long Exchange,
   long Comperand
);

typedef struct hist hist;

typedef struct hist {
    int nbins;
    int64_t* lb;
    int64_t* count;
} *hist_t;

typedef struct entry entry;

typedef struct entry {
    long lock;
    char* file;
    int line;
    int64_t value;
    int64_t count;
    int64_t sum;
    int64_t min;
    int64_t max;
    void (*func)(void*);
    hist* h;

    entry* next;

    
    void* genptr;
    int ivar[NUM_EVARS];
    vprof_align8(int64_t) i64var[NUM_EVARS];
    vprof_align8(double) dvar[NUM_EVARS];
    

    char pad[128]; 
} *entry_t;

#define _VAL ((entry_t)vprofID)->value
#define _COUNT ((entry_t)vprofID)->count
#define _SUM ((entry_t)vprofID)->sum
#define _MIN ((entry_t)vprofID)->min
#define _MAX ((entry_t)vprofID)->max

#define _GENPTR ((entry_t)vprofID)->genptr

#define _IVAR0 ((entry_t)vprofID)->ivar[0]
#define _IVAR1 ((entry_t)vprofID)->ivar[1]
#define _IVAR2 ((entry_t)vprofID)->ivar[2]
#define _IVAR3 ((entry_t)vprofID)->ivar[3]

#define _I64VAR0 ((entry_t)vprofID)->i64var[0]
#define _I64VAR1 ((entry_t)vprofID)->i64var[1]
#define _I64VAR2 ((entry_t)vprofID)->i64var[2]
#define _I64VAR3 ((entry_t)vprofID)->i64var[3]

#define _DVAR0 ((entry_t)vprofID)->dvar[0]
#define _DVAR1 ((entry_t)vprofID)->dvar[1]
#define _DVAR2 ((entry_t)vprofID)->dvar[2]
#define _DVAR3 ((entry_t)vprofID)->dvar[3]

#endif
