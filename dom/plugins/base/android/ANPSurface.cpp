





































#include "assert.h"
#include "ANPBase.h"
#include <android/log.h>
#include "AndroidBridge.h"
#include "gfxImageSurface.h"
#include "gfxContext.h"
#include "nsNPAPIPluginInstance.h"

#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "GeckoPlugins" , ## args)
#define ASSIGN(obj, name)   (obj)->name = anp_surface_##name



static struct ANPSurfaceInterfaceJavaGlue {
  bool        initialized;
  jclass geckoAppShellClass;
  jclass surfaceInfoCls;
  jmethodID getSurfaceInfo;
  jfieldID jFormat;
  jfieldID jWidth ;
  jfieldID jHeight;
} gSurfaceJavaGlue;

#define getClassGlobalRef(env, cname)                                    \
     (jClass = jclass(env->NewGlobalRef(env->FindClass(cname))))

static void init(JNIEnv* env) {
  if (gSurfaceJavaGlue.initialized)
    return;
  
  gSurfaceJavaGlue.geckoAppShellClass = mozilla::AndroidBridge::GetGeckoAppShellClass();
  
  jmethodID getClass = env->GetStaticMethodID(gSurfaceJavaGlue.geckoAppShellClass, 
                                              "getSurfaceInfoClass",
                                              "()Ljava/lang/Class;");

  gSurfaceJavaGlue.surfaceInfoCls = (jclass) env->NewGlobalRef(env->CallStaticObjectMethod(gSurfaceJavaGlue.geckoAppShellClass, getClass));

  gSurfaceJavaGlue.jFormat = env->GetFieldID(gSurfaceJavaGlue.surfaceInfoCls, "format", "I");
  gSurfaceJavaGlue.jWidth = env->GetFieldID(gSurfaceJavaGlue.surfaceInfoCls, "width", "I");
  gSurfaceJavaGlue.jHeight = env->GetFieldID(gSurfaceJavaGlue.surfaceInfoCls, "height", "I");

  gSurfaceJavaGlue.getSurfaceInfo = env->GetStaticMethodID(gSurfaceJavaGlue.geckoAppShellClass, "getSurfaceInfo", "(Landroid/view/SurfaceView;)Lorg/mozilla/gecko/SurfaceInfo;");
  gSurfaceJavaGlue.initialized = true;
}

static bool anp_lock(JNIEnv* env, jobject surfaceView, ANPBitmap* bitmap, ANPRectI* dirtyRect) {
  LOG("%s", __PRETTY_FUNCTION__);
  if (!bitmap || !surfaceView) {
    LOG("%s, null bitmap or surface, exiting", __PRETTY_FUNCTION__);
    return false;
  }

  init(env);

  jobject info = env->CallStaticObjectMethod(gSurfaceJavaGlue.geckoAppShellClass,
                                             gSurfaceJavaGlue.getSurfaceInfo, surfaceView);

  LOG("info: %p", info);
  if (!info)
    return false;

  bitmap->width  = env->GetIntField(info, gSurfaceJavaGlue.jWidth);
  bitmap->height = env->GetIntField(info, gSurfaceJavaGlue.jHeight);

  if (bitmap->width <= 0 || bitmap->height <= 0)
    return false;

  int format = env->GetIntField(info, gSurfaceJavaGlue.jFormat);
  gfxImageFormat targetFormat;

  
  if (format & 0x00000001) {
    
    
    LOG("Unable to handle 32bit pixel format");
    return false;
  } else if (format & 0x00000004) {
    bitmap->format = kRGB_565_ANPBitmapFormat;
    bitmap->rowBytes = bitmap->width * 2;
    targetFormat = gfxASurface::ImageFormatRGB16_565;
  } else {
    LOG("format from glue is unknown %d\n", format);
    return false;
  }

  nsNPAPIPluginInstance* pinst = nsNPAPIPluginInstance::FindByJavaSurface((void*)surfaceView);
  if (!pinst) {
    LOG("Failed to get plugin instance");
    return false;
  }

  NPRect lockRect;
  if (dirtyRect) {
    lockRect.top = dirtyRect->top;
    lockRect.left = dirtyRect->left;
    lockRect.right = dirtyRect->right;
    lockRect.bottom = dirtyRect->bottom;
  } else {
    
    lockRect.top = lockRect.left = 0;
    lockRect.right = bitmap->width;
    lockRect.bottom = bitmap->height;
  }
  
  gfxImageSurface* target = pinst->LockTargetSurface(bitmap->width, bitmap->height, targetFormat, &lockRect);
  bitmap->baseAddr = target->Data();

  env->DeleteLocalRef(info);

  return true;
}

static void anp_unlock(JNIEnv* env, jobject surfaceView) {
  LOG("%s", __PRETTY_FUNCTION__);

  if (!surfaceView) {
    LOG("null surface, exiting %s", __PRETTY_FUNCTION__);
    return;
  }

  nsNPAPIPluginInstance* pinst = nsNPAPIPluginInstance::FindByJavaSurface((void*)surfaceView);
  if (!pinst) {
    LOG("Could not find plugin instance!");
    return;
  }
  
  pinst->UnlockTargetSurface(true );
}



#define ASSIGN(obj, name)   (obj)->name = anp_##name

void InitSurfaceInterface(ANPSurfaceInterfaceV0 *i) {

  ASSIGN(i, lock);
  ASSIGN(i, unlock);

  
  gSurfaceJavaGlue.initialized = false;
}
