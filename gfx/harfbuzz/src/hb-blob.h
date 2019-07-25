

























#ifndef HB_BLOB_H
#define HB_BLOB_H

#include "hb-common.h"

HB_BEGIN_DECLS


typedef enum {
  HB_MEMORY_MODE_DUPLICATE,
  HB_MEMORY_MODE_READONLY,
  HB_MEMORY_MODE_WRITABLE,
  HB_MEMORY_MODE_READONLY_MAY_MAKE_WRITABLE
} hb_memory_mode_t;

typedef struct _hb_blob_t hb_blob_t;

hb_blob_t *
hb_blob_create (const char        *data,
		unsigned int       length,
		hb_memory_mode_t   mode,
		hb_destroy_func_t  destroy,
		void              *user_data);

hb_blob_t *
hb_blob_create_sub_blob (hb_blob_t    *parent,
			 unsigned int  offset,
			 unsigned int  length);

hb_blob_t *
hb_blob_create_empty (void);

hb_blob_t *
hb_blob_reference (hb_blob_t *blob);

unsigned int
hb_blob_get_reference_count (hb_blob_t *blob);

void
hb_blob_destroy (hb_blob_t *blob);

unsigned int
hb_blob_get_length (hb_blob_t *blob);

const char *
hb_blob_lock (hb_blob_t *blob);

void
hb_blob_unlock (hb_blob_t *blob);

hb_bool_t
hb_blob_is_writable (hb_blob_t *blob);

hb_bool_t
hb_blob_try_writable_inplace (hb_blob_t *blob);

hb_bool_t
hb_blob_try_writable (hb_blob_t *blob);


HB_END_DECLS

#endif 
