









#ifndef WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_ANDROID_VIDEO_RENDER_ANDROID_IMPL_H_
#define WEBRTC_MODULES_VIDEO_RENDER_MAIN_SOURCE_ANDROID_VIDEO_RENDER_ANDROID_IMPL_H_

#include <jni.h>
#include "webrtc/modules/video_render/i_video_render.h"
#include "map_wrapper.h"


namespace webrtc {



class CriticalSectionWrapper;
class EventWrapper;
class ThreadWrapper;




class AndroidStream : public VideoRenderCallback {
 public:
  
  
  virtual void DeliverFrame(JNIEnv* jniEnv)=0;

  virtual ~AndroidStream() {};
};

class VideoRenderAndroid: IVideoRender {
 public:
  VideoRenderAndroid(const WebRtc_Word32 id,
                     const VideoRenderType videoRenderType,
                     void* window,
                     const bool fullscreen);

  virtual ~VideoRenderAndroid();

  virtual WebRtc_Word32 Init()=0;

  virtual WebRtc_Word32 ChangeUniqueId(const WebRtc_Word32 id);

  virtual WebRtc_Word32 ChangeWindow(void* window);

  virtual VideoRenderCallback* AddIncomingRenderStream(
      const WebRtc_UWord32 streamId,
      const WebRtc_UWord32 zOrder,
      const float left, const float top,
      const float right, const float bottom);

  virtual WebRtc_Word32 DeleteIncomingRenderStream(
      const WebRtc_UWord32 streamId);

  virtual WebRtc_Word32 GetIncomingRenderStreamProperties(
      const WebRtc_UWord32 streamId,
      WebRtc_UWord32& zOrder,
      float& left, float& top,
      float& right, float& bottom) const;

  virtual WebRtc_Word32 StartRender();

  virtual WebRtc_Word32 StopRender();

  virtual void ReDraw();

  

  virtual VideoRenderType RenderType();

  virtual RawVideoType PerferedVideoType();

  virtual bool FullScreen();

  virtual WebRtc_Word32 GetGraphicsMemory(
      WebRtc_UWord64& totalGraphicsMemory,
      WebRtc_UWord64& availableGraphicsMemory) const;

  virtual WebRtc_Word32 GetScreenResolution(
      WebRtc_UWord32& screenWidth,
      WebRtc_UWord32& screenHeight) const;

  virtual WebRtc_UWord32 RenderFrameRate(const WebRtc_UWord32 streamId);

  virtual WebRtc_Word32 SetStreamCropping(const WebRtc_UWord32 streamId,
                                          const float left, const float top,
                                          const float right,
                                          const float bottom);

  virtual WebRtc_Word32 SetTransparentBackground(const bool enable);

  virtual WebRtc_Word32 ConfigureRenderer(const WebRtc_UWord32 streamId,
                                          const unsigned int zOrder,
                                          const float left, const float top,
                                          const float right,
                                          const float bottom);

  virtual WebRtc_Word32 SetText(const WebRtc_UWord8 textId,
                                const WebRtc_UWord8* text,
                                const WebRtc_Word32 textLength,
                                const WebRtc_UWord32 textColorRef,
                                const WebRtc_UWord32 backgroundColorRef,
                                const float left, const float top,
                                const float rigth, const float bottom);

  virtual WebRtc_Word32 SetBitmap(const void* bitMap,
                                  const WebRtc_UWord8 pictureId,
                                  const void* colorKey, const float left,
                                  const float top, const float right,
                                  const float bottom);
  static JavaVM* g_jvm;

 protected:
  virtual AndroidStream* CreateAndroidRenderChannel(
      WebRtc_Word32 streamId,
      WebRtc_Word32 zOrder,
      const float left,
      const float top,
      const float right,
      const float bottom,
      VideoRenderAndroid& renderer) = 0;

  WebRtc_Word32 _id;
  CriticalSectionWrapper& _critSect;
  VideoRenderType _renderType;
  jobject _ptrWindow;

 private:
  static bool JavaRenderThreadFun(void* obj);
  bool JavaRenderThreadProcess();

  
  MapWrapper _streamsMap;
  
  bool _javaShutDownFlag;
  EventWrapper& _javaShutdownEvent;
  EventWrapper& _javaRenderEvent;
  WebRtc_Word64 _lastJavaRenderEvent;
  JNIEnv* _javaRenderJniEnv; 
  ThreadWrapper* _javaRenderThread;
};

} 

#endif  
