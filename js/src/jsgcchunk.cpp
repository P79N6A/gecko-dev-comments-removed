
































#include <stdlib.h>
#include "jstypes.h"
#include "jsstdint.h"
#include "jsgcchunk.h"

#ifdef XP_WIN
# include "jswin.h"

# ifdef _MSC_VER
#  pragma warning( disable: 4267 4996 4146 )
# endif

#elif defined(XP_OS2)

# define INCL_DOSMEMMGR
# include <os2.h>

#elif defined(XP_MACOSX) || defined(DARWIN)

# include <libkern/OSAtomic.h>
# include <mach/mach_error.h>
# include <mach/mach_init.h>
# include <mach/vm_map.h>
# include <malloc/malloc.h>

#elif defined(XP_UNIX)

# include <unistd.h>
# include <sys/mman.h>

# ifndef MAP_NOSYNC
#  define MAP_NOSYNC    0
# endif

#endif

#ifdef XP_WIN

static void *
MapPages(void *addr, size_t size)
{
    void *p = VirtualAlloc(addr, size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    JS_ASSERT_IF(p && addr, p == addr);
    return p;
}

static void
UnmapPages(void *addr, size_t size)
{
    JS_ALWAYS_TRUE(VirtualFree(addr, 0, MEM_RELEASE));
}

#elif defined(XP_OS2)

#define JS_GC_HAS_MAP_ALIGN 1
#define OS2_MAX_RECURSIONS  16

static void
UnmapPages(void *addr, size_t size)
{
    if (!DosFreeMem(addr))
        return;

    


    unsigned long cb = 2 * size;
    unsigned long flags;
    if (DosQueryMem(addr, &cb, &flags) || cb < size)
        return;

    jsuword base = reinterpret_cast<jsuword>(addr) - ((2 * size) - cb);
    DosFreeMem(reinterpret_cast<void*>(base));

    return;
}

static void *
MapAlignedPagesRecursively(size_t size, size_t alignment, int& recursions)
{
    if (++recursions >= OS2_MAX_RECURSIONS)
        return NULL;

    void *tmp;
    if (DosAllocMem(&tmp, size,
                    OBJ_ANY | PAG_COMMIT | PAG_READ | PAG_WRITE)) {
        JS_ALWAYS_TRUE(DosAllocMem(&tmp, size,
                                   PAG_COMMIT | PAG_READ | PAG_WRITE) == 0);
    }
    size_t offset = reinterpret_cast<jsuword>(tmp) & (alignment - 1);
    if (!offset)
        return tmp;

    



    size_t filler = size + alignment - offset;
    unsigned long cb = filler;
    unsigned long flags = 0;
    unsigned long rc = DosQueryMem(&(static_cast<char*>(tmp))[size],
                                   &cb, &flags);
    if (!rc && (flags & PAG_FREE) && cb >= filler) {
        UnmapPages(tmp, 0);
        if (DosAllocMem(&tmp, filler,
                        OBJ_ANY | PAG_COMMIT | PAG_READ | PAG_WRITE)) {
            JS_ALWAYS_TRUE(DosAllocMem(&tmp, filler,
                                       PAG_COMMIT | PAG_READ | PAG_WRITE) == 0);
        }
    }

    void *p = MapAlignedPagesRecursively(size, alignment, recursions);
    UnmapPages(tmp, 0);

    return p;
}

static void *
MapAlignedPages(size_t size, size_t alignment)
{
    int recursions = -1;

    



    void *p = MapAlignedPagesRecursively(size, alignment, recursions);
    if (p)
        return p;

    



    if (DosAllocMem(&p, 2 * size,
                    OBJ_ANY | PAG_COMMIT | PAG_READ | PAG_WRITE)) {
        JS_ALWAYS_TRUE(DosAllocMem(&p, 2 * size,
                                   PAG_COMMIT | PAG_READ | PAG_WRITE) == 0);
    }

    jsuword addr = reinterpret_cast<jsuword>(p);
    addr = (addr + (alignment - 1)) & ~(alignment - 1);

    return reinterpret_cast<void *>(addr);
}

#elif defined(XP_MACOSX) || defined(DARWIN)

static void *
MapPages(void *addr, size_t size)
{
    vm_address_t p;
    int flags;
    if (addr) {
        p = (vm_address_t) addr;
        flags = 0;
    } else {
        flags = VM_FLAGS_ANYWHERE;
    }

    kern_return_t err = vm_allocate((vm_map_t) mach_task_self(),
                                    &p, (vm_size_t) size, flags);
    if (err != KERN_SUCCESS)
        return NULL;

    JS_ASSERT(p);
    JS_ASSERT_IF(addr, p == (vm_address_t) addr);
    return (void *) p;
}

static void
UnmapPages(void *addr, size_t size)
{
    JS_ALWAYS_TRUE(vm_deallocate((vm_map_t) mach_task_self(),
                                 (vm_address_t) addr,
                                 (vm_size_t) size)
                   == KERN_SUCCESS);
}

#elif defined(XP_UNIX)


# if defined(SOLARIS) && defined(MAP_ALIGN)
#  define JS_GC_HAS_MAP_ALIGN

static void *
MapAlignedPages(size_t size, size_t alignment)
{
    



#ifdef SOLARIS
    void *p = mmap((caddr_t) alignment, size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_NOSYNC | MAP_ALIGN | MAP_ANON, -1, 0);
#else
    void *p = mmap((void *) alignment, size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_NOSYNC | MAP_ALIGN | MAP_ANON, -1, 0);
#endif
    if (p == MAP_FAILED)
        return NULL;
    return p;
}

# else 

static void *
MapPages(void *addr, size_t size)
{
    



    void *p = mmap(addr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON,
                   -1, 0);
    if (p == MAP_FAILED)
        return NULL;
    if (addr && p != addr) {
        
        JS_ALWAYS_TRUE(munmap(p, size) == 0);
        return NULL;
    }
    return p;
}

# endif 

static void
UnmapPages(void *addr, size_t size)
{
#ifdef SOLARIS
    JS_ALWAYS_TRUE(munmap((caddr_t) addr, size) == 0);
#else
    JS_ALWAYS_TRUE(munmap(addr, size) == 0);
#endif
}

#endif

namespace js {

GCChunkAllocator defaultGCChunkAllocator;

inline void *
FindChunkStart(void *p)
{
    jsuword addr = reinterpret_cast<jsuword>(p);
    addr = (addr + GC_CHUNK_MASK) & ~GC_CHUNK_MASK;
    return reinterpret_cast<void *>(addr);
}

JS_FRIEND_API(void *)
AllocGCChunk()
{
    void *p;

#ifdef JS_GC_HAS_MAP_ALIGN
    p = MapAlignedPages(GC_CHUNK_SIZE, GC_CHUNK_SIZE);
    if (!p)
        return NULL;
#else
    





    p = MapPages(NULL, GC_CHUNK_SIZE);
    if (!p)
        return NULL;

    if (reinterpret_cast<jsuword>(p) & GC_CHUNK_MASK) {
        UnmapPages(p, GC_CHUNK_SIZE);
        p = MapPages(FindChunkStart(p), GC_CHUNK_SIZE);
        while (!p) {
            




            p = MapPages(NULL, GC_CHUNK_SIZE * 2);
            if (!p)
                return 0;
            UnmapPages(p, GC_CHUNK_SIZE * 2);
            p = MapPages(FindChunkStart(p), GC_CHUNK_SIZE);

            



        }
    }
#endif 

    JS_ASSERT(!(reinterpret_cast<jsuword>(p) & GC_CHUNK_MASK));
    return p;
}

JS_FRIEND_API(void)
FreeGCChunk(void *p)
{
    JS_ASSERT(p);
    JS_ASSERT(!(reinterpret_cast<jsuword>(p) & GC_CHUNK_MASK));
    UnmapPages(p, GC_CHUNK_SIZE);
}

} 

