






































#include "npapi.h"

namespace mozilla {
namespace plugins {
namespace PluginUtilsOSX {


typedef void (*RemoteProcessEvents) (void*);

NPError ShowCocoaContextMenu(void* aMenu, int aX, int aY, void* pluginModule, RemoteProcessEvents remoteEvent);

void InvokeNativeEventLoop();

} 
} 
} 
