





#ifndef AndroidNativeWindow_h__
#define AndroidNativeWindow_h__
#ifdef MOZ_WIDGET_ANDROID

#include <jni.h>
#include "GLDefs.h"

#include "nsISupports.h"
#include "mozilla/TypedEnum.h"
#include "mozilla/gfx/2D.h"


namespace mozilla {
namespace gl {

enum class AndroidWindowFormat {
  Unknown = -1,
  RGBA_8888 = 1,
  RGBX_8888 = 1 << 1,
  RGB_565 = 1 << 2
};






class AndroidNativeWindow {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(AndroidNativeWindow)

public:

  static AndroidNativeWindow* CreateFromSurface(JNIEnv* aEnv, jobject aSurface);

  gfx::IntSize Size();
  AndroidWindowFormat Format();

  bool SetBuffersGeometry(int32_t aWidth, int32_t aHeight, AndroidWindowFormat aFormat);

  bool Lock(void** out_bits, int32_t* out_width, int32_t* out_height, int32_t* out_stride, AndroidWindowFormat* out_format);
  bool UnlockAndPost();

  void* Handle() { return mWindow; }

protected:
  AndroidNativeWindow(void* aWindow)
    : mWindow(aWindow)
  {

  }

  virtual ~AndroidNativeWindow();

  void* mWindow;
};

}
}


#endif
#endif
