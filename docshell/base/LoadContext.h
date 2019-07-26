





#ifndef LoadContext_h
#define LoadContext_h

#include "SerializedLoadContext.h"
#include "mozilla/Attributes.h"
#include "nsWeakReference.h"

namespace mozilla {











class LoadContext MOZ_FINAL : public nsILoadContext
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSILOADCONTEXT

  LoadContext(const IPC::SerializedLoadContext& aToCopy)
    : mIsNotNull(aToCopy.mIsNotNull)
    , mIsContent(aToCopy.mIsContent)
    , mUsePrivateBrowsing(aToCopy.mUsePrivateBrowsing)
    , mIsInBrowserElement(aToCopy.mIsInBrowserElement)
    , mAppId(aToCopy.mAppId)
  {}

  LoadContext(const IPC::SerializedLoadContext& aToCopy,
              nsIDOMElement* aTopFrameElemenet);

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

