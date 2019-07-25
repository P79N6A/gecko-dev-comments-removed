





































#include "jsalloc.h"
#include "jscntxt.h"

namespace js {

void *
TempAllocPolicy::onOutOfMemory(void *p, size_t nbytes)
{
    return cx->runtime->onOutOfMemory(p, nbytes, cx);
}

void
TempAllocPolicy::reportAllocOverflow() const
{
    js_ReportAllocationOverflow(cx);
}

} 
