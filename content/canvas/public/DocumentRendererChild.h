



































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
    
    bool RenderDocument(nsIDOMWindow *window, const PRInt32& x, const PRInt32& y, const PRInt32& w, const PRInt32& h,
			    const nsString& bgcolor, const PRUint32& flags, const PRBool& flush, 
			    PRUint32& _width, PRUint32& _height, nsCString& data);

private:

    DISALLOW_EVIL_CONSTRUCTORS(DocumentRendererChild);
};

}
}

#endif
