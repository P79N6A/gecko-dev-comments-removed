

























#ifndef HB_GOBJECT_H_IN
#error "Include <hb-gobject.h> instead."
#endif

#ifndef HB_GOBJECT_STRUCTS_H
#define HB_GOBJECT_STRUCTS_H

#include "hb.h"

#include <glib-object.h>

HB_BEGIN_DECLS




GType hb_gobject_blob_get_type (void);
#define HB_GOBJECT_TYPE_BLOB (hb_gobject_blob_get_type ())

GType hb_gobject_buffer_get_type (void);
#define HB_GOBJECT_TYPE_BUFFER (hb_gobject_buffer_get_type ())

GType hb_gobject_face_get_type (void);
#define HB_GOBJECT_TYPE_FACE (hb_gobject_face_get_type ())

GType hb_gobject_font_get_type (void);
#define HB_GOBJECT_TYPE_FONT (hb_gobject_font_get_type ())

GType hb_gobject_font_funcs_get_type (void);
#define HB_GOBJECT_TYPE_FONT_FUNCS (hb_gobject_font_funcs_get_type ())

GType hb_gobject_set_get_type (void);
#define HB_GOBJECT_TYPE_SET (hb_gobject_set_get_type ())

GType hb_gobject_shape_plan_get_type (void);
#define HB_GOBJECT_TYPE_SHAPE_PLAN (hb_gobject_shape_plan_get_type ())

GType hb_gobject_unicode_funcs_get_type (void);
#define HB_GOBJECT_TYPE_UNICODE_FUNCS (hb_gobject_unicode_funcs_get_type ())



GType hb_gobject_feature_get_type (void);
#define HB_GOBJECT_TYPE_FEATURE (hb_gobject_feature_get_type ())

GType hb_gobject_glyph_info_get_type (void);
#define HB_GOBJECT_TYPE_GLYPH_INFO (hb_gobject_glyph_info_get_type ())

GType hb_gobject_glyph_position_get_type (void);
#define HB_GOBJECT_TYPE_GLYPH_POSITION (hb_gobject_glyph_position_get_type ())

GType hb_gobject_segment_properties_get_type (void);
#define HB_GOBJECT_TYPE_SEGMENT_PROPERTIES (hb_gobject_segment_properties_get_type ())

GType hb_gobject_user_data_key_get_type (void);
#define HB_GOBJECT_TYPE_USER_DATA_KEY (hb_gobject_user_data_key_get_type ())







GType hb_gobject_language_get_type (void);
#define HB_GOBJECT_TYPE_LANGUAGE (hb_gobject_language_get_type ())

HB_END_DECLS

#endif 
