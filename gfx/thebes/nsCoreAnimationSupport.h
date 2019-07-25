





#ifndef nsCoreAnimationSupport_h__
#define nsCoreAnimationSupport_h__
#ifdef XP_MACOSX

#import <OpenGL/OpenGL.h>
#import "ApplicationServices/ApplicationServices.h"
#include "nscore.h"
#include "gfxTypes.h"
#include "nsAutoPtr.h"
#include "nsIOSurface.h"


CGColorSpaceRef THEBES_API CreateSystemColorSpace();


struct _CGLPBufferObject;
struct _CGLContextObject;

enum AllowOfflineRendererEnum { ALLOW_OFFLINE_RENDERER, DISALLOW_OFFLINE_RENDERER };

class THEBES_API nsCARenderer {
  NS_INLINE_DECL_REFCOUNTING(nsCARenderer)
public:
  nsCARenderer() : mCARenderer(nullptr), mFBOTexture(0), mOpenGLContext(nullptr),
                   mCGImage(nullptr), mCGData(nullptr), mIOSurface(nullptr), mFBO(0),
                   mIOTexture(0),
                   mUnsupportedWidth(UINT32_MAX), mUnsupportedHeight(UINT32_MAX),
                   mAllowOfflineRenderer(DISALLOW_OFFLINE_RENDERER) {}
  ~nsCARenderer();
  nsresult SetupRenderer(void* aCALayer, int aWidth, int aHeight,
                         AllowOfflineRendererEnum aAllowOfflineRenderer);
  nsresult Render(int aWidth, int aHeight, CGImageRef *aOutCAImage);
  bool isInit() { return mCARenderer != nullptr; }
  




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
  void SetBounds(int aWidth, int aHeight);
  void SetViewport(int aWidth, int aHeight);
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

#endif 
#endif 

