






































#include "npapi.h"
#include "nsRect.h"

namespace mozilla {
namespace plugins {
namespace PluginUtilsOSX {


typedef void (*RemoteProcessEvents) (void*);

NPError ShowCocoaContextMenu(void* aMenu, int aX, int aY, void* pluginModule, RemoteProcessEvents remoteEvent);

void InvokeNativeEventLoop();


typedef void (*DrawPluginFunc) (CGContextRef, void*, nsIntRect aUpdateRect);

void* GetCGLayer(DrawPluginFunc aFunc, void* aPluginInstance);
void ReleaseCGLayer(void* cgLayer);
void Repaint(void* cgLayer, nsIntRect aRect);

bool SetProcessName(const char* aProcessName);

} 
} 
} 
