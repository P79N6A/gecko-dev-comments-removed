

























#ifndef HB_OT_TAG_H
#define HB_OT_TAG_H

#include "hb-common.h"
#include "hb-language.h"

HB_BEGIN_DECLS


#define HB_OT_TAG_DEFAULT_SCRIPT	HB_TAG ('D', 'F', 'L', 'T')
#define HB_OT_TAG_DEFAULT_LANGUAGE	HB_TAG ('d', 'f', 'l', 't')

const hb_tag_t *
hb_ot_tags_from_script (hb_script_t script);

hb_script_t
hb_ot_tag_to_script (hb_tag_t tag);

hb_tag_t
hb_ot_tag_from_language (hb_language_t language);

hb_language_t
hb_ot_tag_to_language (hb_tag_t tag);


HB_END_DECLS

#endif 
