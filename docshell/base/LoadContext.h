





#ifndef LoadContext_h
#define LoadContext_h

#include "SerializedLoadContext.h"

namespace mozilla {











class LoadContext : public nsILoadContext
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
  PRUint32      mAppId;
};

} 

#endif 

