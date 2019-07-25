






































#ifndef dom_plugins_PluginUtilsOSX_h
#define dom_plugins_PluginUtilsOSX_h 1

#include "npapi.h"
#include "nsRect.h"
#include "nsCoreAnimationSupport.h"

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









class THEBES_API nsDoubleBufferCARenderer {
public:
  nsDoubleBufferCARenderer() : mCALayer(nsnull) {}
  size_t GetFrontSurfaceWidth();
  size_t GetFrontSurfaceHeight();
  size_t GetBackSurfaceWidth();
  size_t GetBackSurfaceHeight();
  IOSurfaceID GetFrontSurfaceID();

  bool HasBackSurface();
  bool HasFrontSurface();
  bool HasCALayer();

  void SetCALayer(void *aCALayer);
  bool InitFrontSurface(size_t aWidth, size_t aHeight, AllowOfflineRendererEnum aAllowOfflineRenderer);
  void Render();
  void SwapSurfaces();
  void ClearFrontSurface();
  void ClearBackSurface();

private:
  void *mCALayer;
  nsRefPtr<nsCARenderer> mFrontRenderer;
  nsRefPtr<nsCARenderer> mBackRenderer;
  nsRefPtr<nsIOSurface> mFrontSurface;
  nsRefPtr<nsIOSurface> mBackSurface;
};

} 
} 
} 

#endif 
