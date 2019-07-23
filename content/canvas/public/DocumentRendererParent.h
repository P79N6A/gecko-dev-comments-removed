



































#ifndef mozilla_dom_DocumentRendererParent
#define mozilla_dom_DocumentRendererParent

#include "mozilla/ipc/PDocumentRendererParent.h"
#include "nsICanvasRenderingContextInternal.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "gfxContext.h"

namespace mozilla {
namespace ipc {

class DocumentRendererParent : public PDocumentRendererParent
{
public:
    DocumentRendererParent();
    virtual ~DocumentRendererParent();

    void SetCanvasContext(nsICanvasRenderingContextInternal* aCanvas,
			  gfxContext* ctx);
    void DrawToCanvas(PRUint32 aWidth, PRUint32 aHeight,
		      const nsCString& aData);

    virtual bool Recv__delete__(const PRUint32& w, const PRUint32& h,
                                const nsCString& data);

private:
    nsCOMPtr<nsICanvasRenderingContextInternal> mCanvas;
    nsRefPtr<gfxContext> mCanvasContext;

    DISALLOW_EVIL_CONSTRUCTORS(DocumentRendererParent);
};

}
}

#endif
