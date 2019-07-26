

























#ifndef HB_H_IN
#error "Include <hb.h> instead."
#endif

#ifndef HB_FACE_H
#define HB_FACE_H

#include "hb-common.h"
#include "hb-blob.h"

HB_BEGIN_DECLS






typedef struct hb_face_t hb_face_t;

hb_face_t *
hb_face_create (hb_blob_t    *blob,
		unsigned int  index);

typedef hb_blob_t * (*hb_reference_table_func_t)  (hb_face_t *face, hb_tag_t tag, void *user_data);


hb_face_t *
hb_face_create_for_tables (hb_reference_table_func_t  reference_table_func,
			   void                      *user_data,
			   hb_destroy_func_t          destroy);

hb_face_t *
hb_face_get_empty (void);

hb_face_t *
hb_face_reference (hb_face_t *face);

void
hb_face_destroy (hb_face_t *face);

hb_bool_t
hb_face_set_user_data (hb_face_t          *face,
		       hb_user_data_key_t *key,
		       void *              data,
		       hb_destroy_func_t   destroy,
		       hb_bool_t           replace);


void *
hb_face_get_user_data (hb_face_t          *face,
		       hb_user_data_key_t *key);

void
hb_face_make_immutable (hb_face_t *face);

hb_bool_t
hb_face_is_immutable (hb_face_t *face);


hb_blob_t *
hb_face_reference_table (hb_face_t *face,
			 hb_tag_t   tag);

hb_blob_t *
hb_face_reference_blob (hb_face_t *face);

void
hb_face_set_index (hb_face_t    *face,
		   unsigned int  index);

unsigned int
hb_face_get_index (hb_face_t    *face);

void
hb_face_set_upem (hb_face_t    *face,
		  unsigned int  upem);

unsigned int
hb_face_get_upem (hb_face_t *face);

void
hb_face_set_glyph_count (hb_face_t    *face,
			 unsigned int  glyph_count);

unsigned int
hb_face_get_glyph_count (hb_face_t *face);


HB_END_DECLS

#endif 
