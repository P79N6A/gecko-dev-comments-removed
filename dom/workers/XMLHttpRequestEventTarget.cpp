




#include "XMLHttpRequestEventTarget.h"

USING_WORKERS_NAMESPACE

void
XMLHttpRequestEventTarget::_Trace(JSTracer* aTrc)
{
  EventTarget::_Trace(aTrc);
}

void
XMLHttpRequestEventTarget::_Finalize(JSContext* aCx)
{
  EventTarget::_Finalize(aCx);
}
