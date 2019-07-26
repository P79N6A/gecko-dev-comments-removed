





#include "jsalloc.h"

#include "jscntxt.h"

using namespace js;

void *
TempAllocPolicy::onOutOfMemory(void *p, size_t nbytes)
{
    return static_cast<ThreadSafeContext *>(cx_)->onOutOfMemory(p, nbytes);
}

void
TempAllocPolicy::reportAllocOverflow() const
{
    js_ReportAllocationOverflow(static_cast<ThreadSafeContext *>(cx_));
}
