
































#include <stdlib.h>
#include "jstypes.h"
#include "jsstdint.h"
#include "jsgcchunk.h"
#ifdef JS_64BIT
# include "jsstr.h"
#endif


#ifdef XP_WIN
# include "jswin.h"

# ifdef _MSC_VER
#  pragma warning( disable: 4267 4996 4146 )
# endif

#elif defined(XP_MACOSX) || defined(DARWIN)

# include <libkern/OSAtomic.h>
# include <mach/mach_error.h>
# include <mach/mach_init.h>
# include <mach/vm_map.h>
# include <malloc/malloc.h>

#elif defined(XP_UNIX) || defined(XP_BEOS)

# include <unistd.h>
# include <sys/mman.h>

# ifndef MAP_NOSYNC
#  define MAP_NOSYNC    0
# endif

#endif

#ifdef XP_WIN






# if defined(WINCE) && !defined(MOZ_MEMORY_WINCE6)

#  define JS_GC_HAS_MAP_ALIGN

static void
UnmapPagesAtBase(void *p)
{
    JS_ALWAYS_TRUE(VirtualFree(p, 0, MEM_RELEASE));
}

static void *
MapAlignedPages(size_t size, size_t alignment)
{
    JS_ASSERT(size % alignment == 0);
    JS_ASSERT(size >= alignment);

    void *reserve = VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_NOACCESS);
    if (!reserve)
        return NULL;

    void *p = VirtualAlloc(reserve, size, MEM_COMMIT, PAGE_READWRITE);
    JS_ASSERT(p == reserve);

    size_t mask = alignment - 1;
    size_t offset = (uintptr_t) p & mask;
    if (!offset)
        return p;

    
    UnmapPagesAtBase(reserve);
    reserve = VirtualAlloc(NULL, size + alignment - offset, MEM_RESERVE,
                           PAGE_NOACCESS);
    if (!reserve)
        return NULL;
    if (offset == ((uintptr_t) reserve & mask)) {
        void *aligned = (void *) ((uintptr_t) reserve + alignment - offset);
        p = VirtualAlloc(aligned, size, MEM_COMMIT, PAGE_READWRITE);
        JS_ASSERT(p == aligned);
        return p;
    }

    
    UnmapPagesAtBase(reserve);
    reserve = VirtualAlloc(NULL, size + alignment, MEM_RESERVE, PAGE_NOACCESS);
    if (!reserve)
        return NULL;

    offset = (uintptr_t) reserve & mask;
    void *aligned = (void *) ((uintptr_t) reserve + alignment - offset);
    p = VirtualAlloc(aligned, size, MEM_COMMIT, PAGE_READWRITE);
    JS_ASSERT(p == aligned);

    return p;
}

static void
UnmapPages(void *p, size_t size)
{
    if (VirtualFree(p, 0, MEM_RELEASE))
        return;

    
    JS_ASSERT(GetLastError() == ERROR_INVALID_PARAMETER);
    MEMORY_BASIC_INFORMATION info;
    VirtualQuery(p, &info, sizeof(info));

    UnmapPagesAtBase(info.AllocationBase);
}

# else 

#  ifdef _M_X64

typedef long (*ntavm_fun)(HANDLE handle, void **addr, ULONG zbits,
                          size_t *size, ULONG alloctype, ULONG prot);
typedef long (*ntfvm_fun)(HANDLE handle, void **addr, size_t *size, 
                          ULONG freetype);

static ntavm_fun NtAllocateVirtualMemory;
static ntfvm_fun NtFreeVirtualMemory;

bool 
js::InitNtAllocAPIs()
{
    HMODULE h = GetModuleHandle("ntdll.dll");
    if (!h)
        return false;
    NtAllocateVirtualMemory = ntavm_fun(GetProcAddress(h, "NtAllocateVirtualMemory"));
    if (!NtAllocateVirtualMemory)
        return false;
    NtFreeVirtualMemory = ntfvm_fun(GetProcAddress(h, "NtFreeVirtualMemory"));
    if (!NtFreeVirtualMemory)
        return false;
}


static void *
MapPages(void *addr, size_t size)
{
    long rc = NtAllocateVirtualMemory(INVALID_HANDLE_VALUE, &addr, 1, &size,
                                      MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
    return rc ? NULL : addr;
}

static void
UnmapPages(void *addr, size_t size)
{
    NtFreeVirtualMemory(INVALID_HANDLE_VALUE, &addr, &size, MEM_RELEASE);
}

#  else 

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

#  endif 

# endif 

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

#elif defined(XP_UNIX) || defined(XP_BEOS)


# if defined(SOLARIS) && defined(MAP_ALIGN)
#  define JS_GC_HAS_MAP_ALIGN

static void *
MapAlignedPages(size_t size, size_t alignment)
{
    



	
    void *p = mmap((caddr_t) alignment, size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_NOSYNC | MAP_ALIGN | MAP_ANON, -1, 0);
    if (p == MAP_FAILED)
        return NULL;
    return p;
}

# else 

# if defined(__MACH__) && defined(__APPLE__) && defined(__x86_64__)


static void *
MapPages(void *addr, size_t size)
{
    void * const start = (void *) 0x10000;
    void * const end = (void *) 0x100000000;

    
    if (addr) {
        JS_ASSERT(addr < end);
        void *p = mmap(addr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
        if (p == MAP_FAILED)
            return NULL;
        if (p != addr) {
            JS_ALWAYS_TRUE(munmap(p, size) == 0);
            return NULL;
        }
        return p;
    }

    
    
    
    
    static void *base = start;
    while (true) {
        void *p = mmap(base, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
        if (p == MAP_FAILED)
            return NULL;
        
        if (start <= p && p < end) {
            base = (void *) (uintptr_t(p) + size);
            return p;
        }
        
        
        munmap(p, size);
        if (base != start)
            return NULL;
        base = start;
    }
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

# endif 

static void
UnmapPages(void *addr, size_t size)
{
    JS_ALWAYS_TRUE(munmap((caddr_t) addr, size) == 0);
}

#endif

#ifdef JS_64BIT
bool
JSString::initStringTables()
{
    char *p = (char *) MapPages(NULL, unitStringTableSize + intStringTableSize);
    if (!p)
        return false;
    unitStringTable = (JSString*) memcpy(p, staticUnitStringTable, unitStringTableSize);
    intStringTable = (JSString*) memcpy(p + unitStringTableSize, 
                                        staticIntStringTable, intStringTableSize);

    return true;
}

void
JSString::freeStringTables()
{
    UnmapPages(unitStringTable, unitStringTableSize + intStringTableSize);
    unitStringTable = NULL;
    intStringTable = NULL;
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

