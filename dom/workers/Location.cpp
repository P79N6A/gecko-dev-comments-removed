




#include "Location.h"

#include "DOMBindingInlines.h"

#include "nsTraceRefcnt.h"

BEGIN_WORKERS_NAMESPACE

 already_AddRefed<WorkerLocation>
WorkerLocation::Create(JSContext* aCx, JS::Handle<JSObject*> aGlobal,
                       WorkerPrivate::LocationInfo& aInfo)
{
  nsRefPtr<WorkerLocation> location =
    new WorkerLocation(aCx,
                       NS_ConvertUTF8toUTF16(aInfo.mHref),
                       NS_ConvertUTF8toUTF16(aInfo.mProtocol),
                       NS_ConvertUTF8toUTF16(aInfo.mHost),
                       NS_ConvertUTF8toUTF16(aInfo.mHostname),
                       NS_ConvertUTF8toUTF16(aInfo.mPort),
                       NS_ConvertUTF8toUTF16(aInfo.mPathname),
                       NS_ConvertUTF8toUTF16(aInfo.mSearch),
                       NS_ConvertUTF8toUTF16(aInfo.mHash));

  if (!Wrap(aCx, aGlobal, location)) {
    return nullptr;
  }

  return location.forget();
}

void
WorkerLocation::_trace(JSTracer* aTrc)
{
  DOMBindingBase::_trace(aTrc);
}

void
WorkerLocation::_finalize(JSFreeOp* aFop)
{
  DOMBindingBase::_finalize(aFop);
}

END_WORKERS_NAMESPACE
