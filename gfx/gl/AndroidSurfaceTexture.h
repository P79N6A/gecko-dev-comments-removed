





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

  
  
  bool CanDetach() { return mCanDetach; }

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
  const widget::sdk::Surface::Ref& JavaSurface() { return mSurface; }

private:
  AndroidSurfaceTexture();
  ~AndroidSurfaceTexture();

  bool Init(GLContext* aContext, GLuint aTexture);
  void UpdateCanDetach();

  GLuint mTexture;
  widget::sdk::SurfaceTexture::GlobalRef mSurfaceTexture;
  widget::sdk::Surface::GlobalRef mSurface;

  Monitor mMonitor;
  GLContext* mAttachedContext;
  bool mCanDetach;

  RefPtr<AndroidNativeWindow> mNativeWindow;
  int mID;
  nsCOMPtr<nsIRunnable> mFrameAvailableCallback;
};

}
}


#endif
#endif
