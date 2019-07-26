



























#ifndef HB_H_IN
#error "Include <hb.h> instead."
#endif

#ifndef HB_SHAPE_H
#define HB_SHAPE_H

#include "hb-common.h"
#include "hb-buffer.h"
#include "hb-font.h"

HB_BEGIN_DECLS


typedef struct hb_feature_t {
  hb_tag_t      tag;
  uint32_t      value;
  unsigned int  start;
  unsigned int  end;
} hb_feature_t;


hb_bool_t
hb_feature_from_string (const char *str, int len,
			hb_feature_t *feature);



void
hb_feature_to_string (hb_feature_t *feature,
		      char *buf, unsigned int size);


void
hb_shape (hb_font_t           *font,
	  hb_buffer_t         *buffer,
	  const hb_feature_t  *features,
	  unsigned int         num_features);

hb_bool_t
hb_shape_full (hb_font_t          *font,
	       hb_buffer_t        *buffer,
	       const hb_feature_t *features,
	       unsigned int        num_features,
	       const char * const *shaper_list);

const char **
hb_shape_list_shapers (void);


HB_END_DECLS

#endif 
