





#ifndef MacIOSurface_h__
#define MacIOSurface_h__
#ifdef XP_MACOSX

#import <OpenGL/OpenGL.h>
#include "2D.h"
#include "mozilla/RefPtr.h"

class gfxASurface;
struct _CGLContextObject;

typedef _CGLContextObject* CGLContextObj;
typedef struct CGContext* CGContextRef;
typedef struct CGImage* CGImageRef;
typedef uint32_t IOSurfaceID;

class MacIOSurface : public mozilla::RefCounted<MacIOSurface> {
public:
  typedef mozilla::gfx::SourceSurface SourceSurface;

  static mozilla::TemporaryRef<MacIOSurface> CreateIOSurface(int aWidth, int aHeight,
                                                             double aContentsScaleFactor = 1.0);
  static void ReleaseIOSurface(MacIOSurface *aIOSurface);
  static mozilla::TemporaryRef<MacIOSurface> LookupSurface(IOSurfaceID aSurfaceID,
                                                           double aContentsScaleFactor = 1.0);

  MacIOSurface(const void *aIOSurfacePtr, double aContentsScaleFactor = 1.0)
    : mIOSurfacePtr(aIOSurfacePtr), mContentsScaleFactor(aContentsScaleFactor) {}
  ~MacIOSurface();
  IOSurfaceID GetIOSurfaceID();
  void *GetBaseAddress();
  
  
  
  
  
  size_t GetWidth();
  size_t GetHeight();
  double GetContentsScaleFactor() { return mContentsScaleFactor; }
  size_t GetBytesPerRow();
  void Lock();
  void Unlock();
  
  
  CGLError CGLTexImageIOSurface2D(void *ctxt,
                                  GLenum internalFormat, GLenum format,
                                  GLenum type, GLuint plane);
  mozilla::TemporaryRef<SourceSurface> GetAsSurface();
  CGContextRef CreateIOSurfaceContext();

  
  static CGImageRef CreateImageFromIOSurfaceContext(CGContextRef aContext);
  static mozilla::TemporaryRef<MacIOSurface> IOSurfaceContextGetSurface(CGContextRef aContext,
                                                                        double aContentsScaleFactor = 1.0);

private:
  friend class nsCARenderer;
  const void* mIOSurfacePtr;
  double mContentsScaleFactor;
};

#endif
#endif
