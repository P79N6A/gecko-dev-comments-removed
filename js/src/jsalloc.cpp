





#include "jsalloc.h"

#include "jscntxt.h"

using namespace js;

void *
TempAllocPolicy::onOutOfMemory(void *p, size_t nbytes)
{
    return static_cast<ExclusiveContext *>(cx_)->onOutOfMemory(p, nbytes);
}

void
TempAllocPolicy::reportAllocOverflow() const
{
    ReportAllocationOverflow(static_cast<ExclusiveContext *>(cx_));
}
