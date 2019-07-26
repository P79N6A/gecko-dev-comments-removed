









#include "webrtc/modules/desktop_capture/shared_desktop_frame.h"

#include "webrtc/system_wrappers/interface/atomic32.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class SharedDesktopFrame::Core {
 public:
  Core(DesktopFrame* frame) : frame_(frame) {}

  DesktopFrame* frame() { return frame_.get(); }

  bool HasOneRef() { return ref_count_.Value() == 1; }

  virtual int32_t AddRef() {
    return ++ref_count_;
  }

  virtual int32_t Release() {
    int32_t ref_count;
    ref_count = --ref_count_;
    if (ref_count == 0)
      delete this;
    return ref_count;
  }

 private:
  virtual ~Core() {}

  Atomic32 ref_count_;
  scoped_ptr<DesktopFrame> frame_;

  DISALLOW_COPY_AND_ASSIGN(Core);
};

SharedDesktopFrame::~SharedDesktopFrame() {}


SharedDesktopFrame* SharedDesktopFrame::Wrap(
    DesktopFrame* desktop_frame) {
  scoped_refptr<Core> core(new Core(desktop_frame));
  return new SharedDesktopFrame(core);
}

DesktopFrame* SharedDesktopFrame::GetUnderlyingFrame() {
  return core_->frame();
}

SharedDesktopFrame* SharedDesktopFrame::Share() {
  SharedDesktopFrame* result = new SharedDesktopFrame(core_);
  result->set_dpi(dpi());
  result->set_capture_time_ms(capture_time_ms());
  *result->mutable_updated_region() = updated_region();
  return result;
}

bool SharedDesktopFrame::IsShared() {
  return !core_->HasOneRef();
}

SharedDesktopFrame::SharedDesktopFrame(scoped_refptr<Core> core)
    : DesktopFrame(core->frame()->size(), core->frame()->stride(),
                   core->frame()->data(), core->frame()->shared_memory()),
      core_(core) {
}

}  
