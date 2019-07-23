


#ifndef __ATK_ENUM_TYPES_H__
#define __ATK_ENUM_TYPES_H__

#include <glib-object.h>

G_BEGIN_DECLS

GType atk_hyperlink_state_flags_get_type (void);
#define ATK_TYPE_HYPERLINK_STATE_FLAGS (atk_hyperlink_state_flags_get_type())

GType atk_role_get_type (void);
#define ATK_TYPE_ROLE (atk_role_get_type())
GType atk_layer_get_type (void);
#define ATK_TYPE_LAYER (atk_layer_get_type())

GType atk_relation_type_get_type (void);
#define ATK_TYPE_RELATION_TYPE (atk_relation_type_get_type())

GType atk_state_type_get_type (void);
#define ATK_TYPE_STATE_TYPE (atk_state_type_get_type())

GType atk_text_attribute_get_type (void);
#define ATK_TYPE_TEXT_ATTRIBUTE (atk_text_attribute_get_type())
GType atk_text_boundary_get_type (void);
#define ATK_TYPE_TEXT_BOUNDARY (atk_text_boundary_get_type())
GType atk_text_clip_type_get_type (void);
#define ATK_TYPE_TEXT_CLIP_TYPE (atk_text_clip_type_get_type())

GType atk_key_event_type_get_type (void);
#define ATK_TYPE_KEY_EVENT_TYPE (atk_key_event_type_get_type())
GType atk_coord_type_get_type (void);
#define ATK_TYPE_COORD_TYPE (atk_coord_type_get_type())
G_END_DECLS

#endif 



