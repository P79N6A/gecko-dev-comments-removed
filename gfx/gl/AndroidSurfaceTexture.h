





#ifndef AndroidSurfaceTexture_h__
#define AndroidSurfaceTexture_h__
#ifdef MOZ_WIDGET_ANDROID

#include <jni.h>
#include "nsIRunnable.h"
#include "gfxPlatform.h"
#include "GLDefs.h"

#include "AndroidNativeWindow.h"

class gfxASurface;

namespace mozilla {
namespace gfx {
class Matrix4x4;
}
}

namespace mozilla {
namespace gl {






class AndroidSurfaceTexture {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(AndroidSurfaceTexture)

public:
  static AndroidSurfaceTexture* Create(GLuint aTexture);
  static AndroidSurfaceTexture* Find(int id);

  
  
  static bool Check();

  AndroidNativeWindow* NativeWindow() {
    return mNativeWindow;
  }

  
  void UpdateTexImage();

  bool GetTransformMatrix(mozilla::gfx::Matrix4x4& aMatrix);
  int ID() { return mID; }

  void SetDefaultSize(mozilla::gfx::IntSize size);

  
  
  void SetFrameAvailableCallback(nsIRunnable* aRunnable);

  
  
  void NotifyFrameAvailable();

  GLuint Texture() { return mTexture; }
  jobject JavaSurface() { return mSurface; }
private:
  AndroidSurfaceTexture();
  ~AndroidSurfaceTexture();

  bool Init(GLuint aTexture);

  GLuint mTexture;
  jobject mSurfaceTexture;
  jobject mSurface;

  RefPtr<AndroidNativeWindow> mNativeWindow;
  int mID;
  nsRefPtr<nsIRunnable> mFrameAvailableCallback;
};
  
}
}


#endif
#endif
