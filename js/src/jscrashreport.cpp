







































#include "jsapi.h"
#include "jscntxt.h"
#include "jscrashreport.h"
#include "jscrashformat.h"

#include <time.h>

namespace js {
namespace crash {

const static int stack_snapshot_max_size = 32768;

#if defined(XP_WIN)

#include <windows.h>

static bool
GetStack(uint64 *stack, uint64 *stack_len, CrashRegisters *regs, char *buffer, size_t size)
{
    
    char dummy;
    MEMORY_BASIC_INFORMATION info;
    if (VirtualQuery(reinterpret_cast<LPCVOID>(&dummy), &info, sizeof(info)) == 0)
        return false;
    if (info.State != MEM_COMMIT)
        return false;

    
    uint64 p = uint64(&dummy) - 256;
    uint64 len = stack_snapshot_max_size;

    if (p + len > uint64(info.BaseAddress) + info.RegionSize)
        len = uint64(info.BaseAddress) + info.RegionSize - p;

    if (len > size)
        len = size;

    *stack = p;
    *stack_len = len;

    
#if defined(_MSC_VER) && JS_BITS_PER_WORD == 32
    
    uint32 vip, vsp, vbp;
    __asm {
    Label:
        mov [vbp], ebp;
        mov [vsp], esp;
        mov eax, [Label];
        mov [vip], eax;
    }
    regs->ip = vip;
    regs->sp = vsp;
    regs->bp = vbp;
#else
    CONTEXT context;
    RtlCaptureContext(&context);
#if JS_BITS_PER_WORD == 32
    regs->ip = context.Eip;
    regs->sp = context.Esp;
    regs->bp = context.Ebp;
#else
    regs->ip = context.Rip;
    regs->sp = context.Rsp;
    regs->bp = context.Rbp;
#endif
#endif

    memcpy(buffer, (void *)p, len);

    return true;
}

#elif 0

#include <unistd.h>
#include <ucontext.h>
#include <sys/mman.h>

static bool
GetStack(uint64 *stack, uint64 *stack_len, CrashRegisters *regs, char *buffer, size_t size)
{
    
    char dummy;
    uint64 p = uint64(&dummy) - 256;
    uint64 pgsz = getpagesize();
    uint64 len = stack_snapshot_max_size;
    p &= ~(pgsz - 1);

    
    while (len > 0) {
	if (mlock((const void *)p, len) == 0) {
	    munlock((const void *)p, len);
	    break;
	}
	len -= pgsz;
    }

    if (len > size)
        len = size;

    *stack = p;
    *stack_len = len;

    
    ucontext_t context;
    if (getcontext(&context) != 0)
	return false;

#if JS_BITS_PER_WORD == 64
    regs->sp = (uint64)context.uc_mcontext.gregs[REG_RSP];
    regs->bp = (uint64)context.uc_mcontext.gregs[REG_RBP];
    regs->ip = (uint64)context.uc_mcontext.gregs[REG_RIP];
#elif JS_BITS_PER_WORD == 32
    regs->sp = (uint64)context.uc_mcontext.gregs[REG_ESP];
    regs->bp = (uint64)context.uc_mcontext.gregs[REG_EBP];
    regs->ip = (uint64)context.uc_mcontext.gregs[REG_EIP];
#endif

    memcpy(buffer, (void *)p, len);

    return true;
}

#else

static bool
GetStack(uint64 *stack, uint64 *stack_len, CrashRegisters *regs, char *buffer, size_t size)
{
    return false;
}

#endif

class Stack : private CrashStack
{
public:
    Stack(uint64 id);

    bool snapshot();
};

Stack::Stack(uint64 id)
  : CrashStack(id)
{
}

bool
Stack::snapshot()
{
    snaptime = time(NULL);
    return GetStack(&stack_base, &stack_len, &regs, stack, sizeof(stack));
}

class Ring : private CrashRing
{
public:
    Ring(uint64 id);

    void push(uint64 tag, void *data, size_t size);

private:
    size_t bufferSize() { return crash_buffer_size; }
    void copyBytes(void *data, size_t size);
};

Ring::Ring(uint64 id)
  : CrashRing(id)
{
}

void
Ring::push(uint64 tag, void *data, size_t size)
{
    uint64 t = time(NULL);

    copyBytes(&tag, sizeof(uint64));
    copyBytes(&t, sizeof(uint64));
    copyBytes(data, size);
    uint64 mysize = size;
    copyBytes(&mysize, sizeof(uint64));
}

void
Ring::copyBytes(void *data, size_t size)
{
    if (size >= bufferSize())
        size = bufferSize();

    if (offset + size > bufferSize()) {
        size_t first = bufferSize() - offset;
        size_t second = size - first;
        memcpy(&buffer[offset], data, first);
        memcpy(buffer, (char *)data + first, second);
        offset = second;
    } else {
        memcpy(&buffer[offset], data, size);
        offset += size;
    }
}

static bool gInitialized;

static Stack gGCStack(JS_CRASH_STACK_GC);
static Stack gErrorStack(JS_CRASH_STACK_ERROR);
static Ring gRingBuffer(JS_CRASH_RING);

} 
} 

using namespace js;
using namespace js::crash;

JS_FRIEND_API(void)
js_SnapshotGCStack()
{
    if (gInitialized)
        gGCStack.snapshot();
}

JS_FRIEND_API(void)
js_SnapshotErrorStack()
{
    if (gInitialized)
        gErrorStack.snapshot();
}

JS_FRIEND_API(void)
js_SaveCrashData(uint64 tag, void *ptr, size_t size)
{
    if (gInitialized)
        gRingBuffer.push(tag, ptr, size);
}

JS_PUBLIC_API(void)
JS_EnumerateDiagnosticMemoryRegions(JSEnumerateDiagnosticMemoryCallback callback)
{
#if 1
    if (!gInitialized) {
        gInitialized = true;
        (*callback)(&gGCStack, sizeof(gGCStack));
        (*callback)(&gErrorStack, sizeof(gErrorStack));
        (*callback)(&gRingBuffer, sizeof(gRingBuffer));
    }
#endif
}

