



































#include "mozilla/ipc/DocumentRendererParent.h"
#include "gfxImageSurface.h"
#include "gfxPattern.h"

using namespace mozilla::ipc;

DocumentRendererParent::DocumentRendererParent()
{}

DocumentRendererParent::~DocumentRendererParent()
{}

void DocumentRendererParent::SetCanvasContext(nsICanvasRenderingContextInternal* aCanvas,
                                              gfxContext* ctx)
{
    mCanvas = aCanvas;
    mCanvasContext = ctx;
}

void DocumentRendererParent::DrawToCanvas(PRUint32 aWidth, PRUint32 aHeight,
                                          const nsCString& aData)
{
    if (!mCanvas || !mCanvasContext)
        return;

    nsRefPtr<gfxImageSurface> surf = new gfxImageSurface(reinterpret_cast<PRUint8*>(const_cast<char*>(aData.Data())),
                                                         gfxIntSize(aWidth, aHeight),
                                                         aWidth * 4,
                                                         gfxASurface::ImageFormatARGB32);
    nsRefPtr<gfxPattern> pat = new gfxPattern(surf);

    mCanvasContext->NewPath();
    mCanvasContext->PixelSnappedRectangleAndSetPattern(gfxRect(0, 0, aWidth, aHeight), pat);
    mCanvasContext->Fill();

    
    mCanvasContext->SetColor(gfxRGBA(1,1,1,1));

    gfxRect damageRect = mCanvasContext->UserToDevice(gfxRect(0, 0, aWidth, aHeight));
    mCanvas->Redraw(damageRect);
}

bool
DocumentRendererParent::Recv__delete__(const PRUint32& w, const PRUint32& h,
                                       const nsCString& data)
{
    DrawToCanvas(w, h, data);
    return true;
}
