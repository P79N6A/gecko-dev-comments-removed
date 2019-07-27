





#ifndef dom_plugins_PluginUtilsOSX_h
#define dom_plugins_PluginUtilsOSX_h 1

#include "npapi.h"
#include "mozilla/gfx/QuartzSupport.h"
#include "nsRect.h"

namespace mozilla {
namespace plugins {
namespace PluginUtilsOSX {


typedef void (*RemoteProcessEvents) (void*);

NPError ShowCocoaContextMenu(void* aMenu, int aX, int aY, void* pluginModule, RemoteProcessEvents remoteEvent);

void InvokeNativeEventLoop();


typedef void (*DrawPluginFunc) (CGContextRef, void*, nsIntRect aUpdateRect);

void* GetCGLayer(DrawPluginFunc aFunc, void* aPluginInstance,
                 bool aAvoidCGCrashes, double aContentsScaleFactor);
void ReleaseCGLayer(void* cgLayer);
void Repaint(void* cgLayer, nsIntRect aRect);

bool SetProcessName(const char* aProcessName);









class nsDoubleBufferCARenderer {
public:
  nsDoubleBufferCARenderer() : mCALayer(nullptr), mContentsScaleFactor(1.0) {}
  
  
  
  
  size_t GetFrontSurfaceWidth();
  
  
  size_t GetFrontSurfaceHeight();
  double GetFrontSurfaceContentsScaleFactor();
  
  
  size_t GetBackSurfaceWidth();
  
  
  size_t GetBackSurfaceHeight();
  double GetBackSurfaceContentsScaleFactor();
  IOSurfaceID GetFrontSurfaceID();

  bool HasBackSurface();
  bool HasFrontSurface();
  bool HasCALayer();

  void SetCALayer(void *aCALayer);
  
  
  bool InitFrontSurface(size_t aWidth, size_t aHeight,
                        double aContentsScaleFactor,
                        AllowOfflineRendererEnum aAllowOfflineRenderer);
  void Render();
  void SwapSurfaces();
  void ClearFrontSurface();
  void ClearBackSurface();

  double GetContentsScaleFactor() { return mContentsScaleFactor; }

private:
  void *mCALayer;
  RefPtr<nsCARenderer> mCARenderer;
  RefPtr<MacIOSurface> mFrontSurface;
  RefPtr<MacIOSurface> mBackSurface;
  double mContentsScaleFactor;
};

} 
} 
} 

#endif 
