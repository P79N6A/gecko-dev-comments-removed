









#ifndef WEBRTC_MODULES_AUDIO_PROCESSING_PROCESSING_COMPONENT_H_
#define WEBRTC_MODULES_AUDIO_PROCESSING_PROCESSING_COMPONENT_H_

#include <vector>

#include "webrtc/common.h"

namespace webrtc {

class ProcessingComponent {
 public:
  ProcessingComponent();
  virtual ~ProcessingComponent();

  virtual int Initialize();
  virtual void SetExtraOptions(const Config& config) {}
  virtual int Destroy();

  bool is_component_enabled() const;

 protected:
  virtual int Configure();
  int EnableComponent(bool enable);
  void* handle(int index) const;
  int num_handles() const;

 private:
  virtual void* CreateHandle() const = 0;
  virtual int InitializeHandle(void* handle) const = 0;
  virtual int ConfigureHandle(void* handle) const = 0;
  virtual void DestroyHandle(void* handle) const = 0;
  virtual int num_handles_required() const = 0;
  virtual int GetHandleError(void* handle) const = 0;

  std::vector<void*> handles_;
  bool initialized_;
  bool enabled_;
  int num_handles_;
};

}  

#endif  
