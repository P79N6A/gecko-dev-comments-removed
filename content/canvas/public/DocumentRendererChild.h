



































#ifndef mozilla_dom_DocumentRendererChild
#define mozilla_dom_DocumentRendererChild

#include "mozilla/ipc/PDocumentRendererChild.h"
#include "nsICanvasRenderingContextInternal.h"
#include "nsString.h"
#include "gfxContext.h"

class nsIDOMWindow;

namespace mozilla {
namespace ipc {

class DocumentRendererChild : public PDocumentRendererChild
{
public:
    DocumentRendererChild();
    virtual ~DocumentRendererChild();
    
    bool RenderDocument(nsIDOMWindow *window,
                        const nsRect& documentRect, const nsString& bgcolor,
                        PRUint32 renderFlags, PRBool flushLayout, 
                        nsIntSize* renderedSize, nsCString& data);

private:

    DISALLOW_EVIL_CONSTRUCTORS(DocumentRendererChild);
};

}
}

#endif
