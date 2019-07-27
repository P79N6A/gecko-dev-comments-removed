





#ifndef SANDBOX_SRC_SIDESTEP_PREAMBLE_PATCHER_H__
#define SANDBOX_SRC_SIDESTEP_PREAMBLE_PATCHER_H__

#include <stddef.h>

namespace sidestep {





const size_t kMaxPreambleStubSize = 32;


enum SideStepError {
  SIDESTEP_SUCCESS = 0,
  SIDESTEP_INVALID_PARAMETER,
  SIDESTEP_INSUFFICIENT_BUFFER,
  SIDESTEP_JUMP_INSTRUCTION,
  SIDESTEP_FUNCTION_TOO_SMALL,
  SIDESTEP_UNSUPPORTED_INSTRUCTION,
  SIDESTEP_NO_SUCH_MODULE,
  SIDESTEP_NO_SUCH_FUNCTION,
  SIDESTEP_ACCESS_DENIED,
  SIDESTEP_UNEXPECTED,
};


























class PreamblePatcher {
 public:
  
  
  
  template <class T>
  static SideStepError Patch(T target_function, T replacement_function,
                             void* preamble_stub, size_t stub_size) {
    return RawPatchWithStub(target_function, replacement_function,
                            reinterpret_cast<unsigned char*>(preamble_stub),
                            stub_size, NULL);
  }

 private:

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static SideStepError RawPatchWithStub(void* target_function,
                                        void *replacement_function,
                                        unsigned char* preamble_stub,
                                        size_t stub_size,
                                        size_t* bytes_needed);
};

};  

#endif  
