





































#include <string.h>

#include "nsIFactory.h"
#include "nsIGenericFactory.h"
#include "nsISupports.h"

#include "nsIFrame.h"

#include "nsIDOMHTMLCanvasElement.h"
#include "nsIDOMCanvasRenderingContext2D.h"
#include "nsICanvasRenderingContextInternal.h"

#include "nsIReftestHelper.h"

#include "gfxContext.h"
#include "gfxImageSurface.h"

class nsReftestHelper :
    public nsIReftestHelper
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIREFTESTHELPER
};

NS_IMPL_ISUPPORTS1(nsReftestHelper, nsIReftestHelper)

static already_AddRefed<gfxImageSurface>
CanvasToImageSurface(nsIDOMHTMLCanvasElement *canvas)
{
    PRUint32 w, h;
    nsresult rv;

    nsCOMPtr<nsICanvasElement> elt = do_QueryInterface(canvas);
    rv = elt->GetSize(&w, &h);
    if (NS_FAILED(rv))
        return nsnull;

    nsCOMPtr<nsISupports> ctxg;
    rv = canvas->GetContext(NS_LITERAL_STRING("2d"), getter_AddRefs(ctxg));
    if (NS_FAILED(rv))
        return nsnull;

    nsCOMPtr<nsICanvasRenderingContextInternal> ctx = do_QueryInterface(ctxg);
    if (ctx == nsnull)
        return nsnull;

    nsRefPtr<gfxImageSurface> img = new gfxImageSurface(gfxIntSize(w, h), gfxASurface::ImageFormatARGB32);
    if (img == nsnull)
        return nsnull;

    nsRefPtr<gfxContext> t = new gfxContext(img);
    if (t == nsnull)
        return nsnull;

    t->SetOperator(gfxContext::OPERATOR_CLEAR);
    t->Paint();

    rv = ctx->RenderToSurface(img->CairoSurface());
    if (NS_FAILED(rv))
        return nsnull;

    NS_ADDREF(img.get());
    return img.get();
}

NS_IMETHODIMP
nsReftestHelper::CompareCanvas(nsIDOMHTMLCanvasElement *canvas1,
                               nsIDOMHTMLCanvasElement *canvas2,
                               PRUint32 *result)
{
    if (canvas1 == nsnull ||
        canvas2 == nsnull ||
        result == nsnull)
        return NS_ERROR_FAILURE;

    nsRefPtr<gfxImageSurface> img1 = CanvasToImageSurface(canvas1);
    nsRefPtr<gfxImageSurface> img2 = CanvasToImageSurface(canvas2);

    if (img1 == nsnull || img2 == nsnull ||
        img1->GetSize() != img2->GetSize() ||
        img1->Stride() != img2->Stride())
        return NS_ERROR_FAILURE;

    int v;
    gfxIntSize size = img1->GetSize();
    PRUint32 stride = img1->Stride();

    
    if (stride == size.width * 4) {
        v = memcmp(img1->Data(), img2->Data(), size.width * size.height * 4);
        if (v == 0) {
            *result = 0;
            return NS_OK;
        }
    }

    PRUint32 different = 0;
    for (int j = 0; j < size.height; j++) {
        unsigned char *p1 = img1->Data() + j*stride;
        unsigned char *p2 = img2->Data() + j*stride;
        v = memcmp(p1, p2, stride);

        if (v) {
            for (int i = 0; i < size.width; i++) {
                if (*(PRUint32*) p1 != *(PRUint32*) p2)
                    different++;
                p1 += 4;
                p2 += 4;
            }
        }
    }

    *result = different;
    return NS_OK;
}


NS_GENERIC_FACTORY_CONSTRUCTOR(nsReftestHelper)

static const nsModuleComponentInfo components[] =
{
    { "nsReftestHelper",
      { 0x0269F9AD, 0xA2BD, 0x4AEA,
        { 0xB9, 0x2F, 0xCB, 0xF2, 0x23, 0x80, 0x94, 0x54 } },
      "@mozilla.org/reftest-helper;1",
      nsReftestHelperConstructor },
};

NS_IMPL_NSGETMODULE(nsReftestModule, components)

