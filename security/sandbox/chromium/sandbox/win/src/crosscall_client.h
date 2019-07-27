



#ifndef SANDBOX_SRC_CROSSCALL_CLIENT_H_
#define SANDBOX_SRC_CROSSCALL_CLIENT_H_

#include "sandbox/win/src/crosscall_params.h"
#include "sandbox/win/src/sandbox.h"





























namespace sandbox {



const uint32 kIPCChannelSize = 1024;








template<typename T>
class CopyHelper {
 public:
  CopyHelper(const T& t) : t_(t) {}

  
  const void* GetStart() const {
    return &t_;
  }

  
  
  bool Update(void* buffer) {
    
    return true;
  }

  
  uint32 GetSize() const {
    return sizeof(T);
  }

  
  bool IsInOut() {
    return false;
  }

  
  ArgType GetType() {
    COMPILE_ASSERT(sizeof(T) == sizeof(uint32), need_specialization);
    return UINT32_TYPE;
  }

 private:
  const T& t_;
};



template<>
class CopyHelper<void*> {
 public:
  CopyHelper(void* t) : t_(t) {}

  
  const void* GetStart() const {
    return &t_;
  }

  
  
  bool Update(void* buffer) {
    
    return true;
  }

  
  uint32 GetSize() const {
    return sizeof(t_);
  }

  
  bool IsInOut() {
    return false;
  }

  
  ArgType GetType() {
    return VOIDPTR_TYPE;
  }

 private:
  const void* t_;
};



template<>
class CopyHelper<const wchar_t*> {
 public:
  CopyHelper(const wchar_t* t)
      : t_(t) {
  }

  
  const void* GetStart() const {
    return t_;
  }

  
  
  bool Update(void* buffer) {
    
    return true;
  }

  
  
  uint32 GetSize() const {
    __try {
      return (!t_) ? 0 : static_cast<uint32>(StringLength(t_) * sizeof(t_[0]));
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
      return kuint32max;
    }
  }

  
  bool IsInOut() {
    return false;
  }

  ArgType GetType() {
    return WCHAR_TYPE;
  }

 private:
  
  
  
  static size_t __cdecl StringLength(const wchar_t* wcs) {
    const wchar_t *eos = wcs;
    while (*eos++);
    return static_cast<size_t>(eos - wcs - 1);
  }

  const wchar_t* t_;
};



template<>
class CopyHelper<wchar_t*> : public CopyHelper<const wchar_t*> {
 public:
  typedef CopyHelper<const wchar_t*> Base;
  CopyHelper(wchar_t* t) : Base(t) {}

  const void* GetStart() const {
    return Base::GetStart();
  }

  bool Update(void* buffer) {
    return Base::Update(buffer);
  }

  uint32 GetSize() const {
    return Base::GetSize();
  }

  bool IsInOut() {
    return Base::IsInOut();
  }

  ArgType GetType() {
    return Base::GetType();
  }
};



template<size_t n>
class CopyHelper<const wchar_t[n]> : public CopyHelper<const wchar_t*> {
 public:
  typedef const wchar_t array[n];
  typedef CopyHelper<const wchar_t*> Base;
  CopyHelper(array t) : Base(t) {}

  const void* GetStart() const {
    return Base::GetStart();
  }

  bool Update(void* buffer) {
    return Base::Update(buffer);
  }

  uint32 GetSize() const {
    return Base::GetSize();
  }

  bool IsInOut() {
    return Base::IsInOut();
  }

  ArgType GetType() {
    return Base::GetType();
  }
};




class InOutCountedBuffer : public CountedBuffer {
 public:
  InOutCountedBuffer(void* buffer, uint32 size) : CountedBuffer(buffer, size) {}
};



template<>
class CopyHelper<InOutCountedBuffer> {
 public:
  CopyHelper(const InOutCountedBuffer t) : t_(t) {}

  
  const void* GetStart() const {
    return t_.Buffer();
  }

  
  bool Update(void* buffer) {
    
    
    __try {
      memcpy(t_.Buffer(), buffer, t_.Size());
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
      return false;
    }
    return true;
  }

  
  
  uint32 GetSize() const {
    return t_.Size();
  }

  
  bool IsInOut() {
    return true;
  }

  ArgType GetType() {
    return INOUTPTR_TYPE;
  }

 private:
  const InOutCountedBuffer t_;
};




#define XCALL_GEN_PARAMS_OBJ(num, params) \
  typedef ActualCallParams<num, kIPCChannelSize> ActualParams; \
  void* raw_mem = ipc_provider.GetBuffer(); \
  if (NULL == raw_mem) \
    return SBOX_ERROR_NO_SPACE; \
  ActualParams* params = new(raw_mem) ActualParams(tag);

#define XCALL_GEN_COPY_PARAM(num, params) \
  COMPILE_ASSERT(kMaxIpcParams >= num, too_many_parameters); \
  CopyHelper<Par##num> ch##num(p##num); \
  if (!params->CopyParamIn(num - 1, ch##num.GetStart(), ch##num.GetSize(), \
                           ch##num.IsInOut(), ch##num.GetType())) \
    return SBOX_ERROR_NO_SPACE;

#define XCALL_GEN_UPDATE_PARAM(num, params) \
  if (!ch##num.Update(params->GetParamPtr(num-1))) {\
    ipc_provider.FreeBuffer(raw_mem); \
    return SBOX_ERROR_BAD_PARAMS; \
  }

#define XCALL_GEN_FREE_CHANNEL() \
  ipc_provider.FreeBuffer(raw_mem);


template <typename IPCProvider, typename Par1>
ResultCode CrossCall(IPCProvider& ipc_provider, uint32 tag, const Par1& p1,
                     CrossCallReturn* answer) {
  XCALL_GEN_PARAMS_OBJ(1, call_params);
  XCALL_GEN_COPY_PARAM(1, call_params);

  ResultCode result = ipc_provider.DoCall(call_params, answer);

  if (SBOX_ERROR_CHANNEL_ERROR != result) {
    XCALL_GEN_UPDATE_PARAM(1, call_params);
    XCALL_GEN_FREE_CHANNEL();
  }

  return result;
}


template <typename IPCProvider, typename Par1, typename Par2>
ResultCode CrossCall(IPCProvider& ipc_provider, uint32 tag, const Par1& p1,
                     const Par2& p2, CrossCallReturn* answer) {
  XCALL_GEN_PARAMS_OBJ(2, call_params);
  XCALL_GEN_COPY_PARAM(1, call_params);
  XCALL_GEN_COPY_PARAM(2, call_params);

  ResultCode result = ipc_provider.DoCall(call_params, answer);

  if (SBOX_ERROR_CHANNEL_ERROR != result) {
    XCALL_GEN_UPDATE_PARAM(1, call_params);
    XCALL_GEN_UPDATE_PARAM(2, call_params);
    XCALL_GEN_FREE_CHANNEL();
  }
  return result;
}


template <typename IPCProvider, typename Par1, typename Par2, typename Par3>
ResultCode CrossCall(IPCProvider& ipc_provider, uint32 tag, const Par1& p1,
                     const Par2& p2, const Par3& p3, CrossCallReturn* answer) {
  XCALL_GEN_PARAMS_OBJ(3, call_params);
  XCALL_GEN_COPY_PARAM(1, call_params);
  XCALL_GEN_COPY_PARAM(2, call_params);
  XCALL_GEN_COPY_PARAM(3, call_params);

  ResultCode result = ipc_provider.DoCall(call_params, answer);

  if (SBOX_ERROR_CHANNEL_ERROR != result) {
    XCALL_GEN_UPDATE_PARAM(1, call_params);
    XCALL_GEN_UPDATE_PARAM(2, call_params);
    XCALL_GEN_UPDATE_PARAM(3, call_params);
    XCALL_GEN_FREE_CHANNEL();
  }
  return result;
}


template <typename IPCProvider, typename Par1, typename Par2, typename Par3,
          typename Par4>
ResultCode CrossCall(IPCProvider& ipc_provider, uint32 tag, const Par1& p1,
                     const Par2& p2, const Par3& p3, const Par4& p4,
                     CrossCallReturn* answer) {
  XCALL_GEN_PARAMS_OBJ(4, call_params);
  XCALL_GEN_COPY_PARAM(1, call_params);
  XCALL_GEN_COPY_PARAM(2, call_params);
  XCALL_GEN_COPY_PARAM(3, call_params);
  XCALL_GEN_COPY_PARAM(4, call_params);

  ResultCode result = ipc_provider.DoCall(call_params, answer);

  if (SBOX_ERROR_CHANNEL_ERROR != result) {
    XCALL_GEN_UPDATE_PARAM(1, call_params);
    XCALL_GEN_UPDATE_PARAM(2, call_params);
    XCALL_GEN_UPDATE_PARAM(3, call_params);
    XCALL_GEN_UPDATE_PARAM(4, call_params);
    XCALL_GEN_FREE_CHANNEL();
  }
  return result;
}


template <typename IPCProvider, typename Par1, typename Par2, typename Par3,
          typename Par4, typename Par5>
ResultCode CrossCall(IPCProvider& ipc_provider, uint32 tag, const Par1& p1,
                     const Par2& p2, const Par3& p3, const Par4& p4,
                     const Par5& p5, CrossCallReturn* answer) {
  XCALL_GEN_PARAMS_OBJ(5, call_params);
  XCALL_GEN_COPY_PARAM(1, call_params);
  XCALL_GEN_COPY_PARAM(2, call_params);
  XCALL_GEN_COPY_PARAM(3, call_params);
  XCALL_GEN_COPY_PARAM(4, call_params);
  XCALL_GEN_COPY_PARAM(5, call_params);

  ResultCode result = ipc_provider.DoCall(call_params, answer);

  if (SBOX_ERROR_CHANNEL_ERROR != result) {
    XCALL_GEN_UPDATE_PARAM(1, call_params);
    XCALL_GEN_UPDATE_PARAM(2, call_params);
    XCALL_GEN_UPDATE_PARAM(3, call_params);
    XCALL_GEN_UPDATE_PARAM(4, call_params);
    XCALL_GEN_UPDATE_PARAM(5, call_params);
    XCALL_GEN_FREE_CHANNEL();
  }
  return result;
}


template <typename IPCProvider, typename Par1, typename Par2, typename Par3,
          typename Par4, typename Par5, typename Par6>
ResultCode CrossCall(IPCProvider& ipc_provider, uint32 tag, const Par1& p1,
                     const Par2& p2, const Par3& p3, const Par4& p4,
                     const Par5& p5, const Par6& p6, CrossCallReturn* answer) {
  XCALL_GEN_PARAMS_OBJ(6, call_params);
  XCALL_GEN_COPY_PARAM(1, call_params);
  XCALL_GEN_COPY_PARAM(2, call_params);
  XCALL_GEN_COPY_PARAM(3, call_params);
  XCALL_GEN_COPY_PARAM(4, call_params);
  XCALL_GEN_COPY_PARAM(5, call_params);
  XCALL_GEN_COPY_PARAM(6, call_params);

  ResultCode result = ipc_provider.DoCall(call_params, answer);

  if (SBOX_ERROR_CHANNEL_ERROR != result) {
    XCALL_GEN_UPDATE_PARAM(1, call_params);
    XCALL_GEN_UPDATE_PARAM(2, call_params);
    XCALL_GEN_UPDATE_PARAM(3, call_params);
    XCALL_GEN_UPDATE_PARAM(4, call_params);
    XCALL_GEN_UPDATE_PARAM(5, call_params);
    XCALL_GEN_UPDATE_PARAM(6, call_params);
    XCALL_GEN_FREE_CHANNEL();
  }
  return result;
}


template <typename IPCProvider, typename Par1, typename Par2, typename Par3,
          typename Par4, typename Par5, typename Par6, typename Par7>
ResultCode CrossCall(IPCProvider& ipc_provider, uint32 tag, const Par1& p1,
                     const Par2& p2, const Par3& p3, const Par4& p4,
                     const Par5& p5, const Par6& p6, const Par7& p7,
                     CrossCallReturn* answer) {
  XCALL_GEN_PARAMS_OBJ(7, call_params);
  XCALL_GEN_COPY_PARAM(1, call_params);
  XCALL_GEN_COPY_PARAM(2, call_params);
  XCALL_GEN_COPY_PARAM(3, call_params);
  XCALL_GEN_COPY_PARAM(4, call_params);
  XCALL_GEN_COPY_PARAM(5, call_params);
  XCALL_GEN_COPY_PARAM(6, call_params);
  XCALL_GEN_COPY_PARAM(7, call_params);

  ResultCode result = ipc_provider.DoCall(call_params, answer);

  if (SBOX_ERROR_CHANNEL_ERROR != result) {
    XCALL_GEN_UPDATE_PARAM(1, call_params);
    XCALL_GEN_UPDATE_PARAM(2, call_params);
    XCALL_GEN_UPDATE_PARAM(3, call_params);
    XCALL_GEN_UPDATE_PARAM(4, call_params);
    XCALL_GEN_UPDATE_PARAM(5, call_params);
    XCALL_GEN_UPDATE_PARAM(6, call_params);
    XCALL_GEN_UPDATE_PARAM(7, call_params);
    XCALL_GEN_FREE_CHANNEL();
  }
  return result;
}
}  

#endif  
