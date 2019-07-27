










#define __VPX_MEM_C__

#include "vpx_mem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/vpx_mem_intrnl.h"
#include "vpx/vpx_integer.h"

void *vpx_memalign(size_t align, size_t size) {
  void *addr,
       * x = NULL;

  addr = malloc(size + align - 1 + ADDRESS_STORAGE_SIZE);

  if (addr) {
    x = align_addr((unsigned char *)addr + ADDRESS_STORAGE_SIZE, (int)align);
    
    ((size_t *)x)[-1] = (size_t)addr;
  }

  return x;
}

void *vpx_malloc(size_t size) {
  return vpx_memalign(DEFAULT_ALIGNMENT, size);
}

void *vpx_calloc(size_t num, size_t size) {
  void *x;

  x = vpx_memalign(DEFAULT_ALIGNMENT, num * size);

  if (x)
    memset(x, 0, num * size);

  return x;
}

void *vpx_realloc(void *memblk, size_t size) {
  void *addr,
       * new_addr = NULL;
  int align = DEFAULT_ALIGNMENT;

  







  if (!memblk)
    new_addr = vpx_malloc(size);
  else if (!size)
    vpx_free(memblk);
  else {
    addr   = (void *)(((size_t *)memblk)[-1]);
    memblk = NULL;

    new_addr = realloc(addr, size + align + ADDRESS_STORAGE_SIZE);

    if (new_addr) {
      addr = new_addr;
      new_addr = (void *)(((size_t)
                           ((unsigned char *)new_addr + ADDRESS_STORAGE_SIZE) + (align - 1)) &
                          (size_t) - align);
      
      ((size_t *)new_addr)[-1] = (size_t)addr;
    }
  }

  return new_addr;
}

void vpx_free(void *memblk) {
  if (memblk) {
    void *addr = (void *)(((size_t *)memblk)[-1]);
    free(addr);
  }
}

#if CONFIG_VP9 && CONFIG_VP9_HIGHBITDEPTH
void *vpx_memset16(void *dest, int val, size_t length) {
  int i;
  void *orig = dest;
  uint16_t *dest16 = dest;
  for (i = 0; i < length; i++)
    *dest16++ = val;
  return orig;
}
#endif  
