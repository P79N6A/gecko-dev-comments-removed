





































#ifndef mozilla_tabs_TabChild_h
#define mozilla_tabs_TabChild_h

#include "mozilla/dom/PIFrameEmbeddingChild.h"
#include "nsIWebNavigation.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIWebBrowserChrome2.h"
#include "nsIEmbeddingSiteWindow2.h"
#include "nsIWebBrowserChromeFocus.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMEventTarget.h"

namespace mozilla {
namespace dom {

class TabChild;

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
                 public nsIWebBrowserChromeFocus
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
    virtual bool RecvactivateFrameEvent(const nsString& aType, const bool& capture);
    virtual mozilla::ipc::PDocumentRendererChild* AllocPDocumentRenderer(
            const PRInt32& x,
            const PRInt32& y,
            const PRInt32& w,
            const PRInt32& h,
            const nsString& bgcolor,
            const PRUint32& flags,
            const bool& flush);
    virtual bool DeallocPDocumentRenderer(
            mozilla::ipc::PDocumentRendererChild* __a,
            const PRUint32& w,
            const PRUint32& h,
            const nsCString& data);
    virtual bool RecvPDocumentRendererConstructor(
            mozilla::ipc::PDocumentRendererChild *__a,
            const PRInt32& x,
            const PRInt32& y,
            const PRInt32& w,
            const PRInt32& h,
            const nsString& bgcolor,
            const PRUint32& flags,
            const bool& flush);

private:
    nsCOMPtr<nsIWebNavigation> mWebNav;

    DISALLOW_EVIL_CONSTRUCTORS(TabChild);
};

}
}

#endif 
