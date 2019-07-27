



#include "sandbox/win/src/shared_handles.h"

namespace sandbox {





SharedHandles::SharedHandles() {
  shared_.items = NULL;
  shared_.max_items = 0;
}

bool SharedHandles::Init(void* raw_mem, size_t size_bytes) {
  if (size_bytes < sizeof(shared_.items[0])) {
    
    return false;
  }
  shared_.items = static_cast<SharedItem*>(raw_mem);
  shared_.max_items = size_bytes / sizeof(shared_.items[0]);
  return true;
}



bool SharedHandles::SetHandle(uint32 tag, HANDLE handle) {
  if (0 == tag) {
    
    return false;
  }
  
  SharedItem* empty_slot = FindByTag(0);
  if (NULL == empty_slot) {
    return false;
  }
  empty_slot->tag = tag;
  empty_slot->item = handle;
  return true;
}

bool SharedHandles::GetHandle(uint32 tag, HANDLE* handle) {
  if (0 == tag) {
    
    return false;
  }
  SharedItem* found = FindByTag(tag);
  if (NULL == found) {
    return false;
  }
  *handle = found->item;
  return true;
}

SharedHandles::SharedItem* SharedHandles::FindByTag(uint32 tag) {
  for (size_t ix = 0; ix != shared_.max_items; ++ix) {
    if (tag == shared_.items[ix].tag) {
      return &shared_.items[ix];
    }
  }
  return NULL;
}

}  
