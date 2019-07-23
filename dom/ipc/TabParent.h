





































#ifndef mozilla_tabs_TabParent_h
#define mozilla_tabs_TabParent_h

#include "mozilla/dom/PIFrameEmbeddingParent.h"

#include "mozilla/ipc/GeckoChildProcessHost.h"

class nsIURI;
class nsIDOMElement;

namespace mozilla {
namespace dom {

class TabParent : public PIFrameEmbeddingParent
{
public:
    TabParent();
    virtual ~TabParent();
    void SetOwnerElement(nsIDOMElement* aElement) { mFrameElement = aElement; }

    virtual bool RecvmoveFocus(const bool& aForward);

    void LoadURL(nsIURI* aURI);
    void Move(PRUint32 x, PRUint32 y, PRUint32 width, PRUint32 height);
    void Activate();

    virtual mozilla::ipc::PDocumentRendererParent* AllocPDocumentRenderer(
            const PRInt32& x,
            const PRInt32& y,
            const PRInt32& w,
            const PRInt32& h,
            const nsString& bgcolor,
            const PRUint32& flags,
            const bool& flush);
    virtual bool DeallocPDocumentRenderer(
            mozilla::ipc::PDocumentRendererParent* __a,
            const PRUint32& w,
            const PRUint32& h,
            const nsCString& data);
    virtual bool RecvPDocumentRendererDestructor(
            mozilla::ipc::PDocumentRendererParent* __a,
            const PRUint32& w,
            const PRUint32& h,
            const nsCString& data);
protected:
    nsIDOMElement* mFrameElement;
};

} 
} 

#endif
