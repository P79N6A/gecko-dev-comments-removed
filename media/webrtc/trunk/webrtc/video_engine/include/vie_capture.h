
















#ifndef WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_CAPTURE_H_
#define WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_CAPTURE_H_

#include "webrtc/common_types.h"
#include "webrtc/common_video/interface/i420_video_frame.h"

namespace webrtc {

class VideoEngine;
class VideoCaptureModule;



struct CaptureCapability {
  unsigned int width;
  unsigned int height;
  unsigned int maxFPS;
  RawVideoType rawType;
  VideoCodecType codecType;
  unsigned int expectedCaptureDelay;
  bool interlaced;
  CaptureCapability() {
    width = 0;
    height = 0;
    maxFPS = 0;
    rawType = kVideoI420;
    codecType = kVideoCodecUnknown;
    expectedCaptureDelay = 0;
    interlaced = false;
  }
};


enum Brightness {
  Normal = 0,
  Bright = 1,
  Dark = 2
};


enum CaptureAlarm {
  AlarmRaised = 0,
  AlarmCleared = 1
};

enum RotateCapturedFrame {
  RotateCapturedFrame_0 = 0,
  RotateCapturedFrame_90 = 90,
  RotateCapturedFrame_180 = 180,
  RotateCapturedFrame_270 = 270
};

struct ViEVideoFrameI420 {
  ViEVideoFrameI420() {
    y_plane = NULL;
    u_plane = NULL;
    v_plane = NULL;
    y_pitch = 0;
    u_pitch = 0;
    v_pitch = 0;
    width = 0;
    height = 0;
  }

  unsigned char* y_plane;
  unsigned char* u_plane;
  unsigned char* v_plane;

  int y_pitch;
  int u_pitch;
  int v_pitch;

  unsigned short width;
  unsigned short height;
};







class WEBRTC_DLLEXPORT ViEExternalCapture {
 public:
  ViEExternalCapture() {}
  virtual ~ViEExternalCapture() {}

  
  
  
  virtual int IncomingFrame(unsigned char* video_frame,
                            unsigned int video_frame_length,
                            unsigned short width,
                            unsigned short height,
                            RawVideoType video_type,
                            unsigned long long capture_time = 0) = 0;

  
  
  
  virtual int IncomingFrameI420(
      const ViEVideoFrameI420& video_frame,
      unsigned long long capture_time = 0) = 0;

  virtual void SwapFrame(I420VideoFrame* frame) {}
};





class WEBRTC_DLLEXPORT ViECaptureObserver {
 public:
  
  virtual void BrightnessAlarm(const int capture_id,
                               const Brightness brightness) = 0;

  
  virtual void CapturedFrameRate(const int capture_id,
                                 const unsigned char frame_rate) = 0;

  
  
  virtual void NoPictureAlarm(const int capture_id,
                              const CaptureAlarm alarm) = 0;

 protected:
  virtual ~ViECaptureObserver() {}
};

class WEBRTC_DLLEXPORT ViECapture {
 public:
  
  
  
  static ViECapture* GetInterface(VideoEngine* video_engine);

  
  
  
  
  virtual int Release() = 0;

  
  virtual int NumberOfCaptureDevices() = 0;

  
  virtual int GetCaptureDevice(unsigned int list_number,
                               char* device_nameUTF8,
                               const unsigned int device_nameUTF8Length,
                               char* unique_idUTF8,
                               const unsigned int unique_idUTF8Length) = 0;

  
  virtual int AllocateCaptureDevice(const char* unique_idUTF8,
                                    const unsigned int unique_idUTF8Length,
                                    int& capture_id) = 0;

  
  virtual int AllocateExternalCaptureDevice(
      int& capture_id,
      ViEExternalCapture *&external_capture) = 0;

  
  virtual int AllocateCaptureDevice(VideoCaptureModule& capture_module,
                                    int& capture_id) = 0;

  
  virtual int ReleaseCaptureDevice(const int capture_id) = 0;

  
  
  virtual int ConnectCaptureDevice(const int capture_id,
                                   const int video_channel) = 0;

  
  virtual int DisconnectCaptureDevice(const int video_channel) = 0;

  
  virtual int StartCapture(
      const int capture_id,
      const CaptureCapability& capture_capability = CaptureCapability()) = 0;

  
  virtual int StopCapture(const int capture_id) = 0;

  
  
  virtual int SetRotateCapturedFrames(const int capture_id,
                                      const RotateCapturedFrame rotation) = 0;

  
  
  virtual int SetCaptureDelay(const int capture_id,
                              const unsigned int capture_delay_ms) = 0;

  
  
  virtual int NumberOfCapabilities(
      const char* unique_id_utf8,
      const unsigned int unique_id_utf8_length) = 0;

  
  virtual int GetCaptureCapability(const char* unique_id_utf8,
                                   const unsigned int unique_id_utf8_length,
                                   const unsigned int capability_number,
                                   CaptureCapability& capability) = 0;

  
  
  virtual int ShowCaptureSettingsDialogBox(
      const char* unique_idUTF8,
      const unsigned int unique_id_utf8_length,
      const char* dialog_title,
      void* parent_window = NULL,
      const unsigned int x = 200,
      const unsigned int y = 200) = 0;

  
  
  
  virtual int GetOrientation(const char* unique_id_utf8,
                             RotateCapturedFrame& orientation) = 0;

  
  virtual int EnableBrightnessAlarm(const int capture_id,
                                    const bool enable) = 0;

  
  virtual int RegisterObserver(const int capture_id,
                               ViECaptureObserver& observer) = 0;

  
  virtual int DeregisterObserver(const int capture_id) = 0;

 protected:
  ViECapture() {}
  virtual ~ViECapture() {}
};

}  

#endif  
