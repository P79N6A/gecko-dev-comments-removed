

























#ifndef HB_GOBJECT_H
#define HB_GOBJECT_H

#include "hb.h"

#include <glib-object.h>

HB_BEGIN_DECLS




#define HB_GOBJECT_TYPE_BLOB hb_gobject_blob_get_type ()
GType
hb_gobject_blob_get_type (void);

#define HB_GOBJECT_TYPE_BUFFER hb_gobject_buffer_get_type ()
GType
hb_gobject_buffer_get_type (void);

#define HB_GOBJECT_TYPE_FACE hb_gobject_face_get_type ()
GType
hb_gobject_face_get_type (void);

#define HB_GOBJECT_TYPE_FONT hb_gobject_font_get_type ()
GType
hb_gobject_font_get_type (void);

#define HB_GOBJECT_TYPE_FONT_FUNCS hb_gobject_font_funcs_get_type ()
GType
hb_gobject_font_funcs_get_type (void);

#define HB_GOBJECT_TYPE_UNICODE_FUNCS hb_gobject_unicode_funcs_get_type ()
GType
hb_gobject_unicode_funcs_get_type (void);





HB_END_DECLS

#endif 
