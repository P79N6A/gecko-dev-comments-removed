




































#ifndef nsICanvasRenderingContextInternal_h___
#define nsICanvasRenderingContextInternal_h___

#include "nsISupports.h"
#include "nsICanvasElement.h"
#include "nsIInputStream.h"
#include "nsIDocShell.h"
#include "gfxPattern.h"


#define NS_ICANVASRENDERINGCONTEXTINTERNAL_IID \
  { 0x3c4632ab, 0x8443, 0x4082, { 0xa8, 0xa3, 0x10, 0xe7, 0xcf, 0xba, 0x4c, 0x74 } }

class gfxContext;
class gfxASurface;

namespace mozilla {
namespace ipc {
class Shmem;
}
}

class nsICanvasRenderingContextInternal : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICANVASRENDERINGCONTEXTINTERNAL_IID)

  
  
  NS_IMETHOD SetCanvasElement(nsICanvasElement* aParentCanvas) = 0;

  
  
  NS_IMETHOD SetDimensions(PRInt32 width, PRInt32 height) = 0;

  NS_IMETHOD InitializeWithSurface(nsIDocShell *docShell, gfxASurface *surface, PRInt32 width, PRInt32 height) = 0;

  
  NS_IMETHOD Render(gfxContext *ctx, gfxPattern::GraphicsFilter aFilter) = 0;

  
  
  
  
  
  
  NS_IMETHOD GetInputStream(const char *aMimeType,
                            const PRUnichar *aEncoderOptions,
                            nsIInputStream **aStream) = 0;

  
  
  NS_IMETHOD GetThebesSurface(gfxASurface **surface) = 0;

  
  
  
  
  NS_IMETHOD SetIsOpaque(PRBool isOpaque) = 0;

  
  NS_IMETHOD Redraw(const gfxRect &dirty) = 0;

  
  
  
  
  NS_IMETHOD SetIsShmem(PRBool isShmem) = 0;

  
  
  NS_IMETHOD Swap(mozilla::ipc::Shmem& back,
                  PRInt32 x, PRInt32 y, PRInt32 w, PRInt32 h) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICanvasRenderingContextInternal,
                              NS_ICANVASRENDERINGCONTEXTINTERNAL_IID)

#endif 
