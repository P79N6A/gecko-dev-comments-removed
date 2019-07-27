




































#ifdef _WIN32
#include <windows.h>
#endif

#include <google/protobuf/stubs/once.h>

namespace google {
namespace protobuf {

#ifdef _WIN32

struct ProtobufOnceInternal {
  ProtobufOnceInternal() {
    InitializeCriticalSection(&critical_section);
  }
  ~ProtobufOnceInternal() {
    DeleteCriticalSection(&critical_section);
  }
  CRITICAL_SECTION critical_section;
};

ProtobufOnceType::~ProtobufOnceType()
{
  delete internal_;
  internal_ = NULL;
}

ProtobufOnceType::ProtobufOnceType() {
  
  if (internal_ == NULL) internal_ = new ProtobufOnceInternal;
}

void ProtobufOnceType::Init(void (*init_func)()) {
  
  
  
  
  if (internal_ == NULL) internal_ = new ProtobufOnceInternal;

  EnterCriticalSection(&internal_->critical_section);
  if (!initialized_) {
    init_func();
    initialized_ = true;
  }
  LeaveCriticalSection(&internal_->critical_section);
}

#endif

}  
}  
