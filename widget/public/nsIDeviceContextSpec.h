




































#ifndef nsIDeviceContextSpec_h___
#define nsIDeviceContextSpec_h___

#include "nsIDeviceContext.h"
#include "prtypes.h"

class nsIWidget;
class nsIPrintSettings;

#ifdef MOZ_CAIRO_GFX
class gfxASurface;
#endif

#define NS_IDEVICE_CONTEXT_SPEC_IID   \
{ 0x205c614f, 0x39f8, 0x42e1, \
{ 0x92, 0x53, 0x04, 0x9b, 0x48, 0xc3, 0xcb, 0xd8 } }

class nsIDeviceContextSpec : public nsISupports
{
public:
   NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDEVICE_CONTEXT_SPEC_IID)

   






   NS_IMETHOD Init(nsIWidget *aWidget,
                   nsIPrintSettings* aPrintSettings,
                   PRBool aIsPrintPreview) = 0;

#ifdef MOZ_CAIRO_GFX
   NS_IMETHOD GetSurfaceForPrinter(gfxASurface **nativeSurface) = 0;

   NS_IMETHOD BeginDocument(PRUnichar*  aTitle,
                            PRUnichar*  aPrintToFileName,
                            PRInt32     aStartPage, 
                            PRInt32     aEndPage) = 0;

   NS_IMETHOD EndDocument() = 0;
   NS_IMETHOD BeginPage() = 0;
   NS_IMETHOD EndPage() = 0;


#endif
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDeviceContextSpec,
                              NS_IDEVICE_CONTEXT_SPEC_IID)

#endif
