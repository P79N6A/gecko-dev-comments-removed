



#ifndef GetUserMediaRequest_h__
#define GetUserMediaRequest_h__

#include "mozilla/ErrorResult.h"
#include "nsISupportsImpl.h"
#include "nsAutoPtr.h"
#include "nsWrapperCache.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/MediaStreamTrackBinding.h"
#include "nsPIDOMWindow.h"

namespace mozilla {
namespace dom {
class MediaStreamConstraintsInternal;

class GetUserMediaRequest : public nsISupports, public nsWrapperCache
{
public:
  GetUserMediaRequest(nsPIDOMWindow* aInnerWindow,
                      const nsAString& aCallID,
                      const MediaStreamConstraintsInternal& aConstraints);
  virtual ~GetUserMediaRequest() {};

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(GetUserMediaRequest)

  virtual JSObject* WrapObject(JSContext* cx, JS::Handle<JSObject*> scope)
    MOZ_OVERRIDE;
  nsISupports* GetParentObject();

  uint64_t WindowID();
  uint64_t InnerWindowID();
  void GetCallID(nsString& retval);
  void GetConstraints(MediaStreamConstraintsInternal &result);

private:
  uint64_t mInnerWindowID, mOuterWindowID;
  const nsString mCallID;
  MediaStreamConstraintsInternal mConstraints;
};

} 
} 

#endif 
