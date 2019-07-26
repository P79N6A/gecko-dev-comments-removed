









#include "video_engine/vie_ref_count.h"

#include "system_wrappers/interface/critical_section_wrapper.h"

namespace webrtc {

ViERefCount::ViERefCount()
    : count_(0),
      crit_(CriticalSectionWrapper::CreateCriticalSection()) {
}

ViERefCount::~ViERefCount() {
}

ViERefCount& ViERefCount::operator++(int) {  
  CriticalSectionScoped lock(crit_.get());
  count_++;
  return *this;
}

ViERefCount& ViERefCount::operator--(int) {  
  CriticalSectionScoped lock(crit_.get());
  count_--;
  return *this;
}

void ViERefCount::Reset() {
  CriticalSectionScoped lock(crit_.get());
  count_ = 0;
}

int ViERefCount::GetCount() const {
  return count_;
}

}  
