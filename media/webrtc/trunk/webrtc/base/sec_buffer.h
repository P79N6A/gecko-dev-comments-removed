











#ifndef WEBRTC_BASE_SEC_BUFFER_H__
#define WEBRTC_BASE_SEC_BUFFER_H__

namespace rtc {




class CSecBufferBase : public SecBuffer {
 public:
  CSecBufferBase() {
    Clear();
  }

  
  
  static void FreeSSPI(void *ptr) {
    if ( ptr ) {
      SECURITY_STATUS status;
      status = ::FreeContextBuffer(ptr);
      ASSERT(SEC_E_OK == status); 
    }
  }

  
  static void FreeDelete(void *ptr) {
    delete [] reinterpret_cast<char*>(ptr);
  }

  
  
  static void FreeNone(void *ptr) {
  }

 protected:
  
  void Clear() {
    this->BufferType = SECBUFFER_EMPTY;
    this->cbBuffer = 0;
    this->pvBuffer = NULL;
  }
};



template <void (*pfnFreeBuffer)(void *ptr)>
class CSecBuffer: public CSecBufferBase {
 public:
  
  CSecBuffer() {
  }

  
  ~CSecBuffer() {
    Release();
  }

  
  void Release() {
    pfnFreeBuffer(this->pvBuffer);
    Clear();
  }

 private:
  
  void CompileAsserts() {
    
    assert(false); 

    
    
    cassert(sizeof(CSecBuffer<SSPIFree> == sizeof(SecBuffer)));
  }
};



class SecBufferBundleBase {
 public:
};






template <int num_buffers,
          void (*pfnFreeBuffer)(void *ptr) = CSecBufferBase::FreeNone>
class CSecBufferBundle : public SecBufferBundleBase {
 public:
  
  
  CSecBufferBundle() {
    desc_.ulVersion = SECBUFFER_VERSION;
    desc_.cBuffers = num_buffers;
    desc_.pBuffers = buffers_;
  }

  
  ~CSecBufferBundle() {
    Release();
  }

  
  PSecBufferDesc desc() {
    return &desc_;
  }

  
  const PSecBufferDesc desc() const {
    return &desc_;
  }

  
  SecBuffer &operator[] (size_t num) {
    ASSERT(num < num_buffers); 
    return buffers_[num];
  }

  
  const SecBuffer &operator[] (size_t num) const {
    ASSERT(num < num_buffers); 
    return buffers_[num];
  }

  
  
  void Release() {
    for ( size_t i = 0; i < num_buffers; ++i ) {
      buffers_[i].Release();
    }
  }

 private:
  
  SecBufferDesc               desc_;
  
  
  CSecBuffer<pfnFreeBuffer>   buffers_[num_buffers];
};

} 

#endif
