



#ifndef SANDBOX_SRC_SHARED_HANDLES_H__
#define SANDBOX_SRC_SHARED_HANDLES_H__

#include "base/basictypes.h"

#ifndef HANDLE




typedef void* HANDLE;
#endif

namespace sandbox {





































class SharedHandles {
 public:
  SharedHandles();

  
  
  
  
  bool Init(void* raw_mem, size_t size_bytes);

  
  
  
  
  
  
  
  bool SetHandle(uint32 tag, HANDLE handle);

  
  
  
  
  
  
  
  bool GetHandle(uint32 tag, HANDLE* handle);

 private:
  
  struct SharedItem {
    uint32 tag;
    void* item;
  };

  
  struct SharedMem {
    size_t max_items;
    SharedItem* items;
  };

  
  
  
  SharedItem* FindByTag(uint32 tag);

  SharedMem shared_;
  DISALLOW_COPY_AND_ASSIGN(SharedHandles);
};

}  

#endif  
