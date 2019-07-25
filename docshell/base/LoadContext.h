





#ifndef LoadContext_h
#define LoadContext_h

#include "SerializedLoadContext.h"
#include "mozilla/Attributes.h"

namespace mozilla {











class LoadContext MOZ_FINAL : public nsILoadContext
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSILOADCONTEXT

  LoadContext(const IPC::SerializedLoadContext& toCopy)
    : mIsNotNull(toCopy.mIsNotNull)
    , mIsContent(toCopy.mIsContent)
    , mUsePrivateBrowsing(toCopy.mUsePrivateBrowsing)
    , mIsInBrowserElement(toCopy.mIsInBrowserElement)
    , mAppId(toCopy.mAppId)
  {}

private:
  bool          mIsNotNull;
  bool          mIsContent;
  bool          mUsePrivateBrowsing;
  bool          mIsInBrowserElement;
  uint32_t      mAppId;
};

} 

#endif 

