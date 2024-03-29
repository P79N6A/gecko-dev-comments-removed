





#include "replace_malloc.h"
#include <errno.h>
#include "mozilla/CheckedInt.h"
#include "mozilla/Atomics.h"






static const malloc_table_t* gFuncs = nullptr;


static mozilla::Atomic<malloc_hook_table_t*> gHookTable(nullptr);

class GenericReplaceMallocBridge : public ReplaceMallocBridge
{
  virtual const malloc_table_t*
  RegisterHook(const char* aName, const malloc_table_t* aTable,
               const malloc_hook_table_t* aHookTable) override
  {
    
    if (!gFuncs) {
      return nullptr;
    }

    
    if (!aName) {
      return nullptr;
    }

    
    if (aTable) {
      return nullptr;
    }

    if (aHookTable) {
      
      if (!aHookTable->malloc_hook || !aHookTable->free_hook) {
        return nullptr;
      }
      gHookTable = const_cast<malloc_hook_table_t*>(aHookTable);
      return gFuncs;
    }
    gHookTable = nullptr;

    return nullptr;
  }
};

void
replace_init(const malloc_table_t* aTable)
{
  gFuncs = aTable;
}

ReplaceMallocBridge*
replace_get_bridge()
{
  static GenericReplaceMallocBridge bridge;
  return &bridge;
}

void*
replace_malloc(size_t aSize)
{
  void* ptr = gFuncs->malloc(aSize);
  const malloc_hook_table_t* hook_table = gHookTable;
  if (hook_table) {
    return hook_table->malloc_hook(ptr, aSize);
  }
  return ptr;
}

int
replace_posix_memalign(void** aPtr, size_t aAlignment, size_t aSize)
{
  int ret = gFuncs->posix_memalign(aPtr, aAlignment, aSize);
  const malloc_hook_table_t* hook_table = gHookTable;
  if (hook_table) {
    if (hook_table->posix_memalign_hook) {
      return hook_table->posix_memalign_hook(ret, aPtr, aAlignment, aSize);
    }
    void* ptr = hook_table->malloc_hook(*aPtr, aSize);
    if (!ptr && *aPtr) {
      *aPtr = ptr;
      ret = ENOMEM;
    }
  }
  return ret;
}

void*
replace_aligned_alloc(size_t aAlignment, size_t aSize)
{
  void* ptr = gFuncs->aligned_alloc(aAlignment, aSize);
  const malloc_hook_table_t* hook_table = gHookTable;
  if (hook_table) {
    if (hook_table->aligned_alloc_hook) {
      return hook_table->aligned_alloc_hook(ptr, aAlignment, aSize);
    }
    return hook_table->malloc_hook(ptr, aSize);
  }
  return ptr;
}

void*
replace_calloc(size_t aNum, size_t aSize)
{
  void* ptr = gFuncs->calloc(aNum, aSize);
  const malloc_hook_table_t* hook_table = gHookTable;
  if (hook_table) {
    if (hook_table->calloc_hook) {
      return hook_table->calloc_hook(ptr, aNum, aSize);
    }
    mozilla::CheckedInt<size_t> size = mozilla::CheckedInt<size_t>(aNum) * aSize;
    if (size.isValid()) {
      return hook_table->malloc_hook(ptr, size.value());
    }
    








    return hook_table->malloc_hook(ptr, SIZE_MAX);
  }
  return ptr;
}

void*
replace_realloc(void* aPtr, size_t aSize)
{
  const malloc_hook_table_t* hook_table = gHookTable;
  if (hook_table) {
    if (hook_table->realloc_hook_before) {
      hook_table->realloc_hook_before(aPtr);
    } else {
      hook_table->free_hook(aPtr);
    }
  }
  void* new_ptr = gFuncs->realloc(aPtr, aSize);
  



  if (hook_table) {
    if (hook_table->realloc_hook) {
      

      return hook_table->realloc_hook(new_ptr, aPtr, aSize);
    }
    return hook_table->malloc_hook(new_ptr, aSize);
  }
  return new_ptr;
}

void
replace_free(void* aPtr)
{
  const malloc_hook_table_t* hook_table = gHookTable;
  if (hook_table) {
    hook_table->free_hook(aPtr);
  }
  gFuncs->free(aPtr);
}

void*
replace_memalign(size_t aAlignment, size_t aSize)
{
  void* ptr = gFuncs->memalign(aAlignment, aSize);
  const malloc_hook_table_t* hook_table = gHookTable;
  if (hook_table) {
    if (hook_table->memalign_hook) {
      return hook_table->memalign_hook(ptr, aAlignment, aSize);
    }
    return hook_table->malloc_hook(ptr, aSize);
  }
  return ptr;
}

void*
replace_valloc(size_t aSize)
{
  void* ptr = gFuncs->valloc(aSize);
  const malloc_hook_table_t* hook_table = gHookTable;
  if (hook_table) {
    if (hook_table->valloc_hook) {
      return hook_table->valloc_hook(ptr, aSize);
    }
    return hook_table->malloc_hook(ptr, aSize);
  }
  return ptr;
}

size_t
replace_malloc_usable_size(usable_ptr_t aPtr)
{
  size_t ret = gFuncs->malloc_usable_size(aPtr);
  const malloc_hook_table_t* hook_table = gHookTable;
  if (hook_table && hook_table->malloc_usable_size_hook) {
    return hook_table->malloc_usable_size_hook(ret, aPtr);
  }
  return ret;
}

size_t
replace_malloc_good_size(size_t aSize)
{
  size_t ret = gFuncs->malloc_good_size(aSize);
  const malloc_hook_table_t* hook_table = gHookTable;
  if (hook_table && hook_table->malloc_good_size_hook) {
    return hook_table->malloc_good_size_hook(ret, aSize);
  }
  return ret;
}

void
replace_jemalloc_stats(jemalloc_stats_t* aStats)
{
  gFuncs->jemalloc_stats(aStats);
  const malloc_hook_table_t* hook_table = gHookTable;
  if (hook_table && hook_table->jemalloc_stats_hook) {
    hook_table->jemalloc_stats_hook(aStats);
  }
}

void
replace_jemalloc_purge_freed_pages(void)
{
  gFuncs->jemalloc_purge_freed_pages();
  const malloc_hook_table_t* hook_table = gHookTable;
  if (hook_table && hook_table->jemalloc_purge_freed_pages_hook) {
    hook_table->jemalloc_purge_freed_pages_hook();
  }
}

void
replace_jemalloc_free_dirty_pages(void)
{
  gFuncs->jemalloc_free_dirty_pages();
  const malloc_hook_table_t* hook_table = gHookTable;
  if (hook_table && hook_table->jemalloc_free_dirty_pages_hook) {
    hook_table->jemalloc_free_dirty_pages_hook();
  }
}
