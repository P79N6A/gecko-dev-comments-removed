





#ifndef LoadContext_h
#define LoadContext_h

#include "SerializedLoadContext.h"
#include "mozilla/Attributes.h"
#include "nsWeakReference.h"
#include "nsIDOMElement.h"

class mozIApplication;

namespace mozilla {











class LoadContext MOZ_FINAL : public nsILoadContext
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSILOADCONTEXT

  
  
  LoadContext(const IPC::SerializedLoadContext& aToCopy,
              nsIDOMElement* aTopFrameElement,
              uint32_t aAppId, bool aInBrowser)
    : mIsNotNull(aToCopy.mIsNotNull)
    , mIsContent(aToCopy.mIsContent)
    , mUsePrivateBrowsing(aToCopy.mUsePrivateBrowsing)
    , mIsInBrowserElement(aInBrowser)
    , mAppId(aAppId)
    , mTopFrameElement(do_GetWeakReference(aTopFrameElement))
  {}

private:
  bool          mIsNotNull;
  bool          mIsContent;
  bool          mUsePrivateBrowsing;
  bool          mIsInBrowserElement;
  uint32_t      mAppId;
  nsWeakPtr     mTopFrameElement;
};

} 

#endif 

