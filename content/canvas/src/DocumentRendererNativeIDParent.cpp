



































#include "mozilla/ipc/DocumentRendererNativeIDParent.h"

using namespace mozilla::ipc;

DocumentRendererNativeIDParent::DocumentRendererNativeIDParent()
{}

DocumentRendererNativeIDParent::~DocumentRendererNativeIDParent()
{}

void
DocumentRendererNativeIDParent::SetCanvas(nsICanvasRenderingContextInternal* aCanvas)
{
    mCanvas = aCanvas;
}

bool
DocumentRendererNativeIDParent::Recv__delete__(const PRInt32& x, const PRInt32& y,
                                               const PRInt32& w, const PRInt32& h,
                                               const PRUint32& nativeID)
{
    mCanvas->Swap(nativeID, x, y, w, h);
    return true;
}
