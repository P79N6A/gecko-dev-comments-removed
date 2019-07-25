























#include <android/log.h>
#include "ANPBase.h"
#include "AndroidMediaLayer.h"
#include "nsIPluginInstanceOwner.h"
#include "nsPluginInstanceOwner.h"
#include "nsNPAPIPluginInstance.h"
#include "gfxRect.h"

#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "GeckoPlugins" , ## args)
#define ASSIGN(obj, name)   (obj)->name = anp_video_##name

using namespace mozilla;

static AndroidMediaLayer* GetLayerForInstance(NPP instance) {
  nsNPAPIPluginInstance* pinst = static_cast<nsNPAPIPluginInstance*>(instance->ndata);

  nsPluginInstanceOwner* owner;
  if (NS_FAILED(pinst->GetOwner((nsIPluginInstanceOwner**)&owner))) {
    return NULL;
  }

  return owner->Layer();
}

static void Invalidate(NPP instance) {
  nsNPAPIPluginInstance* pinst = static_cast<nsNPAPIPluginInstance*>(instance->ndata);

  nsPluginInstanceOwner* owner;
  if (NS_FAILED(pinst->GetOwner((nsIPluginInstanceOwner**)&owner)))
    return;

  owner->Invalidate();
}

static ANPNativeWindow anp_video_acquireNativeWindow(NPP instance) {
  AndroidMediaLayer* layer = GetLayerForInstance(instance);
  if (!layer)
    return NULL;

  return layer->RequestNativeWindowForVideo();
}

static void anp_video_setWindowDimensions(NPP instance, const ANPNativeWindow window,
        const ANPRectF* dimensions) {
  AndroidMediaLayer* layer = GetLayerForInstance(instance);
  if (!layer)
    return;

  gfxRect rect(dimensions->left, dimensions->top,
               dimensions->right - dimensions->left,
               dimensions->bottom - dimensions->top);

  layer->SetNativeWindowDimensions(window, rect);
  Invalidate(instance);
}


static void anp_video_releaseNativeWindow(NPP instance, ANPNativeWindow window) {
  AndroidMediaLayer* layer = GetLayerForInstance(instance);
  if (!layer)
    return;

  layer->ReleaseNativeWindowForVideo(window);
  Invalidate(instance);
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
