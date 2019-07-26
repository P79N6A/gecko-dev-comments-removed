





#ifndef LoadContext_h
#define LoadContext_h

#include "SerializedLoadContext.h"
#include "mozilla/Attributes.h"
#include "nsIWeakReferenceUtils.h"
#include "mozilla/dom/Element.h"
#include "nsIInterfaceRequestor.h"

class mozIApplication;

namespace mozilla {














class LoadContext MOZ_FINAL : public nsILoadContext,
                              public nsIInterfaceRequestor
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSILOADCONTEXT
  NS_DECL_NSIINTERFACEREQUESTOR

  
  
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

  
  LoadContext(uint32_t aAppId)
    : mTopFrameElement(nullptr)
    , mAppId(aAppId)
    , mIsContent(false)
    , mUsePrivateBrowsing(false)
    , mIsInBrowserElement(false)
#ifdef DEBUG
    , mIsNotNull(true)
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

