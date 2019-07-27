





#ifndef LoadContext_h
#define LoadContext_h

#include "SerializedLoadContext.h"
#include "mozilla/Attributes.h"
#include "nsIWeakReferenceUtils.h"
#include "mozilla/dom/Element.h"
#include "nsIInterfaceRequestor.h"
#include "nsILoadContext.h"

namespace mozilla {














class LoadContext final
  : public nsILoadContext
  , public nsIInterfaceRequestor
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSILOADCONTEXT
  NS_DECL_NSIINTERFACEREQUESTOR

  
  
  LoadContext(const IPC::SerializedLoadContext& aToCopy,
              dom::Element* aTopFrameElement,
              uint32_t aAppId, bool aInBrowser)
    : mTopFrameElement(do_GetWeakReference(aTopFrameElement))
    , mNestedFrameId(0)
    , mAppId(aAppId)
    , mIsContent(aToCopy.mIsContent)
    , mUsePrivateBrowsing(aToCopy.mUsePrivateBrowsing)
    , mUseRemoteTabs(aToCopy.mUseRemoteTabs)
    , mIsInBrowserElement(aInBrowser)
#ifdef DEBUG
    , mIsNotNull(aToCopy.mIsNotNull)
#endif
  {}

  
  
  LoadContext(const IPC::SerializedLoadContext& aToCopy,
              uint64_t aNestedFrameId,
              uint32_t aAppId, bool aInBrowser)
    : mTopFrameElement(nullptr)
    , mNestedFrameId(aNestedFrameId)
    , mAppId(aAppId)
    , mIsContent(aToCopy.mIsContent)
    , mUsePrivateBrowsing(aToCopy.mUsePrivateBrowsing)
    , mUseRemoteTabs(aToCopy.mUseRemoteTabs)
    , mIsInBrowserElement(aInBrowser)
#ifdef DEBUG
    , mIsNotNull(aToCopy.mIsNotNull)
#endif
  {}

  LoadContext(dom::Element* aTopFrameElement,
              uint32_t aAppId,
              bool aIsContent,
              bool aUsePrivateBrowsing,
              bool aUseRemoteTabs,
              bool aIsInBrowserElement)
    : mTopFrameElement(do_GetWeakReference(aTopFrameElement))
    , mNestedFrameId(0)
    , mAppId(aAppId)
    , mIsContent(aIsContent)
    , mUsePrivateBrowsing(aUsePrivateBrowsing)
    , mUseRemoteTabs(aUseRemoteTabs)
    , mIsInBrowserElement(aIsInBrowserElement)
#ifdef DEBUG
    , mIsNotNull(true)
#endif
  {}

  
  explicit LoadContext(uint32_t aAppId)
    : mTopFrameElement(nullptr)
    , mNestedFrameId(0)
    , mAppId(aAppId)
    , mIsContent(false)
    , mUsePrivateBrowsing(false)
    , mUseRemoteTabs(false)
    , mIsInBrowserElement(false)
#ifdef DEBUG
    , mIsNotNull(true)
#endif
  {}

  
  
  explicit LoadContext(nsIPrincipal* aPrincipal,
                       nsILoadContext* aOptionalBase = nullptr);

private:
  ~LoadContext() {}

  nsWeakPtr mTopFrameElement;
  uint64_t mNestedFrameId;
  uint32_t mAppId;
  bool mIsContent;
  bool mUsePrivateBrowsing;
  bool mUseRemoteTabs;
  bool mIsInBrowserElement;
#ifdef DEBUG
  bool mIsNotNull;
#endif
};

} 

#endif 

