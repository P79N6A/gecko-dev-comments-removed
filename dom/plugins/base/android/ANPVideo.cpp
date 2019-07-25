



#include <android/log.h>
#include "AndroidBridge.h"
#include "ANPBase.h"
#include "nsIPluginInstanceOwner.h"
#include "nsPluginInstanceOwner.h"
#include "nsNPAPIPluginInstance.h"
#include "gfxRect.h"

#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "GeckoPlugins" , ## args)
#define ASSIGN(obj, name)   (obj)->name = anp_video_##name

using namespace mozilla;

typedef nsNPAPIPluginInstance::VideoInfo VideoInfo;

static ANPNativeWindow anp_video_acquireNativeWindow(NPP instance) {
  nsNPAPIPluginInstance* pinst = static_cast<nsNPAPIPluginInstance*>(instance->ndata);

  return pinst->AcquireVideoWindow();
}

static void anp_video_setWindowDimensions(NPP instance, const ANPNativeWindow window,
                                          const ANPRectF* dimensions) {
  nsNPAPIPluginInstance* pinst = static_cast<nsNPAPIPluginInstance*>(instance->ndata);

  gfxRect rect(dimensions->left, dimensions->top,
               dimensions->right - dimensions->left,
               dimensions->bottom - dimensions->top);

  pinst->SetVideoDimensions(window, rect);
  pinst->RedrawPlugin();
}

static void anp_video_releaseNativeWindow(NPP instance, ANPNativeWindow window) {
  nsNPAPIPluginInstance* pinst = static_cast<nsNPAPIPluginInstance*>(instance->ndata);
  pinst->ReleaseVideoWindow(window);
  pinst->RedrawPlugin();
}

static void anp_video_setFramerateCallback(NPP instance, const ANPNativeWindow window, ANPVideoFrameCallbackProc callback) {
  
  NOT_IMPLEMENTED();
}



void InitVideoInterfaceV0(ANPVideoInterfaceV0* i) {
    ASSIGN(i, acquireNativeWindow);
    ASSIGN(i, setWindowDimensions);
    ASSIGN(i, releaseNativeWindow);
}

void InitVideoInterfaceV1(ANPVideoInterfaceV1* i) {
    ASSIGN(i, acquireNativeWindow);
    ASSIGN(i, setWindowDimensions);
    ASSIGN(i, releaseNativeWindow);
    ASSIGN(i, setFramerateCallback);
}
