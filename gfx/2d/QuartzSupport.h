





#ifndef nsCoreAnimationSupport_h__
#define nsCoreAnimationSupport_h__
#ifdef XP_MACOSX

#import <OpenGL/OpenGL.h>
#import "ApplicationServices/ApplicationServices.h"
#include "gfxTypes.h"
#include "mozilla/RefPtr.h"
#include "mozilla/gfx/MacIOSurface.h"


CGColorSpaceRef THEBES_API CreateSystemColorSpace();


struct _CGLPBufferObject;
struct _CGLContextObject;

enum AllowOfflineRendererEnum { ALLOW_OFFLINE_RENDERER, DISALLOW_OFFLINE_RENDERER };

class nsCARenderer : public mozilla::RefCounted<nsCARenderer> {
public:
  nsCARenderer() : mCARenderer(nullptr), mWrapperCALayer(nullptr), mFBOTexture(0),
                   mOpenGLContext(nullptr), mCGImage(nullptr), mCGData(nullptr),
                   mIOSurface(nullptr), mFBO(0), mIOTexture(0),
                   mUnsupportedWidth(UINT32_MAX), mUnsupportedHeight(UINT32_MAX),
                   mAllowOfflineRenderer(DISALLOW_OFFLINE_RENDERER),
                   mContentsScaleFactor(1.0) {}
  ~nsCARenderer();
  
  
  
  
  nsresult SetupRenderer(void* aCALayer, int aWidth, int aHeight,
                         double aContentsScaleFactor,
                         AllowOfflineRendererEnum aAllowOfflineRenderer);
  
  
  nsresult Render(int aWidth, int aHeight,
                  double aContentsScaleFactor,
                  CGImageRef *aOutCAImage);
  bool isInit() { return mCARenderer != nullptr; }
  




  void AttachIOSurface(mozilla::RefPtr<MacIOSurface> aSurface);
  IOSurfaceID GetIOSurfaceID();
  
  
  static nsresult DrawSurfaceToCGContext(CGContextRef aContext,
                                         MacIOSurface *surf,
                                         CGColorSpaceRef aColorSpace,
                                         int aX, int aY,
                                         size_t aWidth, size_t aHeight);

  
  
  void DetachCALayer();
  void AttachCALayer(void *aCALayer);
#ifdef DEBUG
  static void SaveToDisk(MacIOSurface *surf);
#endif
private:
  
  
  void SetBounds(int aWidth, int aHeight);
  
  
  void SetViewport(int aWidth, int aHeight);
  void Destroy();

  void *mCARenderer;
  void *mWrapperCALayer;
  GLuint                    mFBOTexture;
  _CGLContextObject        *mOpenGLContext;
  CGImageRef                mCGImage;
  void                     *mCGData;
  mozilla::RefPtr<MacIOSurface> mIOSurface;
  uint32_t                  mFBO;
  uint32_t                  mIOTexture;
  uint32_t                  mUnsupportedWidth;
  uint32_t                  mUnsupportedHeight;
  AllowOfflineRendererEnum  mAllowOfflineRenderer;
  double                    mContentsScaleFactor;
};

enum CGContextType {
  CG_CONTEXT_TYPE_UNKNOWN = 0,
  
  CG_CONTEXT_TYPE_BITMAP = 4,
  CG_CONTEXT_TYPE_IOSURFACE = 8
};

CGContextType GetContextType(CGContextRef ref);

#endif 
#endif 

