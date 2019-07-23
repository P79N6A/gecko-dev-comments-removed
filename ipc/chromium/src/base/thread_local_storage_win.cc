



#include "base/thread_local_storage.h"

#include <windows.h>

#include "base/logging.h"










long ThreadLocalStorage::tls_key_ = TLS_OUT_OF_INDEXES;




long ThreadLocalStorage::tls_max_ = 1;




ThreadLocalStorage::TLSDestructorFunc
  ThreadLocalStorage::tls_destructors_[kThreadLocalStorageSize];

void** ThreadLocalStorage::Initialize() {
  if (tls_key_ == TLS_OUT_OF_INDEXES) {
    long value = TlsAlloc();
    DCHECK(value != TLS_OUT_OF_INDEXES);

    
    
    
    if (InterlockedCompareExchange(&tls_key_, value, TLS_OUT_OF_INDEXES) !=
            TLS_OUT_OF_INDEXES) {
      
      
      TlsFree(value);
    }
  }
  DCHECK(TlsGetValue(tls_key_) == NULL);

  
  void** tls_data = new void*[kThreadLocalStorageSize];
  memset(tls_data, 0, sizeof(void*[kThreadLocalStorageSize]));
  TlsSetValue(tls_key_, tls_data);
  return tls_data;
}

ThreadLocalStorage::Slot::Slot(TLSDestructorFunc destructor)
    : initialized_(false) {
  Initialize(destructor);
}

bool ThreadLocalStorage::Slot::Initialize(TLSDestructorFunc destructor) {
  if (tls_key_ == TLS_OUT_OF_INDEXES || !TlsGetValue(tls_key_))
    ThreadLocalStorage::Initialize();

  
  slot_ = InterlockedIncrement(&tls_max_) - 1;
  if (slot_ >= kThreadLocalStorageSize) {
    NOTREACHED();
    return false;
  }

  
  tls_destructors_[slot_] = destructor;
  initialized_ = true;
  return true;
}

void ThreadLocalStorage::Slot::Free() {
  
  
  tls_destructors_[slot_] = NULL;
  initialized_ = false;
}

void* ThreadLocalStorage::Slot::Get() const {
  void** tls_data = static_cast<void**>(TlsGetValue(tls_key_));
  if (!tls_data)
    tls_data = ThreadLocalStorage::Initialize();
  DCHECK(slot_ >= 0 && slot_ < kThreadLocalStorageSize);
  return tls_data[slot_];
}

void ThreadLocalStorage::Slot::Set(void* value) {
  void** tls_data = static_cast<void**>(TlsGetValue(tls_key_));
  if (!tls_data)
    tls_data = ThreadLocalStorage::Initialize();
  DCHECK(slot_ >= 0 && slot_ < kThreadLocalStorageSize);
  tls_data[slot_] = value;
}

void ThreadLocalStorage::ThreadExit() {
  if (tls_key_ == TLS_OUT_OF_INDEXES)
    return;

  void** tls_data = static_cast<void**>(TlsGetValue(tls_key_));

  
  if (!tls_data)
    return;

  for (int slot = 0; slot < tls_max_; slot++) {
    if (tls_destructors_[slot] != NULL) {
      void* value = tls_data[slot];
      tls_destructors_[slot](value);
    }
  }

  delete[] tls_data;

  
  TlsSetValue(tls_key_, NULL);
}








#ifdef _WIN64



#pragma comment(linker, "/INCLUDE:_tls_used")

#else  



#pragma comment(linker, "/INCLUDE:__tls_used")

#endif  


void NTAPI OnThreadExit(PVOID module, DWORD reason, PVOID reserved)
{
  
  
  if (DLL_THREAD_DETACH == reason || DLL_PROCESS_DETACH == reason)
    ThreadLocalStorage::ThreadExit();
}











#ifdef _WIN64


#pragma const_seg(".CRT$XLB")



extern const PIMAGE_TLS_CALLBACK p_thread_callback;
const PIMAGE_TLS_CALLBACK p_thread_callback = OnThreadExit;


#pragma const_seg()

#else  

#pragma data_seg(".CRT$XLB")
PIMAGE_TLS_CALLBACK p_thread_callback = OnThreadExit;


#pragma data_seg()

#endif  
