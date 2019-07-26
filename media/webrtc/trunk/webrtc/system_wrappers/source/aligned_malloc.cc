









#include "aligned_malloc.h"

#include <memory.h>
#include <stdlib.h>

#if _WIN32
#include <windows.h>
#else
#include <stdint.h>
#endif

#ifdef WEBRTC_GONK
#include <string.h>
#endif

#include "typedefs.h"



namespace webrtc {

uintptr_t GetRightAlign(uintptr_t start_pos, size_t alignment) {
  
  
  return (start_pos + alignment - 1) & ~(alignment - 1);
}


bool ValidAlignment(size_t alignment) {
  if (!alignment) {
    return false;
  }
  return (alignment & (alignment - 1)) == 0;
}

void* GetRightAlign(const void* pointer, size_t alignment) {
  if (!pointer) {
    return NULL;
  }
  if (!ValidAlignment(alignment)) {
    return NULL;
  }
  uintptr_t start_pos = reinterpret_cast<uintptr_t>(pointer);
  return reinterpret_cast<void*>(GetRightAlign(start_pos, alignment));
}

void* AlignedMalloc(size_t size, size_t alignment) {
  if (size == 0) {
    return NULL;
  }
  if (!ValidAlignment(alignment)) {
    return NULL;
  }

  
  
  
  
  void* memory_pointer = malloc(size + sizeof(uintptr_t) + alignment - 1);
  if (memory_pointer == NULL) {
    return NULL;
  }

  
  
  uintptr_t align_start_pos = reinterpret_cast<uintptr_t>(memory_pointer);
  align_start_pos += sizeof(uintptr_t);
  uintptr_t aligned_pos = GetRightAlign(align_start_pos, alignment);
  void* aligned_pointer = reinterpret_cast<void*>(aligned_pos);

  
  
  uintptr_t header_pos = aligned_pos - sizeof(uintptr_t);
  void* header_pointer = reinterpret_cast<void*>(header_pos);
  uintptr_t memory_start = reinterpret_cast<uintptr_t>(memory_pointer);
  memcpy(header_pointer, &memory_start, sizeof(uintptr_t));

  return aligned_pointer;
}

void AlignedFree(void* mem_block) {
  if (mem_block == NULL) {
    return;
  }
  uintptr_t aligned_pos = reinterpret_cast<uintptr_t>(mem_block);
  uintptr_t header_pos = aligned_pos - sizeof(uintptr_t);

  
  uintptr_t memory_start_pos = *reinterpret_cast<uintptr_t*>(header_pos);
  void* memory_start = reinterpret_cast<void*>(memory_start_pos);
  free(memory_start);
}

}  
