





#ifndef nsSurfaceTexture_h__
#define nsSurfaceTexture_h__
#ifdef MOZ_WIDGET_ANDROID

#include <jni.h>
#include "nsIRunnable.h"
#include "gfxPlatform.h"
#include "GLDefs.h"

class gfxASurface;

namespace mozilla {
namespace gfx {
class Matrix4x4;
}
}






class nsSurfaceTexture {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(nsSurfaceTexture)

public:
  static nsSurfaceTexture* Create(GLuint aTexture);
  static nsSurfaceTexture* Find(int id);

  
  
  static bool Check();
  
  ~nsSurfaceTexture();

  
  
  void* GetNativeWindow();

  
  void UpdateTexImage();

  bool GetTransformMatrix(mozilla::gfx::Matrix4x4& aMatrix);
  int ID() { return mID; }

  
  
  void SetFrameAvailableCallback(nsIRunnable* aRunnable);

  
  
  void NotifyFrameAvailable();
private:
  nsSurfaceTexture();

  bool Init(GLuint aTexture);

  jobject mSurfaceTexture;
  void* mNativeWindow;
  int mID;
  nsRefPtr<nsIRunnable> mFrameAvailableCallback;
};

#endif
#endif
