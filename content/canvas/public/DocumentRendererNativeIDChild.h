



#ifndef mozilla_dom_DocumentRendererNativeIDChild
#define mozilla_dom_DocumentRendererNativeIDChild

#include "mozilla/ipc/PDocumentRendererNativeIDChild.h"

class nsIDOMWindow;
struct gfxMatrix;

namespace mozilla {
namespace ipc {

class DocumentRendererNativeIDChild : public PDocumentRendererNativeIDChild
{
public:
    DocumentRendererNativeIDChild();
    virtual ~DocumentRendererNativeIDChild();

    bool RenderDocument(nsIDOMWindow* window, const int32_t& x,
                        const int32_t& y, const int32_t& w,
                        const int32_t& h, const nsString& aBGColor,
                        const uint32_t& flags, const bool& flush,
                        const gfxMatrix& aMatrix,
                        const int32_t& nativeID);

private:

    DISALLOW_EVIL_CONSTRUCTORS(DocumentRendererNativeIDChild);
};

}
}

#endif
