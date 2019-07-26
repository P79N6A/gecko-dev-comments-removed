






#include "jsalloc.h"
#include "jscntxt.h"

using namespace js;

void *
TempAllocPolicy::onOutOfMemory(void *p, size_t nbytes)
{
    return cx_->runtime->onOutOfMemory(p, nbytes, cx_);
}

void
TempAllocPolicy::reportAllocOverflow() const
{
    js_ReportAllocationOverflow(cx_);
}
