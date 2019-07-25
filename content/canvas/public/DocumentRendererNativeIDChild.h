



































#ifndef mozilla_dom_DocumentRendererNativeIDChild
#define mozilla_dom_DocumentRendererNativeIDChild

#include "mozilla/ipc/PDocumentRendererNativeIDChild.h"

class nsIDOMWindow;
class gfxMatrix;

namespace mozilla {
namespace ipc {

class DocumentRendererNativeIDChild : public PDocumentRendererNativeIDChild
{
public:
    DocumentRendererNativeIDChild();
    virtual ~DocumentRendererNativeIDChild();

    bool RenderDocument(nsIDOMWindow* window, const PRInt32& x,
                        const PRInt32& y, const PRInt32& w,
                        const PRInt32& h, const nsString& aBGColor,
                        const PRUint32& flags, const PRBool& flush,
                        const gfxMatrix& aMatrix,
                        const PRInt32& nativeID);

private:

    DISALLOW_EVIL_CONSTRUCTORS(DocumentRendererNativeIDChild);
};

}
}

#endif
