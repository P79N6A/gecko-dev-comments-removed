












#ifndef WEBRTC_VIDEO_ENGINE_VIE_SHARED_DATA_H_
#define WEBRTC_VIDEO_ENGINE_VIE_SHARED_DATA_H_

#include "video_engine/vie_defines.h"

namespace webrtc {

class ProcessThread;
class ViEChannelManager;
class ViEInputManager;
class ViERenderManager;

class ViESharedData {
 public:
  ViESharedData();
  ~ViESharedData();

  bool Initialized() const;
  int SetInitialized();
  int SetUnInitialized();
  void SetLastError(const int error) const;
  int LastErrorInternal() const;
  void SetOverUseDetectorOptions(const OverUseDetectorOptions& options);
  int NumberOfCores() const;

  int instance_id() { return instance_id_;}
  ViEChannelManager* channel_manager() { return &channel_manager_; }
  ViEInputManager* input_manager() { return &input_manager_; }
  ViERenderManager* render_manager() { return &render_manager_; }

 private:
  static int instance_counter_;
  const int instance_id_;
  bool initialized_;
  const int number_cores_;

  OverUseDetectorOptions over_use_detector_options_;
  ViEChannelManager& channel_manager_;
  ViEInputManager& input_manager_;
  ViERenderManager& render_manager_;
  ProcessThread* module_process_thread_;
  mutable int last_error_;
};

}  

#endif  
