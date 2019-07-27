
























#include "mozilla/WindowsVersion.h"

#include "jsfriendapi.h"
#include "jsmath.h"
#include "jswin.h"

#include "jit/ExecutableAllocator.h"

using namespace js::jit;

uint64_t ExecutableAllocator::rngSeed;

size_t ExecutableAllocator::determinePageSize()
{
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);
    return system_info.dwPageSize;
}

void *ExecutableAllocator::computeRandomAllocationAddress()
{
    










    static const unsigned chunkBits = 16;
#ifdef JS_CPU_X64
    static const uintptr_t base = 0x0000000080000000;
    static const uintptr_t mask = 0x000003ffffff0000;
#elif defined(JS_CPU_X86)
    static const uintptr_t base = 0x04000000;
    static const uintptr_t mask = 0x3fff0000;
#else
# error "Unsupported architecture"
#endif
    uint64_t rand = random_next(&rngSeed, 32) << chunkBits;
    return (void *) (base | (rand & mask));
}

static bool
RandomizeIsBrokenImpl()
{
    
    return !mozilla::IsVistaOrLater();
}

static bool
RandomizeIsBroken()
{
    
    
    static int result = RandomizeIsBrokenImpl();
    return !!result;
}

#ifdef JS_CPU_X64
static js::JitExceptionHandler sJitExceptionHandler;

JS_FRIEND_API(void)
js::SetJitExceptionHandler(JitExceptionHandler handler)
{
    MOZ_ASSERT(!sJitExceptionHandler);
    sJitExceptionHandler = handler;
}



struct UnwindInfo
{
    uint8_t version : 3;
    uint8_t flags : 5;
    uint8_t sizeOfPrologue;
    uint8_t countOfUnwindCodes;
    uint8_t frameRegister : 4;
    uint8_t frameOffset : 4;
    ULONG exceptionHandler;
};

static const unsigned ThunkLength = 12;

struct ExceptionHandlerRecord
{
    RUNTIME_FUNCTION runtimeFunction;
    UnwindInfo unwindInfo;
    uint8_t thunk[ThunkLength];
};






static DWORD
ExceptionHandler(PEXCEPTION_RECORD exceptionRecord, _EXCEPTION_REGISTRATION_RECORD *,
                 PCONTEXT context, _EXCEPTION_REGISTRATION_RECORD **)
{
    return sJitExceptionHandler(exceptionRecord, context);
}



static bool
RegisterExecutableMemory(void *p, size_t bytes, size_t pageSize)
{
    ExceptionHandlerRecord *r = reinterpret_cast<ExceptionHandlerRecord*>(p);

    
    
    
    
    
    
    

    r->runtimeFunction.BeginAddress = pageSize;
    r->runtimeFunction.EndAddress = (DWORD)bytes;
    r->runtimeFunction.UnwindData = offsetof(ExceptionHandlerRecord, unwindInfo);

    r->unwindInfo.version = 1;
    r->unwindInfo.flags = UNW_FLAG_EHANDLER;
    r->unwindInfo.sizeOfPrologue = 0;
    r->unwindInfo.countOfUnwindCodes = 0;
    r->unwindInfo.frameRegister = 0;
    r->unwindInfo.frameOffset = 0;
    r->unwindInfo.exceptionHandler = offsetof(ExceptionHandlerRecord, thunk);

    
    r->thunk[0]  = 0x48;
    r->thunk[1]  = 0xb8;
    void *handler = JS_FUNC_TO_DATA_PTR(void *, ExceptionHandler);
    memcpy(&r->thunk[2], &handler, 8);

    
    r->thunk[10] = 0xff;
    r->thunk[11] = 0xe0;

    DWORD oldProtect;
    if (!VirtualProtect(p, pageSize, PAGE_EXECUTE_READ, &oldProtect))
        return false;

    return RtlAddFunctionTable(&r->runtimeFunction, 1, reinterpret_cast<DWORD64>(p));
}

static void
UnregisterExecutableMemory(void *p, size_t bytes, size_t pageSize)
{
    ExceptionHandlerRecord *r = reinterpret_cast<ExceptionHandlerRecord*>(p);
    RtlDeleteFunctionTable(&r->runtimeFunction);
}
#endif

void *
js::jit::AllocateExecutableMemory(void *addr, size_t bytes, unsigned permissions, const char *tag,
                                  size_t pageSize)
{
    MOZ_ASSERT(bytes % pageSize == 0);
    MOZ_ASSERT(permissions == PAGE_EXECUTE_READWRITE);

#ifdef JS_CPU_X64
    if (sJitExceptionHandler)
        bytes += pageSize;
#endif

    void *p = VirtualAlloc(addr, bytes, MEM_COMMIT | MEM_RESERVE, permissions);
    if (!p)
        return nullptr;

#ifdef JS_CPU_X64
    if (sJitExceptionHandler) {
        if (!RegisterExecutableMemory(p, bytes, pageSize)) {
            VirtualFree(p, 0, MEM_RELEASE);
            return nullptr;
        }

        p = (uint8_t*)p + pageSize;
    }
#endif

    return p;
}

void
js::jit::DeallocateExecutableMemory(void *addr, size_t bytes, size_t pageSize)
{
    MOZ_ASSERT(bytes % pageSize == 0);

#ifdef JS_CPU_X64
    if (sJitExceptionHandler) {
        addr = (uint8_t*)addr - pageSize;
        UnregisterExecutableMemory(addr, bytes, pageSize);
    }
#endif

    VirtualFree(addr, 0, MEM_RELEASE);
}

ExecutablePool::Allocation ExecutableAllocator::systemAlloc(size_t n)
{
    void *allocation = nullptr;
    
    
#ifndef JS_CPU_X64
    if (!RandomizeIsBroken()) {
        void *randomAddress = computeRandomAllocationAddress();
        allocation = AllocateExecutableMemory(randomAddress, n, PAGE_EXECUTE_READWRITE,
                                              "js-jit-code", pageSize);
    }
#endif
    if (!allocation) {
        allocation = AllocateExecutableMemory(nullptr, n, PAGE_EXECUTE_READWRITE,
                                              "js-jit-code", pageSize);
    }
    ExecutablePool::Allocation alloc = { reinterpret_cast<char*>(allocation), n };
    return alloc;
}

void ExecutableAllocator::systemRelease(const ExecutablePool::Allocation& alloc)
{
    DeallocateExecutableMemory(alloc.pages, alloc.size, pageSize);
}

#if ENABLE_ASSEMBLER_WX_EXCLUSIVE
#error "ASSEMBLER_WX_EXCLUSIVE not yet suported on this platform."
#endif
