




































#include <android/log.h>
#include "AndroidMediaLayer.h"
#include "AndroidBridge.h"

#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "AndroidMediaLayer" , ## args)


namespace mozilla {

AndroidMediaLayer::AndroidMediaLayer()
  : mInverted(false) {
}

AndroidMediaLayer::~AndroidMediaLayer() {
  if (mContentData.window) {
    AndroidBridge::Bridge()->ReleaseNativeWindow(mContentData.window);
    mContentData.window = NULL;
  }

  if (mContentData.surface) {
    AndroidBridge::Bridge()->DestroySurface(mContentData.surface);
    mContentData.surface = NULL;
  }

  std::map<void*, SurfaceData*>::iterator it;

  for (it = mVideoSurfaces.begin(); it != mVideoSurfaces.end(); it++) {
    SurfaceData* data = it->second;

    AndroidBridge::Bridge()->ReleaseNativeWindow(data->window);
    AndroidBridge::Bridge()->DestroySurface(data->surface);
    delete data;
  }

  mVideoSurfaces.clear();
}

bool AndroidMediaLayer::EnsureContentSurface() {
  if (!mContentData.surface) {
    mContentData.surface = AndroidBridge::Bridge()->CreateSurface();
    if (mContentData.surface) {
      mContentData.window = AndroidBridge::Bridge()->AcquireNativeWindow(mContentData.surface);
      AndroidBridge::Bridge()->SetNativeWindowFormat(mContentData.window, 0, 0, AndroidBridge::WINDOW_FORMAT_RGBA_8888);
    }
  }

  return mContentData.surface && mContentData.window;
}

void* AndroidMediaLayer::GetNativeWindowForContent() {
  if (!EnsureContentSurface())
    return NULL;

  return mContentData.window;
}

void* AndroidMediaLayer::RequestNativeWindowForVideo() {
  jobject surface = AndroidBridge::Bridge()->CreateSurface();
  if (surface) {
    void* window = AndroidBridge::Bridge()->AcquireNativeWindow(surface);
    if (window) {
      AndroidBridge::Bridge()->SetNativeWindowFormat(window, 0, 0, AndroidBridge::WINDOW_FORMAT_RGBA_8888);
      mVideoSurfaces[window] = new SurfaceData(surface, window);
      return window;
    } else {
      LOG("Failed to create native window from surface");

      
      AndroidBridge::Bridge()->DestroySurface(surface);
    }
  }

  return NULL;
}

void AndroidMediaLayer::ReleaseNativeWindowForVideo(void* aWindow) {
  if (mVideoSurfaces.find(aWindow) == mVideoSurfaces.end())
    return;

  SurfaceData* data = mVideoSurfaces[aWindow];

  AndroidBridge::Bridge()->ReleaseNativeWindow(data->window);
  AndroidBridge::Bridge()->DestroySurface(data->surface);

  mVideoSurfaces.erase(aWindow);
  delete data;
}

void AndroidMediaLayer::SetNativeWindowDimensions(void* aWindow, const gfxRect& aDimensions) {
  if (mVideoSurfaces.find(aWindow) == mVideoSurfaces.end())
    return;

  SurfaceData* data = mVideoSurfaces[aWindow];
  data->dimensions = aDimensions;
}

void AndroidMediaLayer::UpdatePosition(const gfxRect& aRect, float aZoomLevel) {

  std::map<void*, SurfaceData*>::iterator it;

  if (EnsureContentSurface())
    AndroidBridge::Bridge()->ShowSurface(mContentData.surface, aRect, mInverted, true);

  for (it = mVideoSurfaces.begin(); it != mVideoSurfaces.end(); it++) {
    SurfaceData* data = it->second;

    
    
    gfxRect scaledDimensions = data->dimensions;
    scaledDimensions.Scale(aZoomLevel);

    gfxRect videoRect(aRect.x + scaledDimensions.x, aRect.y + scaledDimensions.y,
                      scaledDimensions.width, scaledDimensions.height);
    AndroidBridge::Bridge()->ShowSurface(data->surface, videoRect, mInverted, false);
  }
}

} 
