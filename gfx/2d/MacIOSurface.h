





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
                                                             double aContentsScaleFactor = 1.0,
                                                             bool aHasAlpha = true);
  static void ReleaseIOSurface(MacIOSurface *aIOSurface);
  static mozilla::TemporaryRef<MacIOSurface> LookupSurface(IOSurfaceID aSurfaceID,
                                                           double aContentsScaleFactor = 1.0,
                                                           bool aHasAlpha = true);

  MacIOSurface(const void *aIOSurfacePtr, double aContentsScaleFactor = 1.0, bool aHasAlpha = true)
    : mIOSurfacePtr(aIOSurfacePtr), mContentsScaleFactor(aContentsScaleFactor), mHasAlpha(aHasAlpha) {}
  ~MacIOSurface();
  IOSurfaceID GetIOSurfaceID();
  void *GetBaseAddress();
  
  
  
  
  size_t GetWidth();
  size_t GetHeight();
  double GetContentsScaleFactor() { return mContentsScaleFactor; }
  size_t GetDevicePixelWidth();
  size_t GetDevicePixelHeight();
  size_t GetBytesPerRow();
  void Lock();
  void Unlock();
  bool HasAlpha() { return mHasAlpha; }
  
  
  CGLError CGLTexImageIOSurface2D(CGLContextObj ctxt);
  mozilla::TemporaryRef<SourceSurface> GetAsSurface();
  CGContextRef CreateIOSurfaceContext();

  
  static CGImageRef CreateImageFromIOSurfaceContext(CGContextRef aContext);
  static mozilla::TemporaryRef<MacIOSurface> IOSurfaceContextGetSurface(CGContextRef aContext,
                                                                        double aContentsScaleFactor = 1.0,
                                                                        bool aHasAlpha = true);

private:
  friend class nsCARenderer;
  const void* mIOSurfacePtr;
  double mContentsScaleFactor;
  bool mHasAlpha;
};

#endif
#endif
