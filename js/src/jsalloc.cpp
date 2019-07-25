





































#include "jsalloc.h"
#include "jscntxt.h"

namespace js {

void *
ContextAllocPolicy::onOutOfMemory(void *p, size_t nbytes)
{
    return cx->runtime->onOutOfMemory(p, nbytes, cx);
}

void
ContextAllocPolicy::reportAllocOverflow() const
{
    js_ReportAllocationOverflow(cx);
}

} 
