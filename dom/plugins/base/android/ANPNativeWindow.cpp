






































#include <android/log.h>
#include "AndroidBridge.h"
#include "AndroidMediaLayer.h"
#include "ANPBase.h"
#include "nsIPluginInstanceOwner.h"
#include "nsPluginInstanceOwner.h"
#include "nsNPAPIPluginInstance.h"
#include "gfxRect.h"

using namespace mozilla;
using namespace mozilla;

#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "GeckoPlugins" , ## args)
#define ASSIGN(obj, name)   (obj)->name = anp_native_window_##name

static nsresult GetOwner(NPP instance, nsPluginInstanceOwner** owner) {
  nsNPAPIPluginInstance* pinst = static_cast<nsNPAPIPluginInstance*>(instance->ndata);

  return pinst->GetOwner((nsIPluginInstanceOwner**)owner);
}

static ANPNativeWindow anp_native_window_acquireNativeWindow(NPP instance) {
  nsRefPtr<nsPluginInstanceOwner> owner;
  if (NS_FAILED(GetOwner(instance, getter_AddRefs(owner))))
    return NULL;

  ANPNativeWindow window = owner->Layer()->GetNativeWindowForContent();
  owner->Invalidate();

  return window;
}

static void anp_native_window_invertPluginContent(NPP instance, bool isContentInverted) {
  nsRefPtr<nsPluginInstanceOwner> owner;
  if (NS_FAILED(GetOwner(instance, getter_AddRefs(owner))))
    return;

  owner->Layer()->SetInverted(isContentInverted);
}


void InitNativeWindowInterface(ANPNativeWindowInterfaceV0* i) {
    ASSIGN(i, acquireNativeWindow);
    ASSIGN(i, invertPluginContent);
}
