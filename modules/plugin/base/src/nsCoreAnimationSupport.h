






































#ifndef nsCoreAnimationSupport_h__
#define nsCoreAnimationSupport_h__
#ifdef XP_MACOSX

#import "ApplicationServices/ApplicationServices.h"
#include "nscore.h"


CGColorSpaceRef CreateSystemColorSpace();


struct _CGLPBufferObject;
struct _CGLContextObject;
class nsIOSurface;

class nsCARenderer {
public:
  nsCARenderer() : mCARenderer(nsnull), mPixelBuffer(nsnull), mOpenGLContext(nsnull),
                   mCGImage(nsnull), mCGData(nsnull), mIOSurface(nsnull), mFBO(nsnull),
                   mIOTexture(nsnull) {}
  ~nsCARenderer();
  nsresult SetupRenderer(void* aCALayer, int aWidth, int aHeight);
  nsresult Render(int aWidth, int aHeight, CGImageRef *aOutCAImage);
  bool isInit() { return mCARenderer != nsnull; }
  



 
  void AttachIOSurface(nsIOSurface *aSurface);
  static nsresult DrawSurfaceToCGContext(CGContextRef aContext, 
                                         nsIOSurface *surf, 
                                         CGColorSpaceRef aColorSpace, 
                                         int aX, int aY,
                                         int aWidth, int aHeight);
private:
  void Destroy();

  void *mCARenderer;
  _CGLPBufferObject *mPixelBuffer;
  _CGLContextObject *mOpenGLContext;
  CGImageRef         mCGImage;
  void              *mCGData;
  nsIOSurface       *mIOSurface;
  uint32_t           mFBO;
  uint32_t           mIOTexture;
};

typedef uint32_t IOSurfaceID;

class nsIOSurface {
public:
  static nsIOSurface *CreateIOSurface(int aWidth, int aHeight); 
  static void ReleaseIOSurface(nsIOSurface *aIOSurface); 
  static nsIOSurface *LookupSurface(IOSurfaceID aSurfaceID);

  nsIOSurface(CFTypeRef aIOSurfacePtr) : mIOSurfacePtr(aIOSurfacePtr) {}
  ~nsIOSurface() { CFRelease(mIOSurfacePtr); }
  IOSurfaceID GetIOSurfaceID();
  void *GetBaseAddress();
  size_t GetWidth();
  size_t GetHeight();
  size_t GetBytesPerRow();
  void Lock();
  void Unlock();
private:
  friend class nsCARenderer;
  CFTypeRef mIOSurfacePtr;
};

#endif 
#endif 

