
































#include "protected_memory_allocator.h"
#include <assert.h>


ProtectedMemoryAllocator::ProtectedMemoryAllocator(vm_size_t pool_size) 
  : pool_size_(pool_size),
    next_alloc_offset_(0),
    valid_(false) {
  
  kern_return_t result = vm_allocate(mach_task_self(),
                                     &base_address_,
                                     pool_size,
                                     TRUE
                                     );
  
  valid_ = (result == KERN_SUCCESS);
  assert(valid_);
}


ProtectedMemoryAllocator::~ProtectedMemoryAllocator() {
  vm_deallocate(mach_task_self(),
                base_address_,
                pool_size_
                );
}


char *ProtectedMemoryAllocator::Allocate(vm_size_t bytes) {
  if (valid_ && next_alloc_offset_ + bytes <= pool_size_) {
    char *p = (char*)base_address_ + next_alloc_offset_;
    next_alloc_offset_ += bytes;
    return p;
  }
  
  return NULL;  
}


kern_return_t  ProtectedMemoryAllocator::Protect() {
  kern_return_t result = vm_protect(mach_task_self(),
                                    base_address_,
                                    pool_size_,
                                    FALSE,
                                    VM_PROT_READ);
  
  return result;
}


kern_return_t  ProtectedMemoryAllocator::Unprotect() {
  kern_return_t result = vm_protect(mach_task_self(),
                                    base_address_,
                                    pool_size_,
                                    FALSE,
                                    VM_PROT_READ | VM_PROT_WRITE);
  
  return result;
}
