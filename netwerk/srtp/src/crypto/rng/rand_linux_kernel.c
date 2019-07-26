











































#include "config.h"
#include "rand_source.h"


err_status_t
rand_source_init(void) {
  return err_status_ok;
}

err_status_t
rand_source_get_octet_string(void *dest, uint32_t len) {

  get_random_bytes(dest, len);

  return err_status_ok;
}

err_status_t
rand_source_deinit(void) {
  return err_status_ok;  
}
