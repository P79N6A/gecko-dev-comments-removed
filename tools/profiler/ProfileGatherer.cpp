



#include "mozilla/ProfileGatherer.h"
#include "mozilla/Services.h"
#include "nsIObserverService.h"
#include "TableTicker.h"

using mozilla::dom::AutoJSAPI;
using mozilla::dom::Promise;

namespace mozilla {

NS_IMPL_ISUPPORTS0(ProfileGatherer)

ProfileGatherer::ProfileGatherer(TableTicker* aTicker,
                                 double aSinceTime,
                                 Promise* aPromise)
  : mPromise(aPromise)
  , mTicker(aTicker)
  , mSinceTime(aSinceTime)
  , mPendingProfiles(0)
{
}

void
ProfileGatherer::GatheredOOPProfile()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (NS_WARN_IF(!mPromise)) {
    
    
    return;
  }

  mPendingProfiles--;

  if (mPendingProfiles == 0) {
    
    
    Finish();
  }
}

void
ProfileGatherer::WillGatherOOPProfile()
{
  mPendingProfiles++;
}

void
ProfileGatherer::Start()
{
  MOZ_ASSERT(NS_IsMainThread());

  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  if (os) {
    nsresult rv = os->NotifyObservers(this, "profiler-subprocess-gather", nullptr);
    NS_WARN_IF(NS_FAILED(rv));
  }

  if (!mPendingProfiles) {
    Finish();
  }
}

void
ProfileGatherer::Finish()
{
  MOZ_ASSERT(NS_IsMainThread());
  UniquePtr<char[]> buf = mTicker->ToJSON(mSinceTime);

  AutoJSAPI jsapi;
  if (NS_WARN_IF(!jsapi.Init(mPromise->GlobalJSObject()))) {
    
    
    
    
    mTicker->ProfileGathered();
    return;
  }

  JSContext* cx = jsapi.cx();

  
  JS::RootedValue val(cx);
  {
    NS_ConvertUTF8toUTF16 js_string(nsDependentCString(buf.get()));
    if (!JS_ParseJSON(cx, static_cast<const char16_t*>(js_string.get()),
                      js_string.Length(), &val)) {
      if (!jsapi.HasException()) {
        mPromise->MaybeReject(NS_ERROR_DOM_UNKNOWN_ERR);
      } else {
        JS::RootedValue exn(cx);
        DebugOnly<bool> gotException = jsapi.StealException(&exn);
        MOZ_ASSERT(gotException);

        jsapi.ClearException();
        mPromise->MaybeReject(cx, exn);
      }
    } else {
      mPromise->MaybeResolve(val);
    }
  }

  mTicker->ProfileGathered();
}

} 
