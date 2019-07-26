
























#ifndef HB_GRAPHITE2_H
#define HB_GRAPHITE2_H

#include "hb.h"

HB_BEGIN_DECLS


#define HB_GRAPHITE2_TAG_SILF HB_TAG('S','i','l','f')


gr_face *
hb_graphite2_face_get_gr_face (hb_face_t *face);

gr_font *
hb_graphite2_font_get_gr_font (hb_font_t *font);


HB_END_DECLS

#endif 
