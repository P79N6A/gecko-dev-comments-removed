



#ifndef GetUserMediaRequest_h__
#define GetUserMediaRequest_h__

#include "mozilla/ErrorResult.h"
#include "nsISupportsImpl.h"
#include "nsAutoPtr.h"
#include "nsWrapperCache.h"
#include "mozilla/dom/BindingUtils.h"
#include "nsPIDOMWindow.h"

namespace mozilla {
namespace dom {

struct MediaStreamConstraints;

class GetUserMediaRequest : public nsISupports, public nsWrapperCache
{
public:
  GetUserMediaRequest(nsPIDOMWindow* aInnerWindow,
                      const nsAString& aCallID,
                      const MediaStreamConstraints& aConstraints,
                      bool aIsSecure);
  virtual ~GetUserMediaRequest() {};

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(GetUserMediaRequest)

  virtual JSObject* WrapObject(JSContext* cx)
    MOZ_OVERRIDE;
  nsISupports* GetParentObject();

  uint64_t WindowID();
  uint64_t InnerWindowID();
  bool IsSecure();
  void GetCallID(nsString& retval);
  void GetConstraints(MediaStreamConstraints &result);

private:
  uint64_t mInnerWindowID, mOuterWindowID;
  const nsString mCallID;
  nsAutoPtr<MediaStreamConstraints> mConstraints;
  bool mIsSecure;
};

} 
} 

#endif 
