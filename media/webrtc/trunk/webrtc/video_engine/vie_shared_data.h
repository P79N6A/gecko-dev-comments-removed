












#ifndef WEBRTC_VIDEO_ENGINE_VIE_SHARED_DATA_H_
#define WEBRTC_VIDEO_ENGINE_VIE_SHARED_DATA_H_

#include <map>

#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class Config;
class CpuOveruseObserver;
class ProcessThread;
class ViEChannelManager;
class ViEInputManager;
class ViERenderManager;

class ViESharedData {
 public:
  explicit ViESharedData(const Config& config);
  ~ViESharedData();

  void SetLastError(const int error) const;
  int LastErrorInternal() const;
  int NumberOfCores() const;

  
  int instance_id() { return 0;}
  ViEChannelManager* channel_manager() { return channel_manager_.get(); }
  ViEInputManager* input_manager() { return input_manager_.get(); }
  ViERenderManager* render_manager() { return render_manager_.get(); }

  std::map<int, CpuOveruseObserver*>* overuse_observers() {
    return &overuse_observers_; }

 private:
  const int number_cores_;

  scoped_ptr<ViEChannelManager> channel_manager_;
  scoped_ptr<ViEInputManager> input_manager_;
  scoped_ptr<ViERenderManager> render_manager_;
  ProcessThread* module_process_thread_;
  mutable int last_error_;

  std::map<int, CpuOveruseObserver*> overuse_observers_;
};

}  

#endif  
