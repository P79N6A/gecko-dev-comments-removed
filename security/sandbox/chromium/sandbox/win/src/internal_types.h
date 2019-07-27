



#ifndef SANDBOX_WIN_SRC_INTERNAL_TYPES_H_
#define SANDBOX_WIN_SRC_INTERNAL_TYPES_H_

namespace sandbox {

const wchar_t kNtdllName[] = L"ntdll.dll";
const wchar_t kKerneldllName[] = L"kernel32.dll";
const wchar_t kKernelBasedllName[] = L"kernelbase.dll";




enum ArgType {
  INVALID_TYPE = 0,
  WCHAR_TYPE,
  UINT32_TYPE,
  UNISTR_TYPE,
  VOIDPTR_TYPE,
  INPTR_TYPE,
  INOUTPTR_TYPE,
  LAST_TYPE
};


class CountedBuffer {
 public:
  CountedBuffer(void* buffer, uint32 size) : size_(size), buffer_(buffer) {}

  uint32 Size() const {
    return size_;
  }

  void* Buffer() const {
    return buffer_;
  }

 private:
  uint32 size_;
  void* buffer_;
};



class IPCInt {
 public:
  explicit IPCInt(void* buffer) {
    buffer_.vp = buffer;
  }

  explicit IPCInt(unsigned __int32 i32) {
    buffer_.vp = NULL;
    buffer_.i32 = i32;
  }

  unsigned __int32 As32Bit() const {
    return buffer_.i32;
  }

  void* AsVoidPtr() const {
    return buffer_.vp;
  }

 private:
  union U {
    void* vp;
    unsigned __int32 i32;
  } buffer_;
};

}  

#endif  
