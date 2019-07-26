





























#include "irregexp/RegExpStack.h"

#include "vm/Runtime.h"

using namespace js;
using namespace js::irregexp;

RegExpStackScope::RegExpStackScope(JSRuntime *rt)
  : regexp_stack(&rt->mainThread.regexpStack)
{}

RegExpStackScope::~RegExpStackScope()
{
    regexp_stack->reset();
}

int
irregexp::GrowBacktrackStack(JSRuntime *rt)
{
    return rt->mainThread.regexpStack.grow();
}

RegExpStack::RegExpStack()
  : base_(nullptr), size(0), limit_(nullptr)
{}

RegExpStack::~RegExpStack()
{
    js_free(base_);
}

bool
RegExpStack::init()
{
    base_ = js_malloc(kMinimumStackSize);
    if (!base_)
        return false;

    size = kMinimumStackSize;
    updateLimit();
    return true;
}

void
RegExpStack::reset()
{
    JS_ASSERT(size >= kMinimumStackSize);

    if (size != kMinimumStackSize) {
        base_ = js_realloc(base_, kMinimumStackSize);
        size = kMinimumStackSize;
        updateLimit();
    }
}

bool
RegExpStack::grow()
{
    size_t newSize = size * 2;
    if (newSize > kMaximumStackSize)
        return false;

    void *newBase = js_realloc(base_, newSize);
    if (!newBase)
        return false;

    base_ = newBase;
    size = newSize;
    updateLimit();

    return true;
}
