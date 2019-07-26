



#include "base/basictypes.h"
#include "GetUserMediaRequest.h"
#include "mozilla/dom/MediaStreamBinding.h"
#include "mozilla/dom/GetUserMediaRequestBinding.h"
#include "nsIScriptGlobalObject.h"
#include "nsPIDOMWindow.h"
#include "nsCxPusher.h"

namespace mozilla {
namespace dom {

GetUserMediaRequest::GetUserMediaRequest(
    nsPIDOMWindow* aInnerWindow,
    const nsAString& aCallID,
    const MediaStreamConstraintsInternal& aConstraints,
    bool aIsSecure)
  : mInnerWindowID(aInnerWindow->WindowID())
  , mOuterWindowID(aInnerWindow->GetOuterWindow()->WindowID())
  , mCallID(aCallID)
  , mConstraints(new MediaStreamConstraintsInternal(aConstraints))
  , mIsSecure(aIsSecure)
{
  SetIsDOMBinding();
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(GetUserMediaRequest)
NS_IMPL_CYCLE_COLLECTING_ADDREF(GetUserMediaRequest)
NS_IMPL_CYCLE_COLLECTING_RELEASE(GetUserMediaRequest)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(GetUserMediaRequest)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

JSObject*
GetUserMediaRequest::WrapObject(JSContext* aCx)
{
  return GetUserMediaRequestBinding::Wrap(aCx, this);
}

nsISupports* GetUserMediaRequest::GetParentObject()
{
  return nullptr;
}

void GetUserMediaRequest::GetCallID(nsString& retval)
{
  retval = mCallID;
}

uint64_t GetUserMediaRequest::WindowID()
{
  return mOuterWindowID;
}

uint64_t GetUserMediaRequest::InnerWindowID()
{
  return mInnerWindowID;
}

bool GetUserMediaRequest::IsSecure()
{
  return mIsSecure;
}

void
GetUserMediaRequest::GetConstraints(MediaStreamConstraintsInternal &result)
{
  result = *mConstraints;
}

} 
} 
