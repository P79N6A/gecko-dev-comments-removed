





































#ifndef mozilla_tabs_TabChild_h
#define mozilla_tabs_TabChild_h

#ifndef _IMPL_NS_LAYOUT
#include "mozilla/dom/PIFrameEmbeddingChild.h"
#endif
#include "nsIWebNavigation.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIWebBrowserChrome2.h"
#include "nsIEmbeddingSiteWindow2.h"
#include "nsIWebBrowserChromeFocus.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMEventTarget.h"
#include "nsIInterfaceRequestor.h"
#include "nsIWindowProvider.h"
#include "nsIXPCScriptable.h"
#include "nsIClassInfo.h"
#include "jsapi.h"
#include "nsIXPConnect.h"
#include "nsIDOMWindow.h"
#include "nsIDocShell.h"
#include "nsNetUtil.h"
#include "nsFrameMessageManager.h"
#include "nsIScriptContext.h"
#include "nsDOMEventTargetHelper.h"
#include "nsIPrincipal.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIScriptContext.h"

class gfxMatrix;

namespace mozilla {
namespace dom {

class TabChild;

class TabChildGlobal : public nsDOMEventTargetHelper,
                       public nsIContentFrameMessageManager,
                       public nsIScriptObjectPrincipal,
                       public nsIScriptContextPrincipal
{
public:
  TabChildGlobal(TabChild* aTabChild);
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(TabChildGlobal, nsDOMEventTargetHelper)
  NS_FORWARD_SAFE_NSIFRAMEMESSAGEMANAGER(mMessageManager)
  NS_IMETHOD SendSyncMessage()
  {
    return mMessageManager ? mMessageManager->SendSyncMessage()
                           : NS_ERROR_NULL_POINTER;
  }
  NS_IMETHOD GetContent(nsIDOMWindow** aContent);
  NS_IMETHOD GetDocShell(nsIDocShell** aDocShell);
  NS_IMETHOD Dump(const nsAString& aStr)
  {
    return mMessageManager ? mMessageManager->Dump(aStr) : NS_OK;
  }

  NS_IMETHOD AddEventListener(const nsAString& aType,
                              nsIDOMEventListener* aListener,
                              PRBool aUseCapture)
  {
    
    return nsDOMEventTargetHelper::AddEventListener(aType, aListener,
                                                    aUseCapture, PR_FALSE, 1);
  }
  NS_IMETHOD AddEventListener(const nsAString& aType,
                              nsIDOMEventListener* aListener,
                              PRBool aUseCapture, PRBool aWantsUntrusted,
                              PRUint8 optional_argc)
  {
    return nsDOMEventTargetHelper::AddEventListener(aType, aListener,
                                                    aUseCapture,
                                                    aWantsUntrusted,
                                                    optional_argc);
  }

  virtual nsIScriptObjectPrincipal* GetObjectPrincipal() { return this; }
  virtual JSContext* GetJSContextForEventHandlers();
  virtual nsIPrincipal* GetPrincipal();

  nsCOMPtr<nsIContentFrameMessageManager> mMessageManager;
  TabChild* mTabChild;
};

class ContentListener : public nsIDOMEventListener
{
public:
  ContentListener(TabChild* aTabChild) : mTabChild(aTabChild) {}
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENTLISTENER
protected:
  TabChild* mTabChild;
};

class TabChild : public PIFrameEmbeddingChild,
                 public nsIWebBrowserChrome2,
                 public nsIEmbeddingSiteWindow2,
                 public nsIWebBrowserChromeFocus,
                 public nsIInterfaceRequestor,
                 public nsIWindowProvider
{
public:
    TabChild();
    virtual ~TabChild();
    bool destroyWidget();
    nsresult Init();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBBROWSERCHROME
    NS_DECL_NSIWEBBROWSERCHROME2
    NS_DECL_NSIEMBEDDINGSITEWINDOW
    NS_DECL_NSIEMBEDDINGSITEWINDOW2
    NS_DECL_NSIWEBBROWSERCHROMEFOCUS
    NS_DECL_NSIINTERFACEREQUESTOR
    NS_DECL_NSIWINDOWPROVIDER

    virtual bool RecvcreateWidget(const MagicWindowHandle& parentWidget);
    virtual bool RecvloadURL(const nsCString& uri);
    virtual bool Recvmove(const PRUint32& x,
                          const PRUint32& y,
                          const PRUint32& width,
                          const PRUint32& height);
    virtual bool Recvactivate();
    virtual bool RecvsendMouseEvent(const nsString& aType,
                                    const PRInt32&  aX,
                                    const PRInt32&  aY,
                                    const PRInt32&  aButton,
                                    const PRInt32&  aClickCount,
                                    const PRInt32&  aModifiers,
                                    const bool&     aIgnoreRootScrollFrame);
    virtual bool RecvsendKeyEvent(const nsString& aType,
                                  const PRInt32&  aKeyCode,
                                  const PRInt32&  aCharCode,
                                  const PRInt32&  aModifiers,
                                  const bool&     aPreventDefault);
    virtual bool RecvactivateFrameEvent(const nsString& aType, const bool& capture);
    virtual bool RecvloadRemoteScript(const nsString& aURL);
    virtual bool RecvsendAsyncMessageToChild(const nsString& aMessage,
                                             const nsString& aJSON);
    virtual mozilla::ipc::PDocumentRendererChild* AllocPDocumentRenderer(
            const PRInt32& x,
            const PRInt32& y,
            const PRInt32& w,
            const PRInt32& h,
            const nsString& bgcolor,
            const PRUint32& flags,
            const bool& flush);
    virtual bool DeallocPDocumentRenderer(PDocumentRendererChild* actor);
    virtual bool RecvPDocumentRendererConstructor(
            mozilla::ipc::PDocumentRendererChild *__a,
            const PRInt32& x,
            const PRInt32& y,
            const PRInt32& w,
            const PRInt32& h,
            const nsString& bgcolor,
            const PRUint32& flags,
            const bool& flush);

    virtual PDocumentRendererShmemChild* AllocPDocumentRendererShmem(
            const PRInt32& x,
            const PRInt32& y,
            const PRInt32& w,
            const PRInt32& h,
            const nsString& bgcolor,
            const PRUint32& flags,
            const bool& flush,
            const gfxMatrix& aMatrix,
            const PRInt32& bufw,
            const PRInt32& bufh,
            Shmem& buf);
    virtual bool DeallocPDocumentRendererShmem(PDocumentRendererShmemChild* actor);
    virtual bool RecvPDocumentRendererShmemConstructor(
            PDocumentRendererShmemChild *__a,
            const PRInt32& aX,
            const PRInt32& aY,
            const PRInt32& aW,
            const PRInt32& aH,
            const nsString& bgcolor,
            const PRUint32& flags,
            const bool& flush,
            const gfxMatrix& aMatrix,
            const PRInt32& aBufW,
            const PRInt32& aBufH,
            Shmem& aBuf);

    nsIWebNavigation* WebNavigation() { return mWebNav; }

    JSContext* GetJSContext() { return mCx; }

    nsIPrincipal* GetPrincipal() { return mPrincipal; }
private:
    bool InitTabChildGlobal();

    nsCOMPtr<nsIWebNavigation> mWebNav;

    nsCOMPtr<nsIXPConnectJSObjectHolder> mRootGlobal;

    JSContext* mCx;

    nsCOMPtr<nsIChannel> mChannel;

    TabChildGlobal* mTabChildGlobal;

    nsCOMPtr<nsIPrincipal> mPrincipal;

    DISALLOW_EVIL_CONSTRUCTORS(TabChild);
};

}
}

#endif 
