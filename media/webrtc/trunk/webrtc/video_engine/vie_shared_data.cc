









#include "webrtc/modules/utility/interface/process_thread.h"
#include "webrtc/system_wrappers/interface/cpu_info.h"
#include "webrtc/system_wrappers/interface/trace.h"
#include "webrtc/video_engine/vie_channel_manager.h"
#include "webrtc/video_engine/vie_defines.h"
#include "webrtc/video_engine/vie_input_manager.h"
#include "webrtc/video_engine/vie_render_manager.h"
#include "webrtc/video_engine/vie_shared_data.h"

namespace webrtc {

ViESharedData::ViESharedData(const Config& config)
    : number_cores_(CpuInfo::DetectNumberOfCores()),
      channel_manager_(new ViEChannelManager(0, number_cores_, config)),
      input_manager_(new ViEInputManager(0, config)),
      render_manager_(new ViERenderManager(0)),
      module_process_thread_(ProcessThread::CreateProcessThread()),
      last_error_(0) {
  Trace::CreateTrace();
  channel_manager_->SetModuleProcessThread(module_process_thread_);
  input_manager_->SetModuleProcessThread(module_process_thread_);
  module_process_thread_->Start();
}

ViESharedData::~ViESharedData() {
  
  input_manager_.reset();
  channel_manager_.reset();
  render_manager_.reset();

  module_process_thread_->Stop();
  ProcessThread::DestroyProcessThread(module_process_thread_);
  Trace::ReturnTrace();
}

void ViESharedData::SetLastError(const int error) const {
  last_error_ = error;
}

int ViESharedData::LastErrorInternal() const {
  int error = last_error_;
  last_error_ = 0;
  return error;
}

int ViESharedData::NumberOfCores() const {
  return number_cores_;
}

}  
