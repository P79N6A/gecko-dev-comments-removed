











































#include "alloc.h"
#include "crypto_kernel.h"



debug_module_t mod_alloc = {
  0,                  
  "alloc"             
};










#ifdef SRTP_KERNEL_LINUX

#include <linux/interrupt.h>

void *
crypto_alloc(size_t size) {
  void *ptr;

  ptr = kmalloc(size, in_interrupt() ? GFP_ATOMIC : GFP_KERNEL);

  if (ptr) {
    debug_print(mod_alloc, "(location: %p) allocated", ptr);
  } else
    debug_print(mod_alloc, "allocation failed (asked for %d bytes)\n", size);

  return ptr;
}

void 
crypto_free(void *ptr) {

  debug_print(mod_alloc, "(location: %p) freed", ptr);

  kfree(ptr);
}


#elif defined(HAVE_STDLIB_H)

void *
crypto_alloc(size_t size) {
  void *ptr;

  ptr = malloc(size);
    
  if (ptr) {
    debug_print(mod_alloc, "(location: %p) allocated", ptr);
  } else
    debug_print(mod_alloc, "allocation failed (asked for %d bytes)\n", size);
    
  return ptr;
}

void 
crypto_free(void *ptr) {

  debug_print(mod_alloc, "(location: %p) freed", ptr);

  free(ptr);
}

#else  

#error no memory allocation defined yet 

#endif
