






#ifndef TimelineMarker_h__
#define TimelineMarker_h__

#include "nsString.h"
#include "GeckoProfiler.h"
#include "mozilla/dom/ProfileTimelineMarkerBinding.h"
#include "nsContentUtils.h"
#include "jsapi.h"

class nsDocShell;




class TimelineMarker
{
public:
  TimelineMarker(nsDocShell* aDocShell, const char* aName,
                 TracingMetadata aMetaData);

  TimelineMarker(nsDocShell* aDocShell, const char* aName,
                 TracingMetadata aMetaData,
                 const nsAString& aCause);

  virtual ~TimelineMarker();

  
  
  
  virtual bool Equals(const TimelineMarker* aOther)
  {
    return strcmp(mName, aOther->mName) == 0;
  }

  
  
  
  
  
  virtual void AddDetails(mozilla::dom::ProfileTimelineMarker& aMarker) {}

  virtual void AddLayerRectangles(
      mozilla::dom::Sequence<mozilla::dom::ProfileTimelineLayerRect>&)
  {
    MOZ_ASSERT_UNREACHABLE("can only be called on layer markers");
  }

  const char* GetName() const { return mName; }
  TracingMetadata GetMetaData() const { return mMetaData; }
  DOMHighResTimeStamp GetTime() const { return mTime; }
  const nsString& GetCause() const { return mCause; }

  JSObject* GetStack()
  {
    if (mStackTrace.initialized()) {
      return mStackTrace;
    }
    return nullptr;
  }

protected:
  void CaptureStack()
  {
    JSContext* ctx = nsContentUtils::GetCurrentJSContext();
    if (ctx) {
      JS::RootedObject stack(ctx);
      if (JS::CaptureCurrentStack(ctx, &stack)) {
        mStackTrace.init(ctx, stack.get());
      } else {
        JS_ClearPendingException(ctx);
      }
    }
  }

private:
  const char* mName;
  TracingMetadata mMetaData;
  DOMHighResTimeStamp mTime;
  nsString mCause;

  
  
  
  
  JS::PersistentRooted<JSObject*> mStackTrace;
};

#endif 
