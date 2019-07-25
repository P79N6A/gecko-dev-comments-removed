



























#ifndef _LINKER_DEBUG_H_
#define _LINKER_DEBUG_H_

#include <stdio.h>

#ifndef LINKER_DEBUG
#error LINKER_DEBUG should be defined to either 1 or 0 in Android.mk
#endif




#define LINKER_DEBUG_TO_LOG  1
#define TRACE_DEBUG          1
#define DO_TRACE_LOOKUP      1
#define DO_TRACE_RELO        1
#define TIMING               0
#define STATS                0
#define COUNT_PAGES          0










#undef TRUE
#undef FALSE
#define TRUE                 1
#define FALSE                0




#if LINKER_DEBUG
#include "linker_format.h"
extern int debug_verbosity;
#if LINKER_DEBUG_TO_LOG
extern int format_log(int, const char *, const char *, ...);
#define _PRINTVF(v,f,x...)                                        \
    do {                                                          \
        if (debug_verbosity > (v)) format_log(5-(v),"linker",x);  \
    } while (0)
#else 
extern int format_fd(int, const char *, ...);
#define _PRINTVF(v,f,x...)                           \
    do {                                             \
        if (debug_verbosity > (v)) format_fd(1, x);  \
    } while (0)
#endif 
#else 
#define _PRINTVF(v,f,x...)   do {} while(0)
#endif 

#define PRINT(x...)          _PRINTVF(-1, FALSE, x)
#define INFO(x...)           _PRINTVF(0, TRUE, x)
#define TRACE(x...)          _PRINTVF(1, TRUE, x)
#define WARN(fmt,args...)    \
        _PRINTVF(-1, TRUE, "%s:%d| WARNING: " fmt, __FILE__, __LINE__, ## args)
#define ERROR(fmt,args...)    \
        _PRINTVF(-1, TRUE, "%s:%d| ERROR: " fmt, __FILE__, __LINE__, ## args)


#if TRACE_DEBUG
#define DEBUG(x...)          _PRINTVF(2, TRUE, "DEBUG: " x)
#else 
#define DEBUG(x...)          do {} while (0)
#endif 

#if LINKER_DEBUG
#define TRACE_TYPE(t,x...)   do { if (DO_TRACE_##t) { TRACE(x); } } while (0)
#else  
#define TRACE_TYPE(t,x...)   do {} while (0)
#endif 

#if STATS
#define RELOC_ABSOLUTE        0
#define RELOC_RELATIVE        1
#define RELOC_COPY            2
#define RELOC_SYMBOL          3
#define NUM_RELOC_STATS       4

struct _link_stats {
    int reloc[NUM_RELOC_STATS];
};
extern struct _link_stats linker_stats;

#define COUNT_RELOC(type)                                 \
        do { if (type >= 0 && type < NUM_RELOC_STATS) {   \
                linker_stats.reloc[type] += 1;            \
             } else  {                                    \
                PRINT("Unknown reloc stat requested\n");  \
             }                                            \
           } while(0)
#else 
#define COUNT_RELOC(type)     do {} while(0)
#endif 

#if TIMING
#undef WARN
#define WARN(x...)           do {} while (0)
#endif 

#if COUNT_PAGES
extern unsigned bitmask[];
#define MARK(offset)         do {                                        \
        bitmask[((offset) >> 12) >> 3] |= (1 << (((offset) >> 12) & 7)); \
    } while(0)
#else
#define MARK(x)              do {} while (0)
#endif

#define DEBUG_DUMP_PHDR(phdr, name, pid) do { \
        DEBUG("%5d %s (phdr = 0x%08x)\n", (pid), (name), (unsigned)(phdr));   \
        DEBUG("\t\tphdr->offset   = 0x%08x\n", (unsigned)((phdr)->p_offset)); \
        DEBUG("\t\tphdr->p_vaddr  = 0x%08x\n", (unsigned)((phdr)->p_vaddr));  \
        DEBUG("\t\tphdr->p_paddr  = 0x%08x\n", (unsigned)((phdr)->p_paddr));  \
        DEBUG("\t\tphdr->p_filesz = 0x%08x\n", (unsigned)((phdr)->p_filesz)); \
        DEBUG("\t\tphdr->p_memsz  = 0x%08x\n", (unsigned)((phdr)->p_memsz));  \
        DEBUG("\t\tphdr->p_flags  = 0x%08x\n", (unsigned)((phdr)->p_flags));  \
        DEBUG("\t\tphdr->p_align  = 0x%08x\n", (unsigned)((phdr)->p_align));  \
    } while (0)

#endif 
