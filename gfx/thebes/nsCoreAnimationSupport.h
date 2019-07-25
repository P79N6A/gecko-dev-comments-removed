






































#ifndef nsCoreAnimationSupport_h__
#define nsCoreAnimationSupport_h__
#ifdef XP_MACOSX

#import <QuartzCore/QuartzCore.h>
#import "ApplicationServices/ApplicationServices.h"
#include "nscore.h"
#include "gfxTypes.h"
#include "nsAutoPtr.h"


CGColorSpaceRef THEBES_API CreateSystemColorSpace();


struct _CGLPBufferObject;
struct _CGLContextObject;
class nsIOSurface;

enum AllowOfflineRendererEnum { ALLOW_OFFLINE_RENDERER, DISALLOW_OFFLINE_RENDERER };

typedef uint32_t IOSurfaceID;

class THEBES_API nsCARenderer {
  NS_INLINE_DECL_REFCOUNTING(nsCARenderer)
public:
  nsCARenderer() : mCARenderer(nsnull), mFBOTexture(nsnull), mOpenGLContext(nsnull),
                   mCGImage(nsnull), mCGData(nsnull), mIOSurface(nsnull), mFBO(nsnull),
                   mIOTexture(nsnull),
                   mUnsupportedWidth(UINT32_MAX), mUnsupportedHeight(UINT32_MAX),
                   mAllowOfflineRenderer(DISALLOW_OFFLINE_RENDERER) {}
  ~nsCARenderer();
  nsresult SetupRenderer(void* aCALayer, int aWidth, int aHeight,
                         AllowOfflineRendererEnum aAllowOfflineRenderer);
  nsresult Render(int aWidth, int aHeight, CGImageRef *aOutCAImage);
  bool isInit() { return mCARenderer != nsnull; }
  




  void AttachIOSurface(nsRefPtr<nsIOSurface> aSurface);
  IOSurfaceID GetIOSurfaceID();
  static nsresult DrawSurfaceToCGContext(CGContextRef aContext,
                                         nsIOSurface *surf,
                                         CGColorSpaceRef aColorSpace,
                                         int aX, int aY,
                                         size_t aWidth, size_t aHeight);

  
  
  void DettachCALayer();
  void AttachCALayer(void *aCALayer);
#ifdef DEBUG
  static void SaveToDisk(nsIOSurface *surf);
#endif
private:
  void Destroy();

  void *mCARenderer;
  GLuint                    mFBOTexture;
  _CGLContextObject        *mOpenGLContext;
  CGImageRef                mCGImage;
  void                     *mCGData;
  nsRefPtr<nsIOSurface>     mIOSurface;
  uint32_t                  mFBO;
  uint32_t                  mIOTexture;
  uint32_t                  mUnsupportedWidth;
  uint32_t                  mUnsupportedHeight;
  AllowOfflineRendererEnum  mAllowOfflineRenderer;
};

class THEBES_API nsIOSurface {
    NS_INLINE_DECL_REFCOUNTING(nsIOSurface)
public:
  static already_AddRefed<nsIOSurface> CreateIOSurface(int aWidth, int aHeight);
  static void ReleaseIOSurface(nsIOSurface *aIOSurface);
  static already_AddRefed<nsIOSurface> LookupSurface(IOSurfaceID aSurfaceID);

  nsIOSurface(CFTypeRef aIOSurfacePtr) : mIOSurfacePtr(aIOSurfacePtr) {}
  ~nsIOSurface() { CFRelease(mIOSurfacePtr); }
  IOSurfaceID GetIOSurfaceID();
  void *GetBaseAddress();
  size_t GetWidth();
  size_t GetHeight();
  size_t GetBytesPerRow();
  void Lock();
  void Unlock();
  CGLError CGLTexImageIOSurface2D(CGLContextObj ctxt,
                                  GLenum internalFormat, GLenum format, 
                                  GLenum type, GLuint plane);
private:
  friend class nsCARenderer;
  CFTypeRef mIOSurfacePtr;
};

#endif 
#endif 

