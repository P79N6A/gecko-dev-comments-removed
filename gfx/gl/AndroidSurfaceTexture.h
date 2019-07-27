





#ifndef AndroidSurfaceTexture_h__
#define AndroidSurfaceTexture_h__
#ifdef MOZ_WIDGET_ANDROID

#include <jni.h>
#include "nsIRunnable.h"
#include "gfxPlatform.h"
#include "GLDefs.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/Monitor.h"

#include "AndroidNativeWindow.h"

class gfxASurface;

namespace mozilla {
namespace gfx {
class Matrix4x4;
}
}

namespace mozilla {
namespace gl {

class GLContext;






class AndroidSurfaceTexture {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(AndroidSurfaceTexture)

public:

  
  
  static AndroidSurfaceTexture* Create(GLContext* aGLContext, GLuint aTexture);

  
  
  
  
  static AndroidSurfaceTexture* Create();

  static AndroidSurfaceTexture* Find(int id);

  
  
  static bool Check();

  
  
  
  
  
  
  bool Attach(GLContext* aContext, PRIntervalTime aTiemout = PR_INTERVAL_NO_TIMEOUT);

  
  bool Detach();

  GLContext* GetAttachedContext() { return mAttachedContext; }

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

  bool Init(GLContext* aContext, GLuint aTexture);

  GLuint mTexture;
  jobject mSurfaceTexture;
  jobject mSurface;

  Monitor mMonitor;
  GLContext* mAttachedContext;

  RefPtr<AndroidNativeWindow> mNativeWindow;
  int mID;
  nsRefPtr<nsIRunnable> mFrameAvailableCallback;
};

}
}


#endif
#endif
