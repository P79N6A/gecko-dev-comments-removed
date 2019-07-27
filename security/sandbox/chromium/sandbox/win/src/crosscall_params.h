



#ifndef SANDBOX_SRC_CROSSCALL_PARAMS_H__
#define SANDBOX_SRC_CROSSCALL_PARAMS_H__

#include <windows.h>
#include <lmaccess.h>

#include <memory>

#include "base/basictypes.h"
#include "sandbox/win/src/internal_types.h"
#include "sandbox/win/src/sandbox_types.h"

namespace {



uint32 Align(uint32 value) {
  uint32 alignment = sizeof(int64);
  return ((value + alignment - 1) / alignment) * alignment;
}

}


















namespace sandbox {


const size_t kExtendedReturnCount = 8;



union MultiType {
  uint32 unsigned_int;
  void* pointer;
  HANDLE handle;
  ULONG_PTR ulong_ptr;
};






const int kMaxIpcParams = 9;


struct ParamInfo {
  ArgType type_;
  uint32 offset_;
  uint32 size_;
};





struct CrossCallReturn {
  
  uint32 tag;
  
  ResultCode call_outcome;
  
  
  union {
    NTSTATUS nt_status;
    DWORD    win32_result;
  };
  
  uint32 extended_count;
  
  HANDLE handle;
  
  MultiType extended[kExtendedReturnCount];
};












class CrossCallParams {
 public:
  
  uint32 GetTag() const {
    return tag_;
  }

  
  
  const void* GetBuffer() const {
    return this;
  }

  
  const uint32 GetParamsCount() const {
    return params_count_;
  }

  
  CrossCallReturn* GetCallReturn() {
    return &call_return;
  }

  
  const bool IsInOut() const {
    return (1 == is_in_out_);
  }

  
  void SetIsInOut(bool value) {
    if (value)
      is_in_out_ = 1;
    else
      is_in_out_ = 0;
  }

 protected:
  
  CrossCallParams(uint32 tag, uint32 params_count)
      : tag_(tag),
        params_count_(params_count),
        is_in_out_(0) {
  }

 private:
  uint32 tag_;
  uint32 is_in_out_;
  CrossCallReturn call_return;
  const uint32 params_count_;
  DISALLOW_COPY_AND_ASSIGN(CrossCallParams);
};







































template <size_t NUMBER_PARAMS, size_t BLOCK_SIZE>
class ActualCallParams : public CrossCallParams {
 public:
  
  explicit ActualCallParams(uint32 tag)
      : CrossCallParams(tag, NUMBER_PARAMS) {
    param_info_[0].offset_ =
        static_cast<uint32>(parameters_ - reinterpret_cast<char*>(this));
  }

  
  
  ActualCallParams(uint32 tag, uint32 number_params)
      : CrossCallParams(tag, number_params) {
    param_info_[0].offset_ =
        static_cast<uint32>(parameters_ - reinterpret_cast<char*>(this));
  }

  
  
  uint32 OverrideSize(uint32 new_size) {
    uint32 previous_size = param_info_[NUMBER_PARAMS].offset_;
    param_info_[NUMBER_PARAMS].offset_ = new_size;
    return previous_size;
  }

  
  
  bool CopyParamIn(uint32 index, const void* parameter_address, uint32 size,
                   bool is_in_out, ArgType type) {
    if (index >= NUMBER_PARAMS) {
      return false;
    }

    if (kuint32max == size) {
      
      return false;
    }

    if (size && !parameter_address) {
      return false;
    }

    if ((size > sizeof(*this)) ||
        (param_info_[index].offset_ > (sizeof(*this) - size))) {
      
      return false;
    }

    char* dest = reinterpret_cast<char*>(this) +  param_info_[index].offset_;

    
    
    __try {
      memcpy(dest, parameter_address, size);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
      return false;
    }

    
    
    if (is_in_out)
      SetIsInOut(true);

    param_info_[index + 1].offset_ = Align(param_info_[index].offset_ +
                                                size);
    param_info_[index].size_ = size;
    param_info_[index].type_ = type;
    return true;
  }

  
  void* GetParamPtr(size_t index) {
    return reinterpret_cast<char*>(this) + param_info_[index].offset_;
  }

  
  
  uint32 GetSize() const {
    return param_info_[NUMBER_PARAMS].offset_;
  }

 protected:
  ActualCallParams() : CrossCallParams(0, NUMBER_PARAMS) { }

 private:
  ParamInfo param_info_[NUMBER_PARAMS + 1];
  char parameters_[BLOCK_SIZE - sizeof(CrossCallParams)
                   - sizeof(ParamInfo) * (NUMBER_PARAMS + 1)];
  DISALLOW_COPY_AND_ASSIGN(ActualCallParams);
};

COMPILE_ASSERT(sizeof(ActualCallParams<1, 1024>) == 1024, bad_size_buffer);
COMPILE_ASSERT(sizeof(ActualCallParams<2, 1024>) == 1024, bad_size_buffer);
COMPILE_ASSERT(sizeof(ActualCallParams<3, 1024>) == 1024, bad_size_buffer);

}  

#endif  
