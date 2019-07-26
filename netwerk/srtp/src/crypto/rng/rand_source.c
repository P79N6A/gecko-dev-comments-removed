











































#include "config.h"

#ifdef DEV_URANDOM
# include <fcntl.h>          
# include <unistd.h>         
#elif defined(HAVE_RAND_S)
# define _CRT_RAND_S
# include <stdlib.h>         
#else
# include <stdio.h>
#endif

#include "rand_source.h"














#define RAND_SOURCE_NOT_READY (-1)
#define RAND_SOURCE_READY     (17)

static int dev_random_fdes = RAND_SOURCE_NOT_READY;


err_status_t
rand_source_init(void) {
  if (dev_random_fdes >= 0) {
    
    return err_status_ok;
  }
#ifdef DEV_URANDOM
  
  dev_random_fdes = open(DEV_URANDOM, O_RDONLY);
  if (dev_random_fdes < 0)
    return err_status_init_fail;
#elif defined(HAVE_RAND_S)
  dev_random_fdes = RAND_SOURCE_READY;
#else
  
  fprintf(stderr, "WARNING: no real random source present!\n");
  dev_random_fdes = RAND_SOURCE_READY;
#endif
  return err_status_ok;
}

err_status_t
rand_source_get_octet_string(void *dest, uint32_t len) {

  




#ifdef DEV_URANDOM
  uint8_t *dst = (uint8_t *)dest;
  while (len)
  {
    ssize_t num_read = read(dev_random_fdes, dst, len);
    if (num_read <= 0 || num_read > len)
      return err_status_fail;
    len -= num_read;
    dst += num_read;
  }
#elif defined(HAVE_RAND_S)
  uint8_t *dst = (uint8_t *)dest;
  while (len)
  {
    unsigned int val;
    errno_t err = rand_s(&val);

    if (err != 0)
      return err_status_fail;
  
    *dst++ = val & 0xff;
    len--;
  }
#else
  
  
  uint8_t *dst = (uint8_t *)dest;
  while (len)
  {
	  int val = rand();
	  
	  

	  *dst++ = val & 0xff;
	  len--;
  }
#endif
  return err_status_ok;
}
 
err_status_t
rand_source_deinit(void) {
  if (dev_random_fdes < 0)
    return err_status_dealloc_fail;  

#ifdef DEV_URANDOM
  close(dev_random_fdes);  
#endif
  dev_random_fdes = RAND_SOURCE_NOT_READY;
  
  return err_status_ok;  
}
