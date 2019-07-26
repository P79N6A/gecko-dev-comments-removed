



#include "mozilla/ipc/DocumentRendererParent.h"
#include "gfxImageSurface.h"
#include "gfxPattern.h"
#include "nsICanvasRenderingContextInternal.h"

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

void DocumentRendererParent::DrawToCanvas(const nsIntSize& aSize,
                                          const nsCString& aData)
{
    if (!mCanvas || !mCanvasContext)
        return;

    nsRefPtr<gfxImageSurface> surf =
        new gfxImageSurface(reinterpret_cast<uint8_t*>(const_cast<nsCString&>(aData).BeginWriting()),
                            gfxIntSize(aSize.width, aSize.height),
                            aSize.width * 4,
                            gfxImageFormat::ARGB32);
    nsRefPtr<gfxPattern> pat = new gfxPattern(surf);

    gfxRect rect(gfxPoint(0, 0), gfxSize(aSize.width, aSize.height));
    mCanvasContext->NewPath();
    mCanvasContext->PixelSnappedRectangleAndSetPattern(rect, pat);
    mCanvasContext->Fill();

    
    
    mCanvasContext->SetColor(gfxRGBA(1,1,1,1));

    gfxRect damageRect = mCanvasContext->UserToDevice(rect);
    mCanvas->Redraw(damageRect);
}

void
DocumentRendererParent::ActorDestroy(ActorDestroyReason aWhy)
{
  
}

bool
DocumentRendererParent::Recv__delete__(const nsIntSize& renderedSize,
                                       const nsCString& data)
{
    DrawToCanvas(renderedSize, data);
    return true;
}
