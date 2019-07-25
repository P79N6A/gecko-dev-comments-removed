






































#ifndef nsIOSurface_h__
#define nsIOSurface_h__
#ifdef XP_MACOSX

#import <OpenGL/OpenGL.h>

class gfxASurface;
struct _CGLContextObject;

typedef _CGLContextObject* CGLContextObj;
typedef uint32_t IOSurfaceID;

class THEBES_API nsIOSurface {
    NS_INLINE_DECL_REFCOUNTING(nsIOSurface)
public:
  static already_AddRefed<nsIOSurface> CreateIOSurface(int aWidth, int aHeight);
  static void ReleaseIOSurface(nsIOSurface *aIOSurface);
  static already_AddRefed<nsIOSurface> LookupSurface(IOSurfaceID aSurfaceID);

  nsIOSurface(const void *aIOSurfacePtr) : mIOSurfacePtr(aIOSurfacePtr) {}
  ~nsIOSurface();
  IOSurfaceID GetIOSurfaceID();
  void *GetBaseAddress();
  size_t GetWidth();
  size_t GetHeight();
  size_t GetBytesPerRow();
  void Lock();
  void Unlock();
  
  
  CGLError CGLTexImageIOSurface2D(void *ctxt,
                                  GLenum internalFormat, GLenum format,
                                  GLenum type, GLuint plane);
  already_AddRefed<gfxASurface> GetAsSurface();
private:
  friend class nsCARenderer;
  const void* mIOSurfacePtr;
};

#endif
#endif
