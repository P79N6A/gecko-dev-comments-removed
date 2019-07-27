





#include "mozilla/dom/Headers.h"

#include "mozilla/ErrorResult.h"
#include "mozilla/dom/WorkerPrivate.h"
#include "mozilla/Preferences.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTING_ADDREF(Headers)
NS_IMPL_CYCLE_COLLECTING_RELEASE(Headers)
NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(Headers, mOwner)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(Headers)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END


bool
Headers::PrefEnabled(JSContext* aCx, JSObject* aObj)
{
  using mozilla::dom::workers::WorkerPrivate;
  using mozilla::dom::workers::GetWorkerPrivateFromContext;

  if (NS_IsMainThread()) {
    static bool sPrefCacheInit = false;
    static bool sPrefEnabled = false;
    if (sPrefCacheInit) {
      return sPrefEnabled;
    }
    Preferences::AddBoolVarCache(&sPrefEnabled, "dom.fetch.enabled");
    sPrefCacheInit = true;
    return sPrefEnabled;
  }

  WorkerPrivate* workerPrivate = GetWorkerPrivateFromContext(aCx);
  if (!workerPrivate) {
    return false;
  }

  return workerPrivate->DOMFetchEnabled();
}


already_AddRefed<Headers>
Headers::Constructor(const GlobalObject& aGlobal,
                     const Optional<HeadersOrByteStringSequenceSequenceOrByteStringMozMap>& aInit,
                     ErrorResult& aRv)
{
  nsRefPtr<InternalHeaders> ih = new InternalHeaders();
  nsRefPtr<Headers> headers = new Headers(aGlobal.GetAsSupports(), ih);

  if (!aInit.WasPassed()) {
    return headers.forget();
  }

  if (aInit.Value().IsHeaders()) {
    ih->Fill(*aInit.Value().GetAsHeaders().mInternalHeaders, aRv);
  } else if (aInit.Value().IsByteStringSequenceSequence()) {
    ih->Fill(aInit.Value().GetAsByteStringSequenceSequence(), aRv);
  } else if (aInit.Value().IsByteStringMozMap()) {
    ih->Fill(aInit.Value().GetAsByteStringMozMap(), aRv);
  }

  if (aRv.Failed()) {
    return nullptr;
  }

  return headers.forget();
}


already_AddRefed<Headers>
Headers::Constructor(const GlobalObject& aGlobal,
                     const OwningHeadersOrByteStringSequenceSequenceOrByteStringMozMap& aInit,
                     ErrorResult& aRv)
{
  nsRefPtr<InternalHeaders> ih = new InternalHeaders();
  nsRefPtr<Headers> headers = new Headers(aGlobal.GetAsSupports(), ih);

  if (aInit.IsHeaders()) {
    ih->Fill(*(aInit.GetAsHeaders().get()->mInternalHeaders), aRv);
  } else if (aInit.IsByteStringSequenceSequence()) {
    ih->Fill(aInit.GetAsByteStringSequenceSequence(), aRv);
  } else if (aInit.IsByteStringMozMap()) {
    ih->Fill(aInit.GetAsByteStringMozMap(), aRv);
  }

  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  return headers.forget();
}

JSObject*
Headers::WrapObject(JSContext* aCx)
{
  return mozilla::dom::HeadersBinding::Wrap(aCx, this);
}

Headers::~Headers()
{
}
} 
} 
