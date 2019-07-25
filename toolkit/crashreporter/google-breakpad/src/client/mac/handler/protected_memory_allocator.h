







































#ifndef PROTECTED_MEMORY_ALLOCATOR_H__
#define PROTECTED_MEMORY_ALLOCATOR_H__

#include <mach/mach.h>


class ProtectedMemoryAllocator {
 public:
  ProtectedMemoryAllocator(vm_size_t pool_size);  
  ~ProtectedMemoryAllocator();
  
  
  
  
  
  char *         Allocate(vm_size_t n);
  
  
  char *         GetBaseAddress() { return (char*)base_address_; }

  
  
  vm_size_t      GetTotalSize() { return pool_size_; }

  
  vm_size_t      GetAllocatedSize() { return next_alloc_offset_; }

  
  vm_size_t      GetFreeSize() { return pool_size_ - next_alloc_offset_; }
  
  
  
  kern_return_t  Protect();  

  
  kern_return_t  Unprotect();  
  
 private:
  vm_size_t      pool_size_;
  vm_address_t   base_address_;
  vm_size_t      next_alloc_offset_;
  bool           valid_;
};

#endif 
