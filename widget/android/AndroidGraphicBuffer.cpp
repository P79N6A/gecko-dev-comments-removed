




#include <dlfcn.h>
#include <android/log.h>
#include <GLES2/gl2.h>
#include <nsTArray.h>
#include "AndroidGraphicBuffer.h"
#include "AndroidBridge.h"
#include "mozilla/Preferences.h"

#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "AndroidGraphicBuffer" , ## args)

#define EGL_NATIVE_BUFFER_ANDROID 0x3140
#define EGL_IMAGE_PRESERVED_KHR   0x30D2

typedef void *EGLContext;
typedef void *EGLDisplay;
typedef uint32_t EGLenum;
typedef int32_t EGLint;
typedef uint32_t EGLBoolean;

#define EGL_TRUE 1
#define EGL_FALSE 0
#define EGL_NONE 0x3038
#define EGL_NO_CONTEXT (EGLContext)0
#define EGL_DEFAULT_DISPLAY  (void*)0

#define ANDROID_LIBUI_PATH "libui.so"
#define ANDROID_GLES_PATH "libGLESv2.so"
#define ANDROID_EGL_PATH "libEGL.so"


#define GRAPHIC_BUFFER_SIZE 1024

enum {
    
    GRALLOC_USAGE_SW_READ_NEVER   = 0x00000000,
    
    GRALLOC_USAGE_SW_READ_RARELY  = 0x00000002,
    
    GRALLOC_USAGE_SW_READ_OFTEN   = 0x00000003,
    
    GRALLOC_USAGE_SW_READ_MASK    = 0x0000000F,

    
    GRALLOC_USAGE_SW_WRITE_NEVER  = 0x00000000,
    
    GRALLOC_USAGE_SW_WRITE_RARELY = 0x00000020,
    
    GRALLOC_USAGE_SW_WRITE_OFTEN  = 0x00000030,
    
    GRALLOC_USAGE_SW_WRITE_MASK   = 0x000000F0,

    
    GRALLOC_USAGE_HW_TEXTURE      = 0x00000100,
    
    GRALLOC_USAGE_HW_RENDER       = 0x00000200,
    
    GRALLOC_USAGE_HW_2D           = 0x00000400,
    
    GRALLOC_USAGE_HW_FB           = 0x00001000,
    
    GRALLOC_USAGE_HW_MASK         = 0x00001F00,
};

enum {
    HAL_PIXEL_FORMAT_RGBA_8888          = 1,
    HAL_PIXEL_FORMAT_RGBX_8888          = 2,
    HAL_PIXEL_FORMAT_RGB_888            = 3,
    HAL_PIXEL_FORMAT_RGB_565            = 4,
    HAL_PIXEL_FORMAT_BGRA_8888          = 5,
    HAL_PIXEL_FORMAT_RGBA_5551          = 6,
    HAL_PIXEL_FORMAT_RGBA_4444          = 7,
};

typedef struct ARect {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
} ARect;

static bool gTryRealloc = true;

static class GLFunctions
{
public:
  MOZ_CONSTEXPR GLFunctions() : fGetDisplay(nullptr),
                                fEGLGetError(nullptr),
                                fCreateImageKHR(nullptr),
                                fDestroyImageKHR(nullptr),
                                fImageTargetTexture2DOES(nullptr),
                                fBindTexture(nullptr),
                                fGLGetError(nullptr),
                                fGraphicBufferCtor(nullptr),
                                fGraphicBufferDtor(nullptr),
                                fGraphicBufferLock(nullptr),
                                fGraphicBufferLockRect(nullptr),
                                fGraphicBufferUnlock(nullptr),
                                fGraphicBufferGetNativeBuffer(nullptr),
                                fGraphicBufferReallocate(nullptr),
                                mInitialized(false)
  {
  }

  typedef EGLDisplay (* pfnGetDisplay)(void *display_id);
  pfnGetDisplay fGetDisplay;
  typedef EGLint (* pfnEGLGetError)(void);
  pfnEGLGetError fEGLGetError;

  typedef EGLImageKHR (* pfnCreateImageKHR)(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list);
  pfnCreateImageKHR fCreateImageKHR;
  typedef EGLBoolean (* pfnDestroyImageKHR)(EGLDisplay dpy, EGLImageKHR image);
  pfnDestroyImageKHR fDestroyImageKHR;

  typedef void (* pfnImageTargetTexture2DOES)(GLenum target, EGLImageKHR image);
  pfnImageTargetTexture2DOES fImageTargetTexture2DOES;

  typedef void (* pfnBindTexture)(GLenum target, GLuint texture);
  pfnBindTexture fBindTexture;

  typedef GLenum (* pfnGLGetError)();
  pfnGLGetError fGLGetError;

  typedef void (*pfnGraphicBufferCtor)(void*, uint32_t w, uint32_t h, uint32_t format, uint32_t usage);
  pfnGraphicBufferCtor fGraphicBufferCtor;

  typedef void (*pfnGraphicBufferDtor)(void*);
  pfnGraphicBufferDtor fGraphicBufferDtor;

  typedef int (*pfnGraphicBufferLock)(void*, uint32_t usage, unsigned char **addr);
  pfnGraphicBufferLock fGraphicBufferLock;

  typedef int (*pfnGraphicBufferLockRect)(void*, uint32_t usage, const ARect&, unsigned char **addr);
  pfnGraphicBufferLockRect fGraphicBufferLockRect;

  typedef int (*pfnGraphicBufferUnlock)(void*);
  pfnGraphicBufferUnlock fGraphicBufferUnlock;

  typedef void* (*pfnGraphicBufferGetNativeBuffer)(void*);
  pfnGraphicBufferGetNativeBuffer fGraphicBufferGetNativeBuffer;

  typedef int (*pfnGraphicBufferReallocate)(void*, uint32_t w, uint32_t h, uint32_t format);
  pfnGraphicBufferReallocate fGraphicBufferReallocate;

  bool EnsureInitialized()
  {
    if (mInitialized) {
      return true;
    }

    void *handle = dlopen(ANDROID_EGL_PATH, RTLD_LAZY);
    if (!handle) {
      LOG("Couldn't load EGL library");
      return false;
    }

    fGetDisplay = (pfnGetDisplay)dlsym(handle, "eglGetDisplay");
    fEGLGetError = (pfnEGLGetError)dlsym(handle, "eglGetError");
    fCreateImageKHR = (pfnCreateImageKHR)dlsym(handle, "eglCreateImageKHR");
    fDestroyImageKHR = (pfnDestroyImageKHR)dlsym(handle, "eglDestroyImageKHR");

    if (!fGetDisplay || !fEGLGetError || !fCreateImageKHR || !fDestroyImageKHR) {
      LOG("Failed to find some EGL functions");
      return false;
    }

    handle = dlopen(ANDROID_GLES_PATH, RTLD_LAZY);
    if (!handle) {
      LOG("Couldn't load GL library");
      return false;
    }

    fImageTargetTexture2DOES = (pfnImageTargetTexture2DOES)dlsym(handle, "glEGLImageTargetTexture2DOES");
    fBindTexture = (pfnBindTexture)dlsym(handle, "glBindTexture");
    fGLGetError = (pfnGLGetError)dlsym(handle, "glGetError");

    if (!fImageTargetTexture2DOES || !fBindTexture || !fGLGetError) {
      LOG("Failed to find some GL functions");
      return false;
    }

    handle = dlopen(ANDROID_LIBUI_PATH, RTLD_LAZY);
    if (!handle) {
      LOG("Couldn't load libui.so");
      return false;
    }

    fGraphicBufferCtor = (pfnGraphicBufferCtor)dlsym(handle, "_ZN7android13GraphicBufferC1Ejjij");
    fGraphicBufferDtor = (pfnGraphicBufferDtor)dlsym(handle, "_ZN7android13GraphicBufferD1Ev");
    fGraphicBufferLock = (pfnGraphicBufferLock)dlsym(handle, "_ZN7android13GraphicBuffer4lockEjPPv");
    fGraphicBufferLockRect = (pfnGraphicBufferLockRect)dlsym(handle, "_ZN7android13GraphicBuffer4lockEjRKNS_4RectEPPv");
    fGraphicBufferUnlock = (pfnGraphicBufferUnlock)dlsym(handle, "_ZN7android13GraphicBuffer6unlockEv");
    fGraphicBufferGetNativeBuffer = (pfnGraphicBufferGetNativeBuffer)dlsym(handle, "_ZNK7android13GraphicBuffer15getNativeBufferEv");
    fGraphicBufferReallocate = (pfnGraphicBufferReallocate)dlsym(handle, "_ZN7android13GraphicBuffer10reallocateEjjij");

    if (!fGraphicBufferCtor || !fGraphicBufferDtor || !fGraphicBufferLock ||
        !fGraphicBufferUnlock || !fGraphicBufferGetNativeBuffer) {
      LOG("Failed to lookup some GraphicBuffer functions");
      return false;
    }

    mInitialized = true;
    return true;
  }

private:
  bool mInitialized;

} sGLFunctions;

namespace mozilla {

static void clearGLError()
{
  while (glGetError() != GL_NO_ERROR);
}

static bool ensureNoGLError(const char* name)
{
  bool result = true;
  GLuint error;

  while ((error = glGetError()) != GL_NO_ERROR) {
    LOG("GL error [%s]: %40x\n", name, error);
    result = false;
  }

  return result;
}

AndroidGraphicBuffer::AndroidGraphicBuffer(uint32_t width, uint32_t height, uint32_t usage,
                                           gfxImageFormat format) :
    mWidth(width)
  , mHeight(height)
  , mUsage(usage)
  , mFormat(format)
  , mHandle(0)
  , mEGLImage(0)
{
}

AndroidGraphicBuffer::~AndroidGraphicBuffer()
{
  DestroyBuffer();
}

void
AndroidGraphicBuffer::DestroyBuffer()
{
  










#if 0
  if (mEGLImage) {
    if (sGLFunctions.EnsureInitialized()) {
      sGLFunctions.fDestroyImageKHR(sGLFunctions.fGetDisplay(EGL_DEFAULT_DISPLAY), mEGLImage);
      mEGLImage = nullptr;
    }
  }
#endif
  mEGLImage = nullptr;

  if (mHandle) {
    if (sGLFunctions.EnsureInitialized()) {
      sGLFunctions.fGraphicBufferDtor(mHandle);
    }
    free(mHandle);
    mHandle = nullptr;
  }

}

bool
AndroidGraphicBuffer::EnsureBufferCreated()
{
  if (!mHandle) {
    mHandle = malloc(GRAPHIC_BUFFER_SIZE);
    sGLFunctions.fGraphicBufferCtor(mHandle, mWidth, mHeight, GetAndroidFormat(mFormat), GetAndroidUsage(mUsage));
  }

  return true;
}

bool
AndroidGraphicBuffer::EnsureInitialized()
{
  if (!sGLFunctions.EnsureInitialized()) {
    return false;
  }

  EnsureBufferCreated();
  return true;
}

int
AndroidGraphicBuffer::Lock(uint32_t aUsage, unsigned char **bits)
{
  if (!EnsureInitialized())
    return true;

  return sGLFunctions.fGraphicBufferLock(mHandle, GetAndroidUsage(aUsage), bits);
}

int
AndroidGraphicBuffer::Lock(uint32_t aUsage, const nsIntRect& aRect, unsigned char **bits)
{
  if (!EnsureInitialized())
    return false;

  ARect rect;
  rect.left = aRect.x;
  rect.top = aRect.y;
  rect.right = aRect.x + aRect.width;
  rect.bottom = aRect.y + aRect.height;

  return sGLFunctions.fGraphicBufferLockRect(mHandle, GetAndroidUsage(aUsage), rect, bits);
}

int
AndroidGraphicBuffer::Unlock()
{
  if (!EnsureInitialized())
    return false;

  return sGLFunctions.fGraphicBufferUnlock(mHandle);
}

bool
AndroidGraphicBuffer::Reallocate(uint32_t aWidth, uint32_t aHeight, gfxImageFormat aFormat)
{
  if (!EnsureInitialized())
    return false;

  mWidth = aWidth;
  mHeight = aHeight;
  mFormat = aFormat;

  
  
  if (!gTryRealloc || sGLFunctions.fGraphicBufferReallocate(mHandle, aWidth, aHeight, GetAndroidFormat(aFormat)) != 0) {
    DestroyBuffer();
    EnsureBufferCreated();

    gTryRealloc = false;
  }

  return true;
}

uint32_t
AndroidGraphicBuffer::GetAndroidUsage(uint32_t aUsage)
{
  uint32_t flags = 0;

  if (aUsage & UsageSoftwareRead) {
    flags |= GRALLOC_USAGE_SW_READ_OFTEN;
  }

  if (aUsage & UsageSoftwareWrite) {
    flags |= GRALLOC_USAGE_SW_WRITE_OFTEN;
  }

  if (aUsage & UsageTexture) {
    flags |= GRALLOC_USAGE_HW_TEXTURE;
  }

  if (aUsage & UsageTarget) {
    flags |= GRALLOC_USAGE_HW_RENDER;
  }

  if (aUsage & Usage2D) {
    flags |= GRALLOC_USAGE_HW_2D;
  }

  return flags;
}

uint32_t
AndroidGraphicBuffer::GetAndroidFormat(gfxImageFormat aFormat)
{
  switch (aFormat) {
    case gfxImageFormat::RGB24:
      return HAL_PIXEL_FORMAT_RGBX_8888;
    case gfxImageFormat::RGB16_565:
      return HAL_PIXEL_FORMAT_RGB_565;
    default:
      return 0;
  }
}

bool
AndroidGraphicBuffer::EnsureEGLImage()
{
  if (mEGLImage)
    return true;


  if (!EnsureInitialized())
    return false;

  EGLint eglImgAttrs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE, EGL_NONE };
  void* nativeBuffer = sGLFunctions.fGraphicBufferGetNativeBuffer(mHandle);

  mEGLImage = sGLFunctions.fCreateImageKHR(sGLFunctions.fGetDisplay(EGL_DEFAULT_DISPLAY), EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, (EGLClientBuffer)nativeBuffer, eglImgAttrs);
  return mEGLImage != nullptr;
}

bool
AndroidGraphicBuffer::Bind()
{
  if (!EnsureInitialized())
    return false;

  if (!EnsureEGLImage()) {
    LOG("No valid EGLImage!");
    return false;
  }

  clearGLError();
  sGLFunctions.fImageTargetTexture2DOES(GL_TEXTURE_2D, mEGLImage);
  return ensureNoGLError("glEGLImageTargetTexture2DOES");
}


static void InitWhiteList(nsTArray<nsString>& list)
{
  nsString ele;
  ele.AssignASCII("droid2"); 
  list.AppendElement(ele);
  ele.AssignASCII("GT-I9100"); 
  list.AppendElement(ele);
  ele.AssignASCII("herring"); 
  list.AppendElement(ele);
  ele.AssignASCII("omap4sdp"); 
  list.AppendElement(ele);
  ele.AssignASCII("SGH-I897"); 
  list.AppendElement(ele);
  ele.AssignASCII("sgh-i997"); 
  list.AppendElement(ele);
  ele.AssignASCII("sgh-t839"); 
  list.AppendElement(ele);
  ele.AssignASCII("shadow"); 
  list.AppendElement(ele);
  ele.AssignASCII("spyder"); 
  list.AppendElement(ele);
  ele.AssignASCII("targa"); 
  list.AppendElement(ele);
  ele.AssignASCII("tuna"); 
  list.AppendElement(ele);
  ele.AssignASCII("venus2"); 
  list.AppendElement(ele);
}

bool
AndroidGraphicBuffer::IsBlacklisted()
{
  nsAutoString board;
  if (!AndroidBridge::Bridge()->GetStaticStringField("android/os/Build", "BOARD", board))
    return true;

  NS_ConvertUTF16toUTF8 boardUtf8(board);

  if (Preferences::GetBool("direct-texture.force.enabled", false)) {
    LOG("allowing board '%s' due to prefs override", boardUtf8.get());
    return false;
  }

  if (Preferences::GetBool("direct-texture.force.disabled", false)) {
    LOG("disallowing board '%s' due to prefs override", boardUtf8.get());
    return true;
  }

  static nsTArray<nsString> sListAllowed;
  if (sListAllowed.Length() == 0) {
    InitWhiteList(sListAllowed);
  }
  
  int i = -1;
  if ((i = sListAllowed.BinaryIndexOf(board)) >= 0) {
    nsString name = sListAllowed.ElementAt(i);
    LOG("allowing board '%s' based on '%s'\n", boardUtf8.get(), NS_ConvertUTF16toUTF8(name).get());
    return false;
  }

  LOG("disallowing board: %s\n", boardUtf8.get());
  return true;
}

} 
