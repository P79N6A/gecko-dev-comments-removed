

























#ifndef HB_H_IN
#error "Include <hb.h> instead."
#endif

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

typedef struct hb_blob_t hb_blob_t;

hb_blob_t *
hb_blob_create (const char        *data,
		unsigned int       length,
		hb_memory_mode_t   mode,
		void              *user_data,
		hb_destroy_func_t  destroy);







hb_blob_t *
hb_blob_create_sub_blob (hb_blob_t    *parent,
			 unsigned int  offset,
			 unsigned int  length);

hb_blob_t *
hb_blob_get_empty (void);

hb_blob_t *
hb_blob_reference (hb_blob_t *blob);

void
hb_blob_destroy (hb_blob_t *blob);

hb_bool_t
hb_blob_set_user_data (hb_blob_t          *blob,
		       hb_user_data_key_t *key,
		       void *              data,
		       hb_destroy_func_t   destroy,
		       hb_bool_t           replace);


void *
hb_blob_get_user_data (hb_blob_t          *blob,
		       hb_user_data_key_t *key);


void
hb_blob_make_immutable (hb_blob_t *blob);

hb_bool_t
hb_blob_is_immutable (hb_blob_t *blob);


unsigned int
hb_blob_get_length (hb_blob_t *blob);

const char *
hb_blob_get_data (hb_blob_t *blob, unsigned int *length);

char *
hb_blob_get_data_writable (hb_blob_t *blob, unsigned int *length);


HB_END_DECLS

#endif 
