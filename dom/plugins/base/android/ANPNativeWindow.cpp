





#include <android/log.h>
#include "AndroidBridge.h"
#include "ANPBase.h"
#include "nsIPluginInstanceOwner.h"
#include "nsPluginInstanceOwner.h"
#include "nsNPAPIPluginInstance.h"
#include "gfxRect.h"

using namespace mozilla;
using namespace mozilla;

#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "GeckoPlugins" , ## args)
#define ASSIGN(obj, name)   (obj)->name = anp_native_window_##name

static ANPNativeWindow anp_native_window_acquireNativeWindow(NPP instance) {
  nsNPAPIPluginInstance* pinst = static_cast<nsNPAPIPluginInstance*>(instance->ndata);
  return pinst->AcquireContentWindow();
}

static void anp_native_window_invertPluginContent(NPP instance, bool isContentInverted) {
  nsNPAPIPluginInstance* pinst = static_cast<nsNPAPIPluginInstance*>(instance->ndata);
  pinst->SetInverted(isContentInverted);
  pinst->RedrawPlugin();
}


void InitNativeWindowInterface(ANPNativeWindowInterfaceV0* i) {
    ASSIGN(i, acquireNativeWindow);
    ASSIGN(i, invertPluginContent);
}
