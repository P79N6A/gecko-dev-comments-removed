





#ifndef AndroidSurfaceTexture_h__
#define AndroidSurfaceTexture_h__
#ifdef MOZ_WIDGET_ANDROID

#include <jni.h>
#include "nsIRunnable.h"
#include "gfxPlatform.h"
#include "GLDefs.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/Monitor.h"

#include "SurfaceTexture.h"
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

  
  
  static TemporaryRef<AndroidSurfaceTexture> Create(GLContext* aGLContext, GLuint aTexture);

  
  
  
  
  static TemporaryRef<AndroidSurfaceTexture> Create();

  static AndroidSurfaceTexture* Find(int id);

  
  
  
  
  
  
  nsresult Attach(GLContext* aContext, PRIntervalTime aTiemout = PR_INTERVAL_NO_TIMEOUT);

  
  nsresult Detach();

  GLContext* GetAttachedContext() { return mAttachedContext; }

  AndroidNativeWindow* NativeWindow() {
    return mNativeWindow;
  }

  
  void UpdateTexImage();

  void GetTransformMatrix(mozilla::gfx::Matrix4x4& aMatrix);
  int ID() { return mID; }

  void SetDefaultSize(mozilla::gfx::IntSize size);

  
  
  void SetFrameAvailableCallback(nsIRunnable* aRunnable);

  
  
  void NotifyFrameAvailable();

  GLuint Texture() { return mTexture; }
  jobject JavaSurface() { return mSurface->wrappedObject(); }
private:
  AndroidSurfaceTexture();
  ~AndroidSurfaceTexture();

  bool Init(GLContext* aContext, GLuint aTexture);

  GLuint mTexture;
  nsAutoPtr<mozilla::widget::android::sdk::SurfaceTexture> mSurfaceTexture;
  nsAutoPtr<mozilla::widget::android::sdk::Surface> mSurface;

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
