



































#include "mozilla/ipc/DocumentRendererShmemParent.h"

using namespace mozilla::ipc;

DocumentRendererShmemParent::DocumentRendererShmemParent()
{}

DocumentRendererShmemParent::~DocumentRendererShmemParent()
{}

void
DocumentRendererShmemParent::SetCanvas(nsICanvasRenderingContextInternal* aCanvas)
{
    mCanvas = aCanvas;
}

bool
DocumentRendererShmemParent::Recv__delete__(const PRInt32& x, const PRInt32& y,
                                            const PRInt32& w, const PRInt32& h,
                                            Shmem& data)
{
    mCanvas->Swap(data, x, y, w, h);
    return true;
}
