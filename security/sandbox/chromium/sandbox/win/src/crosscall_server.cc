



#include <string>
#include <vector>

#include "sandbox/win/src/crosscall_server.h"
#include "sandbox/win/src/crosscall_params.h"
#include "sandbox/win/src/crosscall_client.h"
#include "base/logging.h"





namespace {


const size_t kMaxBufferSize = sandbox::kIPCChannelSize;

}

namespace sandbox {



uint32 GetActualBufferSize(uint32 param_count, void* buffer_base) {
  
  typedef ActualCallParams<1, kMaxBufferSize> ActualCP1;
  typedef ActualCallParams<2, kMaxBufferSize> ActualCP2;
  typedef ActualCallParams<3, kMaxBufferSize> ActualCP3;
  typedef ActualCallParams<4, kMaxBufferSize> ActualCP4;
  typedef ActualCallParams<5, kMaxBufferSize> ActualCP5;
  typedef ActualCallParams<6, kMaxBufferSize> ActualCP6;
  typedef ActualCallParams<7, kMaxBufferSize> ActualCP7;
  typedef ActualCallParams<8, kMaxBufferSize> ActualCP8;
  typedef ActualCallParams<9, kMaxBufferSize> ActualCP9;

  
  switch (param_count) {
    case 0:
      return 0;
    case 1:
      return reinterpret_cast<ActualCP1*>(buffer_base)->GetSize();
    case 2:
      return reinterpret_cast<ActualCP2*>(buffer_base)->GetSize();
    case 3:
      return reinterpret_cast<ActualCP3*>(buffer_base)->GetSize();
    case 4:
      return reinterpret_cast<ActualCP4*>(buffer_base)->GetSize();
    case 5:
      return reinterpret_cast<ActualCP5*>(buffer_base)->GetSize();
    case 6:
      return reinterpret_cast<ActualCP6*>(buffer_base)->GetSize();
    case 7:
      return reinterpret_cast<ActualCP7*>(buffer_base)->GetSize();
    case 8:
      return reinterpret_cast<ActualCP8*>(buffer_base)->GetSize();
    case 9:
      return reinterpret_cast<ActualCP9*>(buffer_base)->GetSize();
    default:
      return 0;
  }
}


bool IsSizeWithinRange(uint32 buffer_size, uint32 min_declared_size,
                       uint32 declared_size) {
  if ((buffer_size < min_declared_size) ||
      (sizeof(CrossCallParamsEx) > min_declared_size)) {
    
    
    return false;
  }

  if ((declared_size > buffer_size) || (declared_size < min_declared_size)) {
    
    
    return false;
  }

  return true;
}

CrossCallParamsEx::CrossCallParamsEx()
  :CrossCallParams(0, 0) {
}





void CrossCallParamsEx::operator delete(void* raw_memory) throw() {
  if (NULL == raw_memory) {
    
    return;
  }
  delete[] reinterpret_cast<char*>(raw_memory);
}




CrossCallParamsEx* CrossCallParamsEx::CreateFromBuffer(void* buffer_base,
                                                       uint32 buffer_size,
                                                       uint32* output_size) {
  
  
  if (NULL == buffer_base) {
    return NULL;
  }
  if (buffer_size < sizeof(CrossCallParams)) {
    return NULL;
  }
  if (buffer_size > kMaxBufferSize) {
    return NULL;
  }

  char* backing_mem = NULL;
  uint32 param_count = 0;
  uint32 declared_size;
  uint32 min_declared_size;
  CrossCallParamsEx* copied_params = NULL;

  
  
  __try {
    CrossCallParams* call_params =
        reinterpret_cast<CrossCallParams*>(buffer_base);

    
    
    param_count = call_params->GetParamsCount();
    min_declared_size = sizeof(CrossCallParams) +
                        ((param_count + 1) * sizeof(ParamInfo));

    
    declared_size = GetActualBufferSize(param_count, buffer_base);

    if (!IsSizeWithinRange(buffer_size, min_declared_size, declared_size))
      return NULL;

    
    *output_size = declared_size;
    backing_mem = new char[declared_size];
    copied_params = reinterpret_cast<CrossCallParamsEx*>(backing_mem);
    memcpy(backing_mem, call_params, declared_size);

    
    
    
    _ReadWriteBarrier();

    min_declared_size = sizeof(CrossCallParams) +
                        ((param_count + 1) * sizeof(ParamInfo));

    
    if (copied_params->GetParamsCount() != param_count ||
        GetActualBufferSize(param_count, backing_mem) != declared_size ||
        !IsSizeWithinRange(buffer_size, min_declared_size, declared_size)) {
      delete [] backing_mem;
      return NULL;
    }

  } __except(EXCEPTION_EXECUTE_HANDLER) {
    
    
    delete [] backing_mem;
    return NULL;
  }

  const char* last_byte = &backing_mem[declared_size];
  const char* first_byte = &backing_mem[min_declared_size];

  
  
  for (uint32 ix =0; ix != param_count; ++ix) {
    uint32 size = 0;
    ArgType type;
    char* address = reinterpret_cast<char*>(
                        copied_params->GetRawParameter(ix, &size, &type));
    if ((NULL == address) ||               
        (INVALID_TYPE >= type) || (LAST_TYPE <= type) ||  
        (address < backing_mem) ||         
        (address < first_byte) ||          
        (address > last_byte) ||           
        ((address + size) < address) ||    
        ((address + size) > last_byte)) {  
      
      delete[] backing_mem;
      return NULL;
    }
  }
  
  return copied_params;
}


void* CrossCallParamsEx::GetRawParameter(uint32 index, uint32* size,
                                         ArgType* type) {
  if (index >= GetParamsCount()) {
    return NULL;
  }
  
  
  *size = param_info_[index].size_;
  *type = param_info_[index].type_;

  return param_info_[index].offset_ + reinterpret_cast<char*>(this);
}


bool CrossCallParamsEx::GetParameter32(uint32 index, uint32* param) {
  uint32 size = 0;
  ArgType type;
  void* start = GetRawParameter(index, &size, &type);
  if ((NULL == start) || (4 != size) || (UINT32_TYPE != type)) {
    return false;
  }
  
  *(reinterpret_cast<uint32*>(param)) = *(reinterpret_cast<uint32*>(start));
  return true;
}

bool CrossCallParamsEx::GetParameterVoidPtr(uint32 index, void** param) {
  uint32 size = 0;
  ArgType type;
  void* start = GetRawParameter(index, &size, &type);
  if ((NULL == start) || (sizeof(void*) != size) || (VOIDPTR_TYPE != type)) {
    return false;
  }
  *param = *(reinterpret_cast<void**>(start));
  return true;
}



bool CrossCallParamsEx::GetParameterStr(uint32 index, base::string16* string) {
  uint32 size = 0;
  ArgType type;
  void* start = GetRawParameter(index, &size, &type);
  if (WCHAR_TYPE != type) {
    return false;
  }

  
  if (size == 0) {
    *string = L"";
    return true;
  }

  if ((NULL == start) || ((size % sizeof(wchar_t)) != 0)) {
    return false;
  }
  string->append(reinterpret_cast<wchar_t*>(start), size/(sizeof(wchar_t)));
  return true;
}

bool CrossCallParamsEx::GetParameterPtr(uint32 index, uint32 expected_size,
                                        void** pointer) {
  uint32 size = 0;
  ArgType type;
  void* start = GetRawParameter(index, &size, &type);

  if ((size != expected_size) || (INOUTPTR_TYPE != type)) {
    return false;
  }

  if (NULL == start) {
    return false;
  }

  *pointer = start;
  return true;
}

void SetCallError(ResultCode error, CrossCallReturn* call_return) {
  call_return->call_outcome = error;
  call_return->extended_count = 0;
}

void SetCallSuccess(CrossCallReturn* call_return) {
  call_return->call_outcome = SBOX_ALL_OK;
}

Dispatcher* Dispatcher::OnMessageReady(IPCParams* ipc,
                                      CallbackGeneric* callback) {
  DCHECK(callback);
  std::vector<IPCCall>::iterator it = ipc_calls_.begin();
  for (; it != ipc_calls_.end(); ++it) {
    if (it->params.Matches(ipc)) {
      *callback = it->callback;
      return this;
    }
  }
  return NULL;
}

}  
