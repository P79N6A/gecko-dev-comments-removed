







































#ifdef NS_TRACE_MALLOC
 






#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#ifdef XP_UNIX
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#endif
#include "plhash.h"
#include "pratom.h"
#include "prlog.h"
#include "prlock.h"
#include "prmon.h"
#include "prprf.h"
#include "prenv.h"
#include "prnetdb.h"
#include "nsTraceMalloc.h"
#include "nscore.h"
#include "prinit.h"
#include "prthread.h"
#include "nsStackWalk.h"
#include "nsTraceMallocCallbacks.h"

#ifdef XP_WIN32
#include <sys/timeb.h>
#include <sys/stat.h>

#include <io.h> 

#define WRITE_FLAGS "w"

#endif 

#ifdef XP_UNIX
#define WRITE_FLAGS "w"

#ifdef WRAP_SYSTEM_INCLUDES
#pragma GCC visibility push(default)
#endif
extern __ptr_t __libc_malloc(size_t);
extern __ptr_t __libc_calloc(size_t, size_t);
extern __ptr_t __libc_realloc(__ptr_t, size_t);
extern void    __libc_free(__ptr_t);
extern __ptr_t __libc_memalign(size_t, size_t);
extern __ptr_t __libc_valloc(size_t);
#ifdef WRAP_SYSTEM_INCLUDES
#pragma GCC visibility pop
#endif

#endif 

#ifdef XP_WIN32

#define __libc_malloc(x)                dhw_orig_malloc(x)
#define __libc_calloc(x, y)             dhw_orig_calloc(x,y)
#define __libc_realloc(x, y)            dhw_orig_realloc(x,y)
#define __libc_free(x)                  dhw_orig_free(x)

#endif

typedef struct logfile logfile;

#define STARTUP_TMBUFSIZE (64 * 1024)
#define LOGFILE_TMBUFSIZE (16 * 1024)

struct logfile {
    int         fd;
    int         lfd;            
    char        *buf;
    int         bufsize;
    int         pos;
    uint32      size;
    uint32      simsize;
    logfile     *next;
    logfile     **prevp;
};

static char      default_buf[STARTUP_TMBUFSIZE];
static logfile   default_logfile =
                   {-1, 0, default_buf, STARTUP_TMBUFSIZE, 0, 0, 0, NULL, NULL};
static logfile   *logfile_list = NULL;
static logfile   **logfile_tail = &logfile_list;
static logfile   *logfp = &default_logfile;
static PRLock    *tmlock = NULL;
static char      *sdlogname = NULL; 









static int tracing_enabled = 1;








#define TM_ENTER_LOCK()                                                       \
    PR_BEGIN_MACRO                                                            \
        if (tmlock)                                                           \
            PR_Lock(tmlock);                                                  \
    PR_END_MACRO

#define TM_EXIT_LOCK()                                                        \
    PR_BEGIN_MACRO                                                            \
        if (tmlock)                                                           \
            PR_Unlock(tmlock);                                                \
    PR_END_MACRO














#ifdef XP_WIN32

#include <windows.h>

#define TM_TLS_INDEX_TYPE               DWORD
#define TM_CREATE_TLS_INDEX(i_)         PR_BEGIN_MACRO                        \
                                          (i_) = TlsAlloc();                  \
                                        PR_END_MACRO
#define TM_DESTROY_TLS_INDEX(i_)        TlsFree((i_))
#define TM_GET_TLS_DATA(i_)             TlsGetValue((i_))
#define TM_SET_TLS_DATA(i_, v_)         TlsSetValue((i_), (v_))

#else

#include <pthread.h>

#define TM_TLS_INDEX_TYPE               pthread_key_t
#define TM_CREATE_TLS_INDEX(i_)         pthread_key_create(&(i_), NULL)
#define TM_DESTROY_TLS_INDEX(i_)        pthread_key_delete((i_))
#define TM_GET_TLS_DATA(i_)             pthread_getspecific((i_))
#define TM_SET_TLS_DATA(i_, v_)         pthread_setspecific((i_), (v_))

#endif

static TM_TLS_INDEX_TYPE tls_index;
static tm_thread main_thread; 


#if 0
PR_STATIC_CALLBACK(void)
free_tm_thread(void *priv)
{
    tm_thread *t = (tm_thread*) priv;

    PR_ASSERT(t->suppress_tracing == 0);

    if (t->in_heap) {
        t->suppress_tracing = 1;
        if (t->backtrace_buf.buffer)
            __libc_free(t->backtrace_buf.buffer);

        __libc_free(t);
    }
}
#endif

tm_thread *
tm_get_thread(void)
{
    tm_thread *t;
    tm_thread stack_tm_thread;

    if (!tmlock) {
        return &main_thread;
    }

    t = TM_GET_TLS_DATA(tls_index);

    if (!t) {
        



        stack_tm_thread.suppress_tracing = 1;
        stack_tm_thread.backtrace_buf.buffer = NULL;
        stack_tm_thread.backtrace_buf.size = 0;
        stack_tm_thread.backtrace_buf.entries = 0;
        TM_SET_TLS_DATA(tls_index, &stack_tm_thread);

        t = (tm_thread*) __libc_malloc(sizeof(tm_thread));
        t->suppress_tracing = 0;
        t->backtrace_buf = stack_tm_thread.backtrace_buf;
        TM_SET_TLS_DATA(tls_index, t);

        PR_ASSERT(stack_tm_thread.suppress_tracing == 1); 
    }

    return t;
}


typedef uint32          lfd_set;

#define LFD_SET_STATIC_INITIALIZER 0
#define LFD_SET_SIZE    32

#define LFD_ZERO(s)     (*(s) = 0)
#define LFD_BIT(i)      ((uint32)1 << (i))
#define LFD_TEST(i,s)   (LFD_BIT(i) & *(s))
#define LFD_SET(i,s)    (*(s) |= LFD_BIT(i))
#define LFD_CLR(i,s)    (*(s) &= ~LFD_BIT(i))

static logfile *get_logfile(int fd)
{
    logfile *fp;
    int lfd;

    for (fp = logfile_list; fp; fp = fp->next) {
        if (fp->fd == fd)
            return fp;
    }
    lfd = 0;
retry:
    for (fp = logfile_list; fp; fp = fp->next) {
        if (fp->fd == lfd) {
            if (++lfd >= LFD_SET_SIZE)
                return NULL;
            goto retry;
        }
    }
    fp = __libc_malloc(sizeof(logfile) + LOGFILE_TMBUFSIZE);
    if (!fp)
        return NULL;
    fp->fd = fd;
    fp->lfd = lfd;
    fp->buf = (char*) (fp + 1);
    fp->bufsize = LOGFILE_TMBUFSIZE;
    fp->pos = 0;
    fp->size = fp->simsize = 0;
    fp->next = NULL;
    fp->prevp = logfile_tail;
    *logfile_tail = fp;
    logfile_tail = &fp->next;
    return fp;
}

static void flush_logfile(logfile *fp)
{
    int len, cnt, fd;
    char *bp;

    len = fp->pos;
    if (len == 0)
        return;
    fp->pos = 0;
    fd = fp->fd;
    if (fd >= 0) {
        fp->size += len;
        bp = fp->buf;
        do {
            cnt = write(fd, bp, len);
            if (cnt <= 0) {
                printf("### nsTraceMalloc: write failed or wrote 0 bytes!\n");
                return;
            }
            bp += cnt;
            len -= cnt;
        } while (len > 0);
    }
    fp->simsize += len;
}

static void log_byte(logfile *fp, char byte)
{
    if (fp->pos == fp->bufsize)
        flush_logfile(fp);
    fp->buf[fp->pos++] = byte;
}

static void log_string(logfile *fp, const char *str)
{
    int len, rem, cnt;

    len = strlen(str);
    while ((rem = fp->pos + len - fp->bufsize) > 0) {
        cnt = len - rem;
        strncpy(&fp->buf[fp->pos], str, cnt);
        str += cnt;
        fp->pos += cnt;
        flush_logfile(fp);
        len = rem;
    }
    strncpy(&fp->buf[fp->pos], str, len);
    fp->pos += len;

    
    log_byte(fp, '\0');
}

static void log_filename(logfile* fp, const char* filename)
{
    if (strlen(filename) < 512) {
        char *bp, *cp, buf[512];

        bp = strstr(strcpy(buf, filename), "mozilla");
        if (!bp)
            bp = buf;

        for (cp = bp; *cp; cp++) {
            if (*cp == '\\')
                *cp = '/';
        }

        filename = bp;
    }
    log_string(fp, filename);
}

static void log_uint32(logfile *fp, uint32 ival)
{
    if (ival < 0x80) {
        
        log_byte(fp, (char) ival);
    } else if (ival < 0x4000) {
        
        log_byte(fp, (char) ((ival >> 8) | 0x80));
        log_byte(fp, (char) (ival & 0xff));
    } else if (ival < 0x200000) {
        
        log_byte(fp, (char) ((ival >> 16) | 0xc0));
        log_byte(fp, (char) ((ival >> 8) & 0xff));
        log_byte(fp, (char) (ival & 0xff));
    } else if (ival < 0x10000000) {
        
        log_byte(fp, (char) ((ival >> 24) | 0xe0));
        log_byte(fp, (char) ((ival >> 16) & 0xff));
        log_byte(fp, (char) ((ival >> 8) & 0xff));
        log_byte(fp, (char) (ival & 0xff));
    } else {
        
        log_byte(fp, (char) 0xf0);
        log_byte(fp, (char) ((ival >> 24) & 0xff));
        log_byte(fp, (char) ((ival >> 16) & 0xff));
        log_byte(fp, (char) ((ival >> 8) & 0xff));
        log_byte(fp, (char) (ival & 0xff));
    }
}

static void log_event1(logfile *fp, char event, uint32 serial)
{
    log_byte(fp, event);
    log_uint32(fp, (uint32) serial);
}

static void log_event2(logfile *fp, char event, uint32 serial, size_t size)
{
    log_event1(fp, event, serial);
    log_uint32(fp, (uint32) size);
}

static void log_event3(logfile *fp, char event, uint32 serial, size_t oldsize,
                       size_t size)
{
    log_event2(fp, event, serial, oldsize);
    log_uint32(fp, (uint32) size);
}

static void log_event4(logfile *fp, char event, uint32 serial, uint32 ui2,
                       uint32 ui3, uint32 ui4)
{
    log_event3(fp, event, serial, ui2, ui3);
    log_uint32(fp, ui4);
}

static void log_event5(logfile *fp, char event, uint32 serial, uint32 ui2,
                       uint32 ui3, uint32 ui4, uint32 ui5)
{
    log_event4(fp, event, serial, ui2, ui3, ui4);
    log_uint32(fp, ui5);
}

static void log_event6(logfile *fp, char event, uint32 serial, uint32 ui2,
                       uint32 ui3, uint32 ui4, uint32 ui5, uint32 ui6)
{
    log_event5(fp, event, serial, ui2, ui3, ui4, ui5);
    log_uint32(fp, ui6);
}

static void log_event7(logfile *fp, char event, uint32 serial, uint32 ui2,
                       uint32 ui3, uint32 ui4, uint32 ui5, uint32 ui6,
                       uint32 ui7)
{
    log_event6(fp, event, serial, ui2, ui3, ui4, ui5, ui6);
    log_uint32(fp, ui7);
}

static void log_event8(logfile *fp, char event, uint32 serial, uint32 ui2,
                       uint32 ui3, uint32 ui4, uint32 ui5, uint32 ui6,
                       uint32 ui7, uint32 ui8)
{
    log_event7(fp, event, serial, ui2, ui3, ui4, ui5, ui6, ui7);
    log_uint32(fp, ui8);
}

typedef struct callsite callsite;

struct callsite {
    void*       pc;
    uint32      serial;
    lfd_set     lfdset;
    const char  *name;    
    const char  *library; 
    int         offset;
    callsite    *parent;
    callsite    *siblings;
    callsite    *kids;
};


static uint32 library_serial_generator = 0;
static uint32 method_serial_generator = 0;
static uint32 callsite_serial_generator = 0;
static uint32 tmstats_serial_generator = 0;
static uint32 filename_serial_generator = 0;


static callsite calltree_root =
  {0, 0, LFD_SET_STATIC_INITIALIZER, NULL, NULL, 0, NULL, NULL, NULL};


static nsTMStats tmstats = NS_TMSTATS_STATIC_INITIALIZER;


static callsite *calltree_maxkids_parent;


static callsite *calltree_maxstack_top;


static callsite *last_callsite_recurrence;

static void log_tmstats(logfile *fp)
{
    log_event1(fp, TM_EVENT_STATS, ++tmstats_serial_generator);
    log_uint32(fp, tmstats.calltree_maxstack);
    log_uint32(fp, tmstats.calltree_maxdepth);
    log_uint32(fp, tmstats.calltree_parents);
    log_uint32(fp, tmstats.calltree_maxkids);
    log_uint32(fp, tmstats.calltree_kidhits);
    log_uint32(fp, tmstats.calltree_kidmisses);
    log_uint32(fp, tmstats.calltree_kidsteps);
    log_uint32(fp, tmstats.callsite_recurrences);
    log_uint32(fp, tmstats.backtrace_calls);
    log_uint32(fp, tmstats.backtrace_failures);
    log_uint32(fp, tmstats.btmalloc_failures);
    log_uint32(fp, tmstats.dladdr_failures);
    log_uint32(fp, tmstats.malloc_calls);
    log_uint32(fp, tmstats.malloc_failures);
    log_uint32(fp, tmstats.calloc_calls);
    log_uint32(fp, tmstats.calloc_failures);
    log_uint32(fp, tmstats.realloc_calls);
    log_uint32(fp, tmstats.realloc_failures);
    log_uint32(fp, tmstats.free_calls);
    log_uint32(fp, tmstats.null_free_calls);
    log_uint32(fp, calltree_maxkids_parent ? calltree_maxkids_parent->serial
                                           : 0);
    log_uint32(fp, calltree_maxstack_top ? calltree_maxstack_top->serial : 0);
}

static void *generic_alloctable(void *pool, PRSize size)
{
    return __libc_malloc(size);
}

static void generic_freetable(void *pool, void *item)
{
    __libc_free(item);
}

typedef struct lfdset_entry {
    PLHashEntry base;
    lfd_set     lfdset;
} lfdset_entry;

static PLHashEntry *lfdset_allocentry(void *pool, const void *key)
{
    lfdset_entry *le = __libc_malloc(sizeof *le);
    if (le)
        LFD_ZERO(&le->lfdset);
    return &le->base;
}

static void lfdset_freeentry(void *pool, PLHashEntry *he, PRUintn flag)
{
    lfdset_entry *le;

    if (flag != HT_FREE_ENTRY)
        return;
    le = (lfdset_entry*) he;
    __libc_free((void*) le);
}

static PLHashAllocOps lfdset_hashallocops = {
    generic_alloctable, generic_freetable,
    lfdset_allocentry,  lfdset_freeentry
};


static PLHashTable *libraries = NULL;


static PLHashTable *filenames = NULL;


static PLHashTable *methods = NULL;

static callsite *calltree(void **stack, size_t num_stack_entries)
{
    logfile *fp = logfp;
    void *pc;
    uint32 nkids;
    callsite *parent, *site, **csp, *tmp;
    int maxstack;
    uint32 library_serial, method_serial, filename_serial;
    const char *library, *method, *filename;
    char *slash;
    PLHashNumber hash;
    PLHashEntry **hep, *he;
    lfdset_entry *le;
    size_t stack_index;
    nsCodeAddressDetails details;
    nsresult rv;

    




    TM_ENTER_LOCK();

    maxstack = (num_stack_entries > tmstats.calltree_maxstack);
    if (maxstack) {
        
        tmstats.calltree_maxstack = num_stack_entries;
        tmstats.calltree_maxdepth = num_stack_entries;
    }

    
    parent = &calltree_root;
    stack_index = num_stack_entries;
    do {
        --stack_index;
        pc = stack[stack_index];

        csp = &parent->kids;
        while ((site = *csp) != NULL) {
            if (site->pc == pc) {
                tmstats.calltree_kidhits++;

                
                *csp = site->siblings;
                site->siblings = parent->kids;
                parent->kids = site;

                
                if (!LFD_TEST(fp->lfd, &site->lfdset)) {
                    





                    break;
                }

                
                goto upward;
            }
            tmstats.calltree_kidsteps++;
            csp = &site->siblings;
        }

        if (!site) {
            tmstats.calltree_kidmisses++;

            
            for (site = parent; site; site = site->parent) {
                if (site->pc == pc) {
                    tmstats.callsite_recurrences++;
                    last_callsite_recurrence = site;
                    goto upward;
                }
            }
        }

        




        











        TM_EXIT_LOCK();
        rv = NS_DescribeCodeAddress(pc, &details);
        TM_ENTER_LOCK();
        if (NS_FAILED(rv)) {
            tmstats.dladdr_failures++;
            goto fail;
        }

        
        library_serial = 0;
        library = NULL;
        if (details.library[0]) {
            if (!libraries) {
                libraries = PL_NewHashTable(100, PL_HashString,
                                            PL_CompareStrings, PL_CompareValues,
                                            &lfdset_hashallocops, NULL);
                if (!libraries) {
                    tmstats.btmalloc_failures++;
                    goto fail;
                }
            }
            hash = PL_HashString(details.library);
            hep = PL_HashTableRawLookup(libraries, hash, details.library);
            he = *hep;
            if (he) {
                library = (char*) he->key;
                library_serial = (uint32) NS_PTR_TO_INT32(he->value);
                le = (lfdset_entry *) he;
                if (LFD_TEST(fp->lfd, &le->lfdset)) {
                    
                    le = NULL;
                }
            } else {
                library = strdup(details.library);
                if (library) {
                    library_serial = ++library_serial_generator;
                    he = PL_HashTableRawAdd(libraries, hep, hash, library,
                                            (void*) library_serial);
                }
                if (!he) {
                    tmstats.btmalloc_failures++;
                    goto fail;
                }
                le = (lfdset_entry *) he;
            }
            if (le) {
                
                slash = strrchr(library, '/');
                log_event1(fp, TM_EVENT_LIBRARY, library_serial);
                log_string(fp, slash ? slash + 1 : library);
                LFD_SET(fp->lfd, &le->lfdset);
            }
        }

        


        filename_serial = 0;
        filename = details.filename[0] ? details.filename : "noname";
        if (!filenames) {
            filenames = PL_NewHashTable(100, PL_HashString,
                                        PL_CompareStrings, PL_CompareValues,
                                        &lfdset_hashallocops, NULL);
            if (!filenames) {
                tmstats.btmalloc_failures++;
                return NULL;
            }
        }
        hash = PL_HashString(filename);
        hep = PL_HashTableRawLookup(filenames, hash, filename);
        he = *hep;
        if (he) {
            filename = (char*) he->key;
            filename_serial = (uint32) NS_PTR_TO_INT32(he->value);
            le = (lfdset_entry *) he;
            if (LFD_TEST(fp->lfd, &le->lfdset)) {
                
                le = NULL;
            }
        } else {
            filename = strdup(filename);
            if (filename) {
                filename_serial = ++filename_serial_generator;
                he = PL_HashTableRawAdd(filenames, hep, hash, filename,
                                        (void*) filename_serial);
            }
            if (!he) {
                tmstats.btmalloc_failures++;
                return NULL;
            }
            le = (lfdset_entry *) he;
        }
        if (le) {
            
            log_event1(fp, TM_EVENT_FILENAME, filename_serial);
            log_filename(fp, filename);
            LFD_SET(fp->lfd, &le->lfdset);
        }

        if (!details.function[0]) {
            PR_snprintf(details.function, sizeof(details.function),
                        "%s+%X", library ? library : "main", details.loffset);
        }

        
        method_serial = 0;
        if (!methods) {
            methods = PL_NewHashTable(10000, PL_HashString,
                                      PL_CompareStrings, PL_CompareValues,
                                      &lfdset_hashallocops, NULL);
            if (!methods) {
                tmstats.btmalloc_failures++;
                goto fail;
            }
        }
        hash = PL_HashString(details.function);
        hep = PL_HashTableRawLookup(methods, hash, details.function);
        he = *hep;
        if (he) {
            method = (char*) he->key;
            method_serial = (uint32) NS_PTR_TO_INT32(he->value);
            le = (lfdset_entry *) he;
            if (LFD_TEST(fp->lfd, &le->lfdset)) {
                
                le = NULL;
            }
        } else {
            method = strdup(details.function);
            if (method) {
                method_serial = ++method_serial_generator;
                he = PL_HashTableRawAdd(methods, hep, hash, method,
                                        (void*) method_serial);
            }
            if (!he) {
                tmstats.btmalloc_failures++;
                return NULL;
            }
            le = (lfdset_entry *) he;
        }
        if (le) {
            log_event4(fp, TM_EVENT_METHOD, method_serial, library_serial,
                       filename_serial, details.lineno);
            log_string(fp, method);
            LFD_SET(fp->lfd, &le->lfdset);
        }

        
        if (!site) {
            site = __libc_malloc(sizeof(callsite));
            if (!site) {
                tmstats.btmalloc_failures++;
                goto fail;
            }

            
            if (!parent->kids)
                tmstats.calltree_parents++;
            nkids = 1;
            for (tmp = parent->kids; tmp; tmp = tmp->siblings)
                nkids++;
            if (nkids > tmstats.calltree_maxkids) {
                tmstats.calltree_maxkids = nkids;
                calltree_maxkids_parent = parent;
            }

            
            site->pc = pc;
            site->serial = ++callsite_serial_generator;
            LFD_ZERO(&site->lfdset);
            site->name = method;
            site->library = library;
            site->offset = details.loffset;
            site->parent = parent;
            site->siblings = parent->kids;
            parent->kids = site;
            site->kids = NULL;
        }

        
        log_event4(fp, TM_EVENT_CALLSITE, site->serial, parent->serial,
                   method_serial, details.foffset);
        LFD_SET(fp->lfd, &site->lfdset);

      upward:
        parent = site;
    } while (stack_index > 0);

    if (maxstack)
        calltree_maxstack_top = site;

    TM_EXIT_LOCK();

    return site;
  fail:
    TM_EXIT_LOCK();
    return NULL;
}




PR_STATIC_CALLBACK(void)
stack_callback(void *pc, void *closure)
{
    stack_buffer_info *info = (stack_buffer_info*) closure;

    



    if (info->entries < info->size)
        info->buffer[info->entries] = pc;
    ++info->entries;
}





callsite *
backtrace(tm_thread *t, int skip)
{
    callsite *site;
    stack_buffer_info *info = &t->backtrace_buf;
    void ** new_stack_buffer;
    size_t new_stack_buffer_size;

    t->suppress_tracing++;

    







    
    
    info->entries = 0;
    NS_StackWalk(stack_callback, skip, info);

    





    if (info->entries > info->size) {
        new_stack_buffer_size = 2 * info->entries;
        new_stack_buffer = __libc_realloc(info->buffer,
                               new_stack_buffer_size * sizeof(void*));
        if (!new_stack_buffer)
            return NULL;
        info->buffer = new_stack_buffer;
        info->size = new_stack_buffer_size;

        
        info->entries = 0;
        NS_StackWalk(stack_callback, skip, info);

        PR_ASSERT(info->entries * 2 == new_stack_buffer_size); 
    }

    if (info->entries == 0) {
        t->suppress_tracing--;
        return NULL;
    }

    site = calltree(info->buffer, info->entries);

    TM_ENTER_LOCK();
    tmstats.backtrace_calls++;
    if (!site) {
        tmstats.backtrace_failures++;
        PR_ASSERT(tmstats.backtrace_failures < 100);
    }
    TM_EXIT_LOCK();

    t->suppress_tracing--;
    return site;
}

typedef struct allocation {
    PLHashEntry entry;
    size_t      size;
    FILE        *trackfp;       
} allocation;

#define ALLOC_HEAP_SIZE 150000

static allocation alloc_heap[ALLOC_HEAP_SIZE];
static allocation *alloc_freelist = NULL;
static int alloc_heap_initialized = 0;

static PLHashEntry *alloc_allocentry(void *pool, const void *key)
{
    allocation **listp, *alloc;
    int n;

    if (!alloc_heap_initialized) {
        n = ALLOC_HEAP_SIZE;
        listp = &alloc_freelist;
        for (alloc = alloc_heap; --n >= 0; alloc++) {
            *listp = alloc;
            listp = (allocation**) &alloc->entry.next;
        }
        *listp = NULL;
        alloc_heap_initialized = 1;
    }

    listp = &alloc_freelist;
    alloc = *listp;
    if (!alloc)
        return __libc_malloc(sizeof(allocation));
    *listp = (allocation*) alloc->entry.next;
    return &alloc->entry;
}

static void alloc_freeentry(void *pool, PLHashEntry *he, PRUintn flag)
{
    allocation *alloc;

    if (flag != HT_FREE_ENTRY)
        return;
    alloc = (allocation*) he;
    if ((PRUptrdiff)(alloc - alloc_heap) < (PRUptrdiff)ALLOC_HEAP_SIZE) {
        alloc->entry.next = &alloc_freelist->entry;
        alloc_freelist = alloc;
    } else {
        __libc_free((void*) alloc);
    }
}

static PLHashAllocOps alloc_hashallocops = {
    generic_alloctable, generic_freetable,
    alloc_allocentry,   alloc_freeentry
};

static PLHashNumber hash_pointer(const void *key)
{
    return (PLHashNumber) key;
}

static PLHashTable *allocations = NULL;

static PLHashTable *new_allocations(void)
{
    allocations = PL_NewHashTable(200000, hash_pointer,
                                  PL_CompareValues, PL_CompareValues,
                                  &alloc_hashallocops, NULL);
    return allocations;
}

#define get_allocations() (allocations ? allocations : new_allocations())

#ifdef XP_UNIX

NS_EXTERNAL_VIS_(__ptr_t)
malloc(size_t size)
{
    PRUint32 start, end;
    __ptr_t ptr;
    callsite *site;
    PLHashEntry *he;
    allocation *alloc;
    tm_thread *t;

    if (!tracing_enabled || !PR_Initialized() ||
        (t = tm_get_thread())->suppress_tracing != 0) {
        return __libc_malloc(size);
    }

    start = PR_IntervalNow();
    ptr = __libc_malloc(size);
    end = PR_IntervalNow();

    site = backtrace(t, 1);

    t->suppress_tracing++;
    TM_ENTER_LOCK();
    tmstats.malloc_calls++;
    if (!ptr) {
        tmstats.malloc_failures++;
    } else {
        if (site)
            log_event5(logfp, TM_EVENT_MALLOC,
                       site->serial, start, end - start,
                       (uint32)NS_PTR_TO_INT32(ptr), size);
        if (get_allocations()) {
            he = PL_HashTableAdd(allocations, ptr, site);
            if (he) {
                alloc = (allocation*) he;
                alloc->size = size;
                alloc->trackfp = NULL;
            }
        }
    }
    TM_EXIT_LOCK();
    t->suppress_tracing--;

    return ptr;
}

NS_EXTERNAL_VIS_(__ptr_t)
calloc(size_t count, size_t size)
{
    PRUint32 start, end;
    __ptr_t ptr;
    callsite *site;
    PLHashEntry *he;
    allocation *alloc;
    tm_thread *t;

    










    if (!tracing_enabled || !PR_Initialized() ||
        (t = tm_get_thread())->suppress_tracing != 0) {
        return __libc_calloc(count, size);
    }

    start = PR_IntervalNow();
    ptr = __libc_calloc(count, size);
    end = PR_IntervalNow();

    site = backtrace(t, 1);

    t->suppress_tracing++;
    TM_ENTER_LOCK();
    tmstats.calloc_calls++;
    if (!ptr) {
        tmstats.calloc_failures++;
    } else {
        size *= count;
        if (site) {
            log_event5(logfp, TM_EVENT_CALLOC,
                       site->serial, start, end - start,
                       (uint32)NS_PTR_TO_INT32(ptr), size);
        }
        if (get_allocations()) {
            he = PL_HashTableAdd(allocations, ptr, site);
            if (he) {
                alloc = (allocation*) he;
                alloc->size = size;
                alloc->trackfp = NULL;
            }
        }
    }
    TM_EXIT_LOCK();
    t->suppress_tracing--;
    return ptr;
}

NS_EXTERNAL_VIS_(__ptr_t)
realloc(__ptr_t ptr, size_t size)
{
    PRUint32 start, end;
    __ptr_t oldptr;
    callsite *oldsite, *site;
    size_t oldsize;
    PLHashNumber hash;
    PLHashEntry **hep, *he;
    allocation *alloc;
    FILE *trackfp = NULL;
    tm_thread *t;

    if (!tracing_enabled || !PR_Initialized() ||
        (t = tm_get_thread())->suppress_tracing != 0) {
        return __libc_realloc(ptr, size);
    }

    t->suppress_tracing++;
    TM_ENTER_LOCK();
    tmstats.realloc_calls++;
    if (PR_TRUE) {
        oldptr = ptr;
        oldsite = NULL;
        oldsize = 0;
        he = NULL;
        if (oldptr && get_allocations()) {
            hash = hash_pointer(oldptr);
            hep = PL_HashTableRawLookup(allocations, hash, oldptr);
            he = *hep;
            if (he) {
                oldsite = (callsite*) he->value;
                alloc = (allocation*) he;
                oldsize = alloc->size;
                trackfp = alloc->trackfp;
                if (trackfp) {
                    fprintf(alloc->trackfp,
                            "\nrealloc(%p, %lu), oldsize %lu, alloc site %p\n",
                            (void*) ptr, (unsigned long) size,
                            (unsigned long) oldsize, (void*) oldsite);
                    NS_TraceStack(1, trackfp);
                }
            }
        }
    }
    TM_EXIT_LOCK();
    t->suppress_tracing--;

    start = PR_IntervalNow();
    ptr = __libc_realloc(ptr, size);
    end = PR_IntervalNow();

    site = backtrace(t, 1);

    t->suppress_tracing++;
    TM_ENTER_LOCK();
    if (!ptr && size) {
        



        tmstats.realloc_failures++;
    } else {
        if (site) {
            log_event8(logfp, TM_EVENT_REALLOC,
                       site->serial, start, end - start,
                       (uint32)NS_PTR_TO_INT32(ptr), size,
                       oldsite ? oldsite->serial : 0,
                       (uint32)NS_PTR_TO_INT32(oldptr), oldsize);
        }
        if (ptr && allocations) {
            if (ptr != oldptr) {
                




                if (he)
                    PL_HashTableRemove(allocations, oldptr);

                
                he = PL_HashTableAdd(allocations, ptr, site);
            } else {
                



                if (!he)
                    he = PL_HashTableAdd(allocations, ptr, site);
            }
            if (he) {
                alloc = (allocation*) he;
                alloc->size = size;
                alloc->trackfp = trackfp;
            }
        }
    }
    TM_EXIT_LOCK();
    t->suppress_tracing--;
    return ptr;
}

NS_EXTERNAL_VIS_(void*)
valloc(size_t size)
{
    PRUint32 start, end;
    __ptr_t ptr;
    callsite *site;
    PLHashEntry *he;
    allocation *alloc;
    tm_thread *t;

    if (!tracing_enabled || !PR_Initialized() ||
        (t = tm_get_thread())->suppress_tracing != 0) {
        return __libc_valloc(size);
    }

    start = PR_IntervalNow();
    ptr = __libc_valloc(size);
    end = PR_IntervalNow();

    site = backtrace(t, 1);

    t->suppress_tracing++;
    TM_ENTER_LOCK();
    tmstats.malloc_calls++; 
    if (!ptr) {
        tmstats.malloc_failures++; 
    } else {
        if (site)
            log_event5(logfp, TM_EVENT_MALLOC, 
                       site->serial, start, end - start,
                       (uint32)NS_PTR_TO_INT32(ptr), size);
        if (get_allocations()) {
            he = PL_HashTableAdd(allocations, ptr, site);
            if (he) {
                alloc = (allocation*) he;
                alloc->size = size;
                alloc->trackfp = NULL;
            }
        }
    }
    TM_EXIT_LOCK();
    t->suppress_tracing--;
    return ptr;
}

NS_EXTERNAL_VIS_(void*)
memalign(size_t boundary, size_t size)
{
    PRUint32 start, end;
    __ptr_t ptr;
    callsite *site;
    PLHashEntry *he;
    allocation *alloc;
    tm_thread *t;

    if (!tracing_enabled || !PR_Initialized() ||
        (t = tm_get_thread())->suppress_tracing != 0) {
        return __libc_memalign(boundary, size);
    }

    start = PR_IntervalNow();
    ptr = __libc_memalign(boundary, size);
    end = PR_IntervalNow();

    site = backtrace(t, 1);

    t->suppress_tracing++;
    TM_ENTER_LOCK();
    tmstats.malloc_calls++; 
    if (!ptr) {
        tmstats.malloc_failures++; 
    } else {
        if (site) {
            log_event5(logfp, TM_EVENT_MALLOC, 
                       site->serial, start, end - start,
                       (uint32)NS_PTR_TO_INT32(ptr), size);
        }
        if (get_allocations()) {
            he = PL_HashTableAdd(allocations, ptr, site);
            if (he) {
                alloc = (allocation*) he;
                alloc->size = size;
                alloc->trackfp = NULL;
            }
        }
    }
    TM_EXIT_LOCK();
    t->suppress_tracing--;
    return ptr;
}

NS_EXTERNAL_VIS_(int)
posix_memalign(void **memptr, size_t alignment, size_t size)
{
    __ptr_t ptr = memalign(alignment, size);
    if (!ptr)
        return ENOMEM;
    *memptr = ptr;
    return 0;
}

NS_EXTERNAL_VIS_(void)
free(__ptr_t ptr)
{
    PLHashEntry **hep, *he;
    callsite *site;
    allocation *alloc;
    uint32 serial = 0, size = 0;
    PRUint32 start, end;
    tm_thread *t;

    if (!tracing_enabled || !PR_Initialized() ||
        (t = tm_get_thread())->suppress_tracing != 0) {
        __libc_free(ptr);
        return;
    }

    t->suppress_tracing++;
    TM_ENTER_LOCK();
    tmstats.free_calls++;
    if (!ptr) {
        tmstats.null_free_calls++;
    } else {
        if (get_allocations()) {
            hep = PL_HashTableRawLookup(allocations, hash_pointer(ptr), ptr);
            he = *hep;
            if (he) {
                site = (callsite*) he->value;
                if (site) {
                    alloc = (allocation*) he;
                    serial = site->serial;
                    size = alloc->size;
                    if (alloc->trackfp) {
                        fprintf(alloc->trackfp, "\nfree(%p), alloc site %p\n",
                                (void*) ptr, (void*) site);
                        NS_TraceStack(1, alloc->trackfp);
                    }
                }
                PL_HashTableRawRemove(allocations, hep, he);
            }
        }
    }
    TM_EXIT_LOCK();
    t->suppress_tracing--;

    start = PR_IntervalNow();
    __libc_free(ptr);
    end = PR_IntervalNow();

    if (size != 0) {
        t->suppress_tracing++;
        TM_ENTER_LOCK();
        log_event5(logfp, TM_EVENT_FREE,
                   serial, start, end - start,
                   (uint32)NS_PTR_TO_INT32(ptr), size);
        TM_EXIT_LOCK();
        t->suppress_tracing--;
    }
}

NS_EXTERNAL_VIS_(void)
cfree(void *ptr)
{
    free(ptr);
}

#endif 

static const char magic[] = NS_TRACE_MALLOC_MAGIC;

static void
log_header(int logfd)
{
    uint32 ticksPerSec = PR_htonl(PR_TicksPerSecond());
    (void) write(logfd, magic, NS_TRACE_MALLOC_MAGIC_SIZE);
    (void) write(logfd, &ticksPerSec, sizeof ticksPerSec);
}

PR_IMPLEMENT(void) NS_TraceMallocStartup(int logfd)
{
    
    PR_ASSERT(tracing_enabled == 1);
    PR_ASSERT(logfp == &default_logfile);
    tracing_enabled = (logfd >= 0);

    if (tracing_enabled) {
        PR_ASSERT(logfp->simsize == 0); 

        
        logfp->fd = logfd;
        logfile_list = &default_logfile;
        logfp->prevp = &logfile_list;
        logfile_tail = &logfp->next;
        log_header(logfd);
    }

    atexit(NS_TraceMallocShutdown);

    





    main_thread.suppress_tracing++;
    TM_CREATE_TLS_INDEX(tls_index);
    TM_SET_TLS_DATA(tls_index, &main_thread);
    tmlock = PR_NewLock();
    main_thread.suppress_tracing--;

#ifdef XP_WIN32
    
    if (tracing_enabled) {
        StartupHooker();
    }
#endif
}







static const char TMLOG_OPTION[] = "--trace-malloc";
static const char SDLOG_OPTION[] = "--shutdown-leaks";

#define SHOULD_PARSE_ARG(name_, log_, arg_) \
    (0 == strncmp(arg_, name_, sizeof(name_) - 1))

#define PARSE_ARG(name_, log_, argv_, i_, consumed_)                          \
    PR_BEGIN_MACRO                                                            \
        char _nextchar = argv_[i_][sizeof(name_) - 1];                        \
        if (_nextchar == '=') {                                               \
            log_ = argv_[i_] + sizeof(name_);                                 \
            consumed_ = 1;                                                    \
        } else if (_nextchar == '\0') {                                       \
            log_ = argv_[i_+1];                                               \
            consumed_ = 2;                                                    \
        }                                                                     \
    PR_END_MACRO

PR_IMPLEMENT(int) NS_TraceMallocStartupArgs(int argc, char* argv[])
{
    int i, logfd = -1, consumed, logflags;
    char *tmlogname = NULL; 

    




    for (i = 1; i < argc; i += consumed) {
        consumed = 0;
        if (SHOULD_PARSE_ARG(TMLOG_OPTION, tmlogname, argv[i]))
            PARSE_ARG(TMLOG_OPTION, tmlogname, argv, i, consumed);
        else if (SHOULD_PARSE_ARG(SDLOG_OPTION, sdlogname, argv[i]))
            PARSE_ARG(SDLOG_OPTION, sdlogname, argv, i, consumed);

        if (consumed) {
#ifndef XP_WIN32 
            int j;
            
            argc -= consumed;
            for (j = i; j < argc; ++j)
                argv[j] = argv[j+consumed];
            argv[argc] = NULL;
            consumed = 0; 
#endif
        } else {
            consumed = 1;
        }
    }

    if (tmlogname) {
#ifdef XP_UNIX
        int pipefds[2];
#endif

        switch (*tmlogname) {
#ifdef XP_UNIX
          case '|':
            if (pipe(pipefds) == 0) {
                pid_t pid = fork();
                if (pid == 0) {
                    
                    int maxargc, nargc;
                    char **nargv, *token;

                    if (pipefds[0] != 0) {
                        dup2(pipefds[0], 0);
                        close(pipefds[0]);
                    }
                    close(pipefds[1]);

                    tmlogname = strtok(tmlogname + 1, " \t");
                    maxargc = 3;
                    nargv = (char **) malloc((maxargc+1) * sizeof(char *));
                    if (!nargv) exit(1);
                    nargc = 0;
                    nargv[nargc++] = tmlogname;
                    while ((token = strtok(NULL, " \t")) != NULL) {
                        if (nargc == maxargc) {
                            maxargc *= 2;
                            nargv = (char**)
                                realloc(nargv, (maxargc+1) * sizeof(char*));
                            if (!nargv) exit(1);
                        }
                        nargv[nargc++] = token;
                    }
                    nargv[nargc] = NULL;

                    (void) setsid();
                    execvp(tmlogname, nargv);
                    exit(127);
                }

                if (pid > 0) {
                    
                    close(pipefds[0]);
                    logfd = pipefds[1];
                }
            }
            if (logfd < 0) {
                fprintf(stderr,
                    "%s: can't pipe to trace-malloc child process %s: %s\n",
                    argv[0], tmlogname, strerror(errno));
                exit(1);
            }
            break;
#endif 
          case '-':
            
            
            if (tmlogname[1] == '\0')
                break;
            

          default:
            logflags = O_CREAT | O_WRONLY | O_TRUNC;
#if defined(XP_WIN32)
            


            logflags |= O_BINARY;
#endif
            logfd = open(tmlogname, logflags, 0644);
            if (logfd < 0) {
                fprintf(stderr,
                    "%s: can't create trace-malloc log named %s: %s\n",
                    argv[0], tmlogname, strerror(errno));
                exit(1);
            }
            break;
        }
    }

    NS_TraceMallocStartup(logfd);
    return argc;
}

PR_IMPLEMENT(void) NS_TraceMallocShutdown()
{
    logfile *fp;

    if (sdlogname)
        NS_TraceMallocDumpAllocations(sdlogname);

    if (tmstats.backtrace_failures) {
        fprintf(stderr,
                "TraceMalloc backtrace failures: %lu (malloc %lu dladdr %lu)\n",
                (unsigned long) tmstats.backtrace_failures,
                (unsigned long) tmstats.btmalloc_failures,
                (unsigned long) tmstats.dladdr_failures);
    }
    while ((fp = logfile_list) != NULL) {
        logfile_list = fp->next;
        log_tmstats(fp);
        flush_logfile(fp);
        if (fp->fd >= 0) {
            close(fp->fd);
            fp->fd = -1;
        }
        if (fp != &default_logfile) {
            if (fp == logfp)
                logfp = &default_logfile;
            free((void*) fp);
        }
    }
    if (tmlock) {
        PRLock *lock = tmlock;
        tmlock = NULL;
        PR_DestroyLock(lock);
    }
#ifdef XP_WIN32
    if (tracing_enabled) {
        ShutdownHooker();
    }
#endif
}

PR_IMPLEMENT(void) NS_TraceMallocDisable()
{
    logfile *fp;
    tm_thread *t = tm_get_thread();

    t->suppress_tracing++;
    TM_ENTER_LOCK();
    for (fp = logfile_list; fp; fp = fp->next)
        flush_logfile(fp);
    tracing_enabled = 0;
    TM_EXIT_LOCK();
    t->suppress_tracing--;
}

PR_IMPLEMENT(void) NS_TraceMallocEnable()
{
    tm_thread *t = tm_get_thread();

    t->suppress_tracing++;
    TM_ENTER_LOCK();
    tracing_enabled = 1;
    TM_EXIT_LOCK();
    t->suppress_tracing--;
}

PR_IMPLEMENT(int) NS_TraceMallocChangeLogFD(int fd)
{
    logfile *oldfp, *fp;
    struct stat sb;
    tm_thread *t = tm_get_thread();

    t->suppress_tracing++;
    TM_ENTER_LOCK();
    oldfp = logfp;
    if (oldfp->fd != fd) {
        flush_logfile(oldfp);
        fp = get_logfile(fd);
        if (!fp) {
            TM_EXIT_LOCK();
            t->suppress_tracing--;
            return -2;
        }
        if (fd >= 0 && fstat(fd, &sb) == 0 && sb.st_size == 0)
            log_header(fd);
        logfp = fp;
    }
    TM_EXIT_LOCK();
    t->suppress_tracing--;
    return oldfp->fd;
}

static PRIntn
lfd_clr_enumerator(PLHashEntry *he, PRIntn i, void *arg)
{
    lfdset_entry *le = (lfdset_entry*) he;
    logfile *fp = (logfile*) arg;

    LFD_CLR(fp->lfd, &le->lfdset);
    return HT_ENUMERATE_NEXT;
}

static void
lfd_clr_walk(callsite *site, logfile *fp)
{
    callsite *kid;

    LFD_CLR(fp->lfd, &site->lfdset);
    for (kid = site->kids; kid; kid = kid->siblings)
        lfd_clr_walk(kid, fp);
}

PR_IMPLEMENT(void)
NS_TraceMallocCloseLogFD(int fd)
{
    logfile *fp;
    tm_thread *t = tm_get_thread();

    t->suppress_tracing++;
    TM_ENTER_LOCK();

    fp = get_logfile(fd);
    if (fp) {
        flush_logfile(fp);
        if (fp == &default_logfile) {
            
            fp->fd = -1;

            
            PR_ASSERT(fp->lfd == 0);
        } else {
            
            PL_HashTableEnumerateEntries(libraries, lfd_clr_enumerator, fp);
            PL_HashTableEnumerateEntries(methods, lfd_clr_enumerator, fp);
            lfd_clr_walk(&calltree_root, fp);

            
            *fp->prevp = fp->next;
            if (!fp->next) {
                PR_ASSERT(logfile_tail == &fp->next);
                logfile_tail = fp->prevp;
            }

            
            if (fp == logfp)
                logfp = &default_logfile;
            free((void*) fp);
        }
    }

    TM_EXIT_LOCK();
    t->suppress_tracing--;
    close(fd);
}

PR_IMPLEMENT(void)
NS_TraceMallocLogTimestamp(const char *caption)
{
    logfile *fp;
#ifdef XP_UNIX
    struct timeval tv;
#endif
#ifdef XP_WIN32
    struct _timeb tb;
#endif
    tm_thread *t = tm_get_thread();

    t->suppress_tracing++;
    TM_ENTER_LOCK();

    fp = logfp;
    log_byte(fp, TM_EVENT_TIMESTAMP);

#ifdef XP_UNIX
    gettimeofday(&tv, NULL);
    log_uint32(fp, (uint32) tv.tv_sec);
    log_uint32(fp, (uint32) tv.tv_usec);
#endif
#ifdef XP_WIN32
    _ftime(&tb);
    log_uint32(fp, (uint32) tb.time);
    log_uint32(fp, (uint32) tb.millitm);
#endif
    log_string(fp, caption);

    TM_EXIT_LOCK();
    t->suppress_tracing--;
}

static PRIntn
allocation_enumerator(PLHashEntry *he, PRIntn i, void *arg)
{
    allocation *alloc = (allocation*) he;
    FILE *ofp = (FILE*) arg;
    callsite *site = (callsite*) he->value;

    extern const char* nsGetTypeName(const void* ptr);
    unsigned long *p, *end;

    fprintf(ofp, "%p <%s> (%lu)\n",
            he->key,
            nsGetTypeName(he->key),
            (unsigned long) alloc->size);

    for (p   = (unsigned long*) he->key,
         end = (unsigned long*) ((char*)he->key + alloc->size);
         p < end; ++p)
        fprintf(ofp, "\t0x%08lX\n", *p);

    while (site) {
        if (site->name || site->parent) {
            fprintf(ofp, "%s[%s +0x%X]\n",
                    site->name, site->library, site->offset);
        }
        site = site->parent;
    }
    fputc('\n', ofp);
    return HT_ENUMERATE_NEXT;
}

PR_IMPLEMENT(void)
NS_TraceStack(int skip, FILE *ofp)
{
    callsite *site;
    tm_thread *t = tm_get_thread();

    site = backtrace(t, skip + 1);
    while (site) {
        if (site->name || site->parent) {
            fprintf(ofp, "%s[%s +0x%X]\n",
                    site->name, site->library, site->offset);
        }
        site = site->parent;
    }
}

PR_IMPLEMENT(int)
NS_TraceMallocDumpAllocations(const char *pathname)
{
    FILE *ofp;
    int rv;
    ofp = fopen(pathname, WRITE_FLAGS);
    if (!ofp)
        return -1;
    if (allocations)
        PL_HashTableEnumerateEntries(allocations, allocation_enumerator, ofp);
    rv = ferror(ofp) ? -1 : 0;
    fclose(ofp);
    return rv;
}

PR_IMPLEMENT(void)
NS_TraceMallocFlushLogfiles()
{
    logfile *fp;
    tm_thread *t = tm_get_thread();

    t->suppress_tracing++;
    TM_ENTER_LOCK();

    for (fp = logfile_list; fp; fp = fp->next)
        flush_logfile(fp);

    TM_EXIT_LOCK();
    t->suppress_tracing--;
}

PR_IMPLEMENT(void)
NS_TrackAllocation(void* ptr, FILE *ofp)
{
    PLHashEntry **hep;
    allocation *alloc;
    tm_thread *t = tm_get_thread();

    fprintf(ofp, "Trying to track %p\n", (void*) ptr);
    setlinebuf(ofp);

    t->suppress_tracing++;
    TM_ENTER_LOCK();
    if (get_allocations()) {
        hep = PL_HashTableRawLookup(allocations, hash_pointer(ptr), ptr);
        alloc = (allocation*) *hep;
        if (alloc) {
            fprintf(ofp, "Tracking %p\n", (void*) ptr);
            alloc->trackfp = ofp;
        } else {
            fprintf(ofp, "Not tracking %p\n", (void*) ptr);
        }
    }
    TM_EXIT_LOCK();
    t->suppress_tracing--;
}

#ifdef XP_WIN32

PR_IMPLEMENT(void)
MallocCallback(void *ptr, size_t size, PRUint32 start, PRUint32 end, tm_thread *t)
{
    callsite *site;
    PLHashEntry *he;
    allocation *alloc;

    if (!tracing_enabled || t->suppress_tracing != 0)
        return;

    site = backtrace(t, 2);

    t->suppress_tracing++;
    TM_ENTER_LOCK();
    tmstats.malloc_calls++;
    if (!ptr) {
        tmstats.malloc_failures++;
    } else {
        if (site)
            log_event5(logfp, TM_EVENT_MALLOC,
                       site->serial, start, end - start,
                       (uint32)NS_PTR_TO_INT32(ptr), size);
        if (get_allocations()) {
            he = PL_HashTableAdd(allocations, ptr, site);
            if (he) {
                alloc = (allocation*) he;
                alloc->size = size;
            }
        }
    }
    TM_EXIT_LOCK();
    t->suppress_tracing--;
}

PR_IMPLEMENT(void)
CallocCallback(void *ptr, size_t count, size_t size, PRUint32 start, PRUint32 end, tm_thread *t)
{
    callsite *site;
    PLHashEntry *he;
    allocation *alloc;

    if (!tracing_enabled || t->suppress_tracing != 0)
        return;

    site = backtrace(t, 2);

    t->suppress_tracing++;
    TM_ENTER_LOCK();
    tmstats.calloc_calls++;
    if (!ptr) {
        tmstats.calloc_failures++;
    } else {
        size *= count;
        if (site)
            log_event5(logfp, TM_EVENT_CALLOC,
                       site->serial, start, end - start,
                       (uint32)NS_PTR_TO_INT32(ptr), size);
        if (get_allocations()) {
            he = PL_HashTableAdd(allocations, ptr, site);
            if (he) {
                alloc = (allocation*) he;
                alloc->size = size;
            }
        }
    }
    TM_EXIT_LOCK();
    t->suppress_tracing--;
}

PR_IMPLEMENT(void)
ReallocCallback(void * oldptr, void *ptr, size_t size, PRUint32 start, PRUint32 end, tm_thread *t)
{
    callsite *oldsite, *site;
    size_t oldsize;
    PLHashNumber hash;
    PLHashEntry **hep, *he;
    allocation *alloc;

    if (!tracing_enabled || t->suppress_tracing != 0)
        return;

    site = backtrace(t, 2);

    t->suppress_tracing++;
    TM_ENTER_LOCK();
    tmstats.realloc_calls++;
    if (PR_TRUE) {
        oldsite = NULL;
        oldsize = 0;
        he = NULL;
        if (oldptr && get_allocations()) {
            hash = hash_pointer(oldptr);
            hep = PL_HashTableRawLookup(allocations, hash, oldptr);
            he = *hep;
            if (he) {
                oldsite = (callsite*) he->value;
                alloc = (allocation*) he;
                oldsize = alloc->size;
            }
        }
    }
    if (!ptr && size) {
        tmstats.realloc_failures++;

        



    } else {
        if (site) {
            log_event8(logfp, TM_EVENT_REALLOC,
                       site->serial, start, end - start,
                       (uint32)NS_PTR_TO_INT32(ptr), size,
                       oldsite ? oldsite->serial : 0,
                       (uint32)NS_PTR_TO_INT32(oldptr), oldsize);
        }
        if (ptr && allocations) {
            if (ptr != oldptr) {
                



                if (he)
                    PL_HashTableRawRemove(allocations, hep, he);

                
                he = PL_HashTableAdd(allocations, ptr, site);
            } else {
                



                if (!he)
                    he = PL_HashTableAdd(allocations, ptr, site);
            }
            if (he) {
                alloc = (allocation*) he;
                alloc->size = size;
            }
        }
    }
    TM_EXIT_LOCK();
    t->suppress_tracing--;
}

PR_IMPLEMENT(void)
FreeCallback(void * ptr, PRUint32 start, PRUint32 end, tm_thread *t)
{
    PLHashEntry **hep, *he;
    callsite *site;
    allocation *alloc;

    if (!tracing_enabled || t->suppress_tracing != 0)
        return;

    t->suppress_tracing++;
    TM_ENTER_LOCK();
    tmstats.free_calls++;
    if (!ptr) {
        tmstats.null_free_calls++;
    } else {
        if (get_allocations()) {
            hep = PL_HashTableRawLookup(allocations, hash_pointer(ptr), ptr);
            he = *hep;
            if (he) {
                site = (callsite*) he->value;
                if (site) {
                    alloc = (allocation*) he;
                    log_event5(logfp, TM_EVENT_FREE,
                               site->serial, start, end - start,
                               (uint32)NS_PTR_TO_INT32(ptr), alloc->size);
                }
                PL_HashTableRawRemove(allocations, hep, he);
            }
        }
    }
    TM_EXIT_LOCK();
    t->suppress_tracing--;
}

#endif 

#endif 
