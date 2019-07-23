



















#ifndef __ATK_TEXT_H__
#define __ATK_TEXT_H__

#include <glib-object.h>
#include <atk/atkobject.h>
#include <atk/atkutil.h>

#ifdef __cplusplus
extern "C" {
#endif



































typedef enum
{
  ATK_TEXT_ATTR_INVALID = 0,
  ATK_TEXT_ATTR_LEFT_MARGIN,
  ATK_TEXT_ATTR_RIGHT_MARGIN,
  ATK_TEXT_ATTR_INDENT,
  ATK_TEXT_ATTR_INVISIBLE,
  ATK_TEXT_ATTR_EDITABLE,
  ATK_TEXT_ATTR_PIXELS_ABOVE_LINES,
  ATK_TEXT_ATTR_PIXELS_BELOW_LINES,
  ATK_TEXT_ATTR_PIXELS_INSIDE_WRAP,
  ATK_TEXT_ATTR_BG_FULL_HEIGHT,
  ATK_TEXT_ATTR_RISE,
  ATK_TEXT_ATTR_UNDERLINE,
  ATK_TEXT_ATTR_STRIKETHROUGH,
  ATK_TEXT_ATTR_SIZE,
  ATK_TEXT_ATTR_SCALE,
  ATK_TEXT_ATTR_WEIGHT,
  ATK_TEXT_ATTR_LANGUAGE,
  ATK_TEXT_ATTR_FAMILY_NAME,
  ATK_TEXT_ATTR_BG_COLOR,
  ATK_TEXT_ATTR_FG_COLOR,
  ATK_TEXT_ATTR_BG_STIPPLE,
  ATK_TEXT_ATTR_FG_STIPPLE,
  ATK_TEXT_ATTR_WRAP_MODE,
  ATK_TEXT_ATTR_DIRECTION,
  ATK_TEXT_ATTR_JUSTIFICATION,
  ATK_TEXT_ATTR_STRETCH,
  ATK_TEXT_ATTR_VARIANT,
  ATK_TEXT_ATTR_STYLE,
  ATK_TEXT_ATTR_LAST_DEFINED
} AtkTextAttribute;

AtkTextAttribute         atk_text_attribute_register   (const gchar *name);


#define ATK_TYPE_TEXT                    (atk_text_get_type ())
#define ATK_IS_TEXT(obj)                 G_TYPE_CHECK_INSTANCE_TYPE ((obj), ATK_TYPE_TEXT)
#define ATK_TEXT(obj)                    G_TYPE_CHECK_INSTANCE_CAST ((obj), ATK_TYPE_TEXT, AtkText)
#define ATK_TEXT_GET_IFACE(obj)          (G_TYPE_INSTANCE_GET_INTERFACE ((obj), ATK_TYPE_TEXT, AtkTextIface))

#ifndef _TYPEDEF_ATK_TEXT_
#define _TYPEDEF_ATK_TEXT_
typedef struct _AtkText AtkText;
#endif
typedef struct _AtkTextIface AtkTextIface;

















typedef enum {
  ATK_TEXT_BOUNDARY_CHAR,
  ATK_TEXT_BOUNDARY_WORD_START,
  ATK_TEXT_BOUNDARY_WORD_END,
  ATK_TEXT_BOUNDARY_SENTENCE_START,
  ATK_TEXT_BOUNDARY_SENTENCE_END,
  ATK_TEXT_BOUNDARY_LINE_START,
  ATK_TEXT_BOUNDARY_LINE_END
} AtkTextBoundary;











typedef struct _AtkTextRectangle AtkTextRectangle;

struct _AtkTextRectangle {
  gint x;
  gint y;
  gint width;
  gint height;
};










typedef struct _AtkTextRange AtkTextRange;

struct _AtkTextRange {
  AtkTextRectangle bounds;
  gint start_offset;
  gint end_offset;
  gchar* content;
};










typedef enum {
    ATK_TEXT_CLIP_NONE,
    ATK_TEXT_CLIP_MIN,
    ATK_TEXT_CLIP_MAX,
    ATK_TEXT_CLIP_BOTH
} AtkTextClipType;

struct _AtkTextIface
{
  GTypeInterface parent;

  gchar*         (* get_text)                     (AtkText          *text,
                                                   gint             start_offset,
                                                   gint             end_offset);
  gchar*         (* get_text_after_offset)        (AtkText          *text,
                                                   gint             offset,
                                                   AtkTextBoundary  boundary_type,
						   gint             *start_offset,
						   gint             *end_offset);
  gchar*         (* get_text_at_offset)           (AtkText          *text,
                                                   gint             offset,
                                                   AtkTextBoundary  boundary_type,
						   gint             *start_offset,
						   gint             *end_offset);
  gunichar       (* get_character_at_offset)      (AtkText          *text,
                                                   gint             offset);
  gchar*         (* get_text_before_offset)       (AtkText          *text,
                                                   gint             offset,
                                                   AtkTextBoundary  boundary_type,
 						   gint             *start_offset,
						   gint             *end_offset);
  gint           (* get_caret_offset)             (AtkText          *text);
  AtkAttributeSet* (* get_run_attributes)         (AtkText	    *text,
						   gint	  	    offset,
						   gint             *start_offset,
						   gint	 	    *end_offset);
  AtkAttributeSet* (* get_default_attributes)     (AtkText	    *text);
  void           (* get_character_extents)        (AtkText          *text,
                                                   gint             offset,
                                                   gint             *x,
                                                   gint             *y,
                                                   gint             *width,
                                                   gint             *height,
                                                   AtkCoordType	    coords);
  gint           (* get_character_count)          (AtkText          *text);
  gint           (* get_offset_at_point)          (AtkText          *text,
                                                   gint             x,
                                                   gint             y,
                                                   AtkCoordType	    coords);
  gint		 (* get_n_selections)		  (AtkText          *text);
  gchar*         (* get_selection)	          (AtkText          *text,
						   gint		    selection_num,
						   gint		    *start_offset,
						   gint		    *end_offset);
  gboolean       (* add_selection)		  (AtkText          *text,
						   gint		    start_offset,
						   gint		    end_offset);
  gboolean       (* remove_selection)		  (AtkText          *text,
						   gint             selection_num);
  gboolean       (* set_selection)		  (AtkText          *text,
						   gint		    selection_num,
						   gint		    start_offset,
						   gint		    end_offset);
  gboolean       (* set_caret_offset)             (AtkText          *text,
                                                   gint             offset);

  


  void		 (* text_changed)                 (AtkText          *text,
                                                   gint             position,
                                                   gint             length);
  void           (* text_caret_moved)             (AtkText          *text,
                                                   gint             location);
  void           (* text_selection_changed)       (AtkText          *text);

  void           (* text_attributes_changed)      (AtkText          *text);


  void           (* get_range_extents)            (AtkText          *text,
                                                   gint             start_offset,
                                                   gint             end_offset,
                                                   AtkCoordType     coord_type,
                                                   AtkTextRectangle *rect);

  AtkTextRange** (* get_bounded_ranges)           (AtkText          *text,
                                                   AtkTextRectangle *rect,
                                                   AtkCoordType     coord_type,
                                                   AtkTextClipType  x_clip_type,
                                                   AtkTextClipType  y_clip_type);
 

  AtkFunction    pad4;
};

GType            atk_text_get_type (void);









gchar*        atk_text_get_text                           (AtkText          *text,
                                                           gint             start_offset,
                                                           gint             end_offset);
gunichar      atk_text_get_character_at_offset            (AtkText          *text,
                                                           gint             offset);
gchar*        atk_text_get_text_after_offset              (AtkText          *text,
                                                           gint             offset,
                                                           AtkTextBoundary  boundary_type,
							   gint             *start_offset,
							   gint	            *end_offset);
gchar*        atk_text_get_text_at_offset                 (AtkText          *text,
                                                           gint             offset,
                                                           AtkTextBoundary  boundary_type,
							   gint             *start_offset,
							   gint             *end_offset);
gchar*        atk_text_get_text_before_offset             (AtkText          *text,
                                                           gint             offset,
                                                           AtkTextBoundary  boundary_type,
							   gint             *start_offset,
							   gint	            *end_offset);
gint          atk_text_get_caret_offset                   (AtkText          *text);
void          atk_text_get_character_extents              (AtkText          *text,
                                                           gint             offset,
                                                           gint             *x,
                                                           gint             *y,
                                                           gint             *width,
                                                           gint             *height,
                                                           AtkCoordType	    coords);
AtkAttributeSet* atk_text_get_run_attributes              (AtkText	    *text,
						           gint	  	    offset,
						           gint             *start_offset,
						           gint	 	    *end_offset);
AtkAttributeSet* atk_text_get_default_attributes          (AtkText	    *text);
gint          atk_text_get_character_count                (AtkText          *text);
gint          atk_text_get_offset_at_point                (AtkText          *text,
                                                           gint             x,
                                                           gint             y,
                                                           AtkCoordType	    coords);
gint          atk_text_get_n_selections			  (AtkText          *text);
gchar*        atk_text_get_selection			  (AtkText          *text,
							   gint		    selection_num,
							   gint             *start_offset,
							   gint             *end_offset);
gboolean      atk_text_add_selection                      (AtkText          *text,
							   gint             start_offset,
							   gint             end_offset);
gboolean      atk_text_remove_selection                   (AtkText          *text,
							   gint		    selection_num);
gboolean      atk_text_set_selection                      (AtkText          *text,
							   gint		    selection_num,
							   gint             start_offset,
							   gint             end_offset);
gboolean      atk_text_set_caret_offset                   (AtkText          *text,
                                                           gint             offset);
void          atk_text_get_range_extents                  (AtkText          *text,

                                                           gint             start_offset,
                                                           gint             end_offset,
                                                           AtkCoordType     coord_type,
                                                           AtkTextRectangle *rect);
AtkTextRange**  atk_text_get_bounded_ranges               (AtkText          *text,
                                                           AtkTextRectangle *rect,
                                                           AtkCoordType     coord_type,
                                                           AtkTextClipType  x_clip_type,
                                                           AtkTextClipType  y_clip_type);
void          atk_text_free_ranges                        (AtkTextRange     **ranges);
void 	      atk_attribute_set_free                      (AtkAttributeSet  *attrib_set);
G_CONST_RETURN gchar*  atk_text_attribute_get_name        (AtkTextAttribute attr);
AtkTextAttribute       atk_text_attribute_for_name        (const gchar      *name);
G_CONST_RETURN gchar*  atk_text_attribute_get_value       (AtkTextAttribute attr,
                                                           gint             index_);

#ifdef __cplusplus
}
#endif 


#endif
