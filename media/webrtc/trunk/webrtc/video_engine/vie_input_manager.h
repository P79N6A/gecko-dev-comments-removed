









#ifndef WEBRTC_VIDEO_ENGINE_VIE_INPUT_MANAGER_H_
#define WEBRTC_VIDEO_ENGINE_VIE_INPUT_MANAGER_H_

#include "webrtc/modules/video_capture/include/video_capture.h"
#include "system_wrappers/interface/map_wrapper.h"
#include "system_wrappers/interface/scoped_ptr.h"
#include "typedefs.h"  
#include "video_engine/include/vie_capture.h"
#include "video_engine/vie_defines.h"
#include "video_engine/vie_frame_provider_base.h"
#include "video_engine/vie_manager_base.h"

namespace webrtc {

class CriticalSectionWrapper;
class ProcessThread;
class RWLockWrapper;
class ViECapturer;
class ViEExternalCapture;
class ViEFilePlayer;
class VoiceEngine;

class ViEInputManager : private ViEManagerBase {
  friend class ViEInputManagerScoped;
 public:
  explicit ViEInputManager(int engine_id);
  ~ViEInputManager();

  void SetModuleProcessThread(ProcessThread* module_process_thread);

  
  int NumberOfCaptureDevices();

  
  int GetDeviceName(WebRtc_UWord32 device_number,
                    char* device_nameUTF8,
                    WebRtc_UWord32 device_name_length,
                    char* device_unique_idUTF8,
                    WebRtc_UWord32 device_unique_idUTF8Length);

  
  int NumberOfCaptureCapabilities(const char* device_unique_idUTF8);

  
  int GetCaptureCapability(const char* device_unique_idUTF8,
                           const WebRtc_UWord32 device_capability_number,
                           CaptureCapability& capability);

  
  int DisplayCaptureSettingsDialogBox(const char* device_unique_idUTF8,
                                      const char* dialog_titleUTF8,
                                      void* parent_window,
                                      WebRtc_UWord32 positionX,
                                      WebRtc_UWord32 positionY);
  int GetOrientation(const char* device_unique_idUTF8,
                     RotateCapturedFrame& orientation);

  
  
  
  int CreateCaptureDevice(const char* device_unique_idUTF8,
                          const WebRtc_UWord32 device_unique_idUTF8Length,
                          int& capture_id);
  int CreateCaptureDevice(VideoCaptureModule* capture_module,
                          int& capture_id);
  int CreateExternalCaptureDevice(ViEExternalCapture*& external_capture,
                                  int& capture_id);
  int DestroyCaptureDevice(int capture_id);

  int CreateFilePlayer(const char* file_nameUTF8, const bool loop,
                       const FileFormats file_format,
                       VoiceEngine* voe_ptr,
                       int& file_id);
  int DestroyFilePlayer(int file_id);

 private:
  
  bool GetFreeCaptureId(int* freecapture_id);

  
  void ReturnCaptureId(int capture_id);

  
  bool GetFreeFileId(int* free_file_id);

  
  void ReturnFileId(int file_id);

  
  ViEFrameProviderBase* ViEFrameProvider(
      const ViEFrameCallback* capture_observer) const;

  
  ViEFrameProviderBase* ViEFrameProvider(int provider_id) const;

  
  ViECapturer* ViECapturePtr(int capture_id) const;

  
  ViEFilePlayer* ViEFilePlayerPtr(int file_id) const;

  int engine_id_;
  scoped_ptr<CriticalSectionWrapper> map_cs_;
  MapWrapper vie_frame_provider_map_;

  
  VideoCaptureModule::DeviceInfo* capture_device_info_;
  int free_capture_device_id_[kViEMaxCaptureDevices];

  
  int free_file_id_[kViEMaxFilePlayers];

  ProcessThread* module_process_thread_;  
};


class ViEInputManagerScoped: private ViEManagerScopedBase {
 public:
  explicit ViEInputManagerScoped(const ViEInputManager& vie_input_manager);

  ViECapturer* Capture(int capture_id) const;
  ViEFilePlayer* FilePlayer(int file_id) const;
  ViEFrameProviderBase* FrameProvider(int provider_id) const;
  ViEFrameProviderBase* FrameProvider(const ViEFrameCallback*
                                      capture_observer) const;
};

}  

#endif  
