








































#ifndef __LP64__
#import <Carbon/Carbon.h>
#endif

#include "nsRect.h"
#include "nsIWidget.h"
#include "npapi.h"



#ifndef __LP64__

void NS_NPAPI_CarbonWindowFrame(WindowRef aWindow, nsRect& outRect);
#endif


void NS_NPAPI_CocoaWindowFrame(void* aWindow, nsRect& outRect);


NPError NS_NPAPI_ShowCocoaContextMenu(void* menu, nsIWidget* widget, NPCocoaEvent* event);

NPBool NS_NPAPI_ConvertPointCocoa(void* inView,
                                  double sourceX, double sourceY, NPCoordinateSpace sourceSpace,
                                  double *destX, double *destY, NPCoordinateSpace destSpace);


struct _CGLPBufferObject;
struct _CGLContextObject;
class nsCARenderer {
public:
  nsCARenderer() : mCARenderer(NULL), mPixelBuffer(NULL), mOpenGLContext(NULL), 
                    mCGImage(NULL), mCGData(NULL) {}
  ~nsCARenderer();
  nsresult SetupRenderer(void* aCALayer, int aWidth, int aHeight);  
  nsresult Render(CGContextRef aCGContext, int aWidth, int aHeight);  
private:
  void Destroy();

  void *mCARenderer;
  _CGLPBufferObject *mPixelBuffer;
  _CGLContextObject *mOpenGLContext;
  CGImageRef mCGImage;
  void *mCGData;
};
