

























#ifndef HB_BLOB_PRIVATE_H
#define HB_BLOB_PRIVATE_H

#include "hb-private.h"

#include "hb-blob.h"

HB_BEGIN_DECLS


struct _hb_blob_t {
  hb_reference_count_t ref_count;

  unsigned int length;

  hb_mutex_t lock;
  

  unsigned int lock_count;
  hb_memory_mode_t mode;

  const char *data;

  hb_destroy_func_t destroy;
  void *user_data;
};

extern HB_INTERNAL hb_blob_t _hb_blob_nil;


HB_END_DECLS

#endif 
