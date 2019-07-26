





#ifndef LoadContext_h
#define LoadContext_h

#include "SerializedLoadContext.h"
#include "mozilla/Attributes.h"
#include "nsIWeakReferenceUtils.h"
#include "mozilla/dom/Element.h"

class mozIApplication;

namespace mozilla {











class LoadContext MOZ_FINAL : public nsILoadContext
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSILOADCONTEXT

  
  
  LoadContext(const IPC::SerializedLoadContext& aToCopy,
              dom::Element* aTopFrameElement,
              uint32_t aAppId, bool aInBrowser)
    : mTopFrameElement(do_GetWeakReference(aTopFrameElement))
    , mAppId(aAppId)
    , mIsContent(aToCopy.mIsContent)
    , mUsePrivateBrowsing(aToCopy.mUsePrivateBrowsing)
    , mIsInBrowserElement(aInBrowser)
#ifdef DEBUG
    , mIsNotNull(aToCopy.mIsNotNull)
#endif
  {}

private:
  nsWeakPtr     mTopFrameElement;
  uint32_t      mAppId;
  bool          mIsContent;
  bool          mUsePrivateBrowsing;
  bool          mIsInBrowserElement;
#ifdef DEBUG
  bool          mIsNotNull;
#endif
};

} 

#endif 

