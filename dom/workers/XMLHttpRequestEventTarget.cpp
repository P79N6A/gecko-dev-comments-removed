




#include "XMLHttpRequestEventTarget.h"

USING_WORKERS_NAMESPACE

void
XMLHttpRequestEventTarget::_trace(JSTracer* aTrc)
{
  EventTarget::_trace(aTrc);
}

void
XMLHttpRequestEventTarget::_finalize(JSFreeOp* aFop)
{
  EventTarget::_finalize(aFop);
}
