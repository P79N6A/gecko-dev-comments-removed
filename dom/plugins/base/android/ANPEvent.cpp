





































#include "assert.h"
#include "ANPBase.h"
#include <android/log.h>
#include "nsThreadUtils.h"
#include "nsNPAPIPluginInstance.h"
#include "AndroidBridge.h"
#include "nsNPAPIPlugin.h"

#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "GeckoPlugins" , ## args)
#define ASSIGN(obj, name)   (obj)->name = anp_event_##name

void
anp_event_postEvent(NPP instance, const ANPEvent* event)
{
  LOG("%s", __PRETTY_FUNCTION__);

  nsNPAPIPluginInstance* pinst = static_cast<nsNPAPIPluginInstance*>(instance->ndata);
  pinst->PostEvent((void*) event);
  
  LOG("returning from %s", __PRETTY_FUNCTION__);
}


void InitEventInterface(ANPEventInterfaceV0 *i) {
  _assert(i->inSize == sizeof(*i));
  ASSIGN(i, postEvent);
}
