

























#ifndef HB_SHAPE_H
#define HB_SHAPE_H

#include "hb-common.h"
#include "hb-buffer.h"
#include "hb-font.h"

HB_BEGIN_DECLS


typedef struct _hb_feature_t {
  hb_tag_t      tag;
  uint32_t      value;
  unsigned int  start;
  unsigned int  end;
} hb_feature_t;


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
	       const char * const *shaper_options,
	       const char * const *shaper_list);

const char **
hb_shape_list_shapers (void);


HB_END_DECLS

#endif 
