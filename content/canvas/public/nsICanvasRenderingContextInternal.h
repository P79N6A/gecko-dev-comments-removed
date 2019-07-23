




































#ifndef nsICanvasRenderingContextInternal_h___
#define nsICanvasRenderingContextInternal_h___

#include "nsISupports.h"
#include "nsICanvasElement.h"
#include "nsIInputStream.h"


#define NS_ICANVASRENDERINGCONTEXTINTERNAL_IID \
  { 0xeab854fd, 0xaa5e, 0x44bb, { 0x8c, 0xc5, 0x8d, 0x02, 0xf8, 0x4b, 0x02, 0x16 } }

class gfxContext;

class nsICanvasRenderingContextInternal : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICANVASRENDERINGCONTEXTINTERNAL_IID)

  
  
  NS_IMETHOD SetCanvasElement(nsICanvasElement* aParentCanvas) = 0;

  
  
  NS_IMETHOD SetDimensions(PRInt32 width, PRInt32 height) = 0;

  
  NS_IMETHOD Render(gfxContext *ctx) = 0;

  
  
  
  
  
  
  NS_IMETHOD GetInputStream(const char *aMimeType,
                            const PRUnichar *aEncoderOptions,
                            nsIInputStream **aStream) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICanvasRenderingContextInternal,
                              NS_ICANVASRENDERINGCONTEXTINTERNAL_IID)

#endif 
