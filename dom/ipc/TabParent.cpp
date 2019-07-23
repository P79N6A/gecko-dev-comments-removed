





































#include "TabParent.h"

#include "mozilla/ipc/GeckoThread.h"
#include "mozilla/ipc/DocumentRendererParent.h"

#include "nsIURI.h"

using mozilla::ipc::BrowserProcessSubThread;
using mozilla::ipc::DocumentRendererParent;

namespace mozilla {
namespace dom {

TabParent::TabParent()
{
}

TabParent::~TabParent()
{
}

void
TabParent::LoadURL(nsIURI* aURI)
{
    nsCString spec;
    aURI->GetSpec(spec);

    SendloadURL(spec);
}

void
TabParent::Move(PRUint32 x, PRUint32 y, PRUint32 width, PRUint32 height)
{
    Sendmove(x, y, width, height);
}

mozilla::ipc::PDocumentRendererParent*
TabParent::AllocPDocumentRenderer(const PRInt32& x,
        const PRInt32& y, const PRInt32& w, const PRInt32& h, const nsString& bgcolor,
        const PRUint32& flags, const bool& flush)
{
    return new DocumentRendererParent();
}

bool
TabParent::DeallocPDocumentRenderer(mozilla::ipc::PDocumentRendererParent* __a,
        const PRUint32& w, const PRUint32& h, const nsCString& data)
{
    NS_ENSURE_ARG_POINTER(__a);
    delete __a;
    return true;
}

bool
TabParent::RecvPDocumentRendererDestructor(PDocumentRendererParent* __a,
        const PRUint32& w, const PRUint32& h, const nsCString& data)
{
    NS_ENSURE_ARG_POINTER(__a);
    DocumentRendererParent *renderer = static_cast<DocumentRendererParent *>(__a);
    renderer->DrawToCanvas(w, h, data);

    return true;
}

} 
} 
