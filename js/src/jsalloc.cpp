





#include "jsalloc.h"

#include "jscntxt.h"

using namespace js;

void*
TempAllocPolicy::onOutOfMemory(AllocFunction allocFunc, size_t nbytes, void* reallocPtr)
{
    return static_cast<ExclusiveContext*>(cx_)->onOutOfMemory(allocFunc, nbytes, reallocPtr);
}

void
TempAllocPolicy::reportAllocOverflow() const
{
    ReportAllocationOverflow(static_cast<ExclusiveContext*>(cx_));
}
