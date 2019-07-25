




#include "XMLHttpRequestEventTarget.h"

USING_WORKERS_NAMESPACE

void
XMLHttpRequestEventTarget::_Trace(JSTracer* aTrc)
{
  EventTarget::_Trace(aTrc);
}

void
XMLHttpRequestEventTarget::_Finalize(JSFreeOp* aFop)
{
  EventTarget::_Finalize(aFop);
}
