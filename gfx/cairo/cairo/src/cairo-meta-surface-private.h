


































#ifndef CAIRO_META_SURFACE_H
#define CAIRO_META_SURFACE_H

#include "cairoint.h"
#include "cairo-path-fixed-private.h"

typedef enum {
    
    CAIRO_COMMAND_PAINT,
    CAIRO_COMMAND_MASK,
    CAIRO_COMMAND_STROKE,
    CAIRO_COMMAND_FILL,
    CAIRO_COMMAND_SHOW_GLYPHS,

    





    CAIRO_COMMAND_INTERSECT_CLIP_PATH

} cairo_command_type_t;

typedef struct _cairo_command_paint {
    cairo_command_type_t	 type;
    cairo_operator_t		 op;
    cairo_pattern_union_t	 source;
} cairo_command_paint_t;

typedef struct _cairo_command_mask {
    cairo_command_type_t	 type;
    cairo_operator_t		 op;
    cairo_pattern_union_t	 source;
    cairo_pattern_union_t	 mask;
} cairo_command_mask_t;

typedef struct _cairo_command_stroke {
    cairo_command_type_t	 type;
    cairo_operator_t		 op;
    cairo_pattern_union_t	 source;
    cairo_path_fixed_t		 path;
    cairo_stroke_style_t	 style;
    cairo_matrix_t		 ctm;
    cairo_matrix_t		 ctm_inverse;
    double			 tolerance;
    cairo_antialias_t		 antialias;
} cairo_command_stroke_t;

typedef struct _cairo_command_fill {
    cairo_command_type_t	 type;
    cairo_operator_t		 op;
    cairo_pattern_union_t	 source;
    cairo_path_fixed_t		 path;
    cairo_fill_rule_t		 fill_rule;
    double			 tolerance;
    cairo_antialias_t		 antialias;
} cairo_command_fill_t;

typedef struct _cairo_command_show_glyphs {
    cairo_command_type_t	 type;
    cairo_operator_t		 op;
    cairo_pattern_union_t	 source;
    cairo_glyph_t		*glyphs;
    unsigned int		 num_glyphs;
    cairo_scaled_font_t		*scaled_font;
} cairo_command_show_glyphs_t;

typedef struct _cairo_command_intersect_clip_path {
    cairo_command_type_t	type;
    cairo_path_fixed_t	       *path_pointer;
    cairo_path_fixed_t		path;
    cairo_fill_rule_t		fill_rule;
    double			tolerance;
    cairo_antialias_t		antialias;
} cairo_command_intersect_clip_path_t;

typedef union _cairo_command {
    cairo_command_type_t			type;

    
    cairo_command_paint_t			paint;
    cairo_command_mask_t			mask;
    cairo_command_stroke_t			stroke;
    cairo_command_fill_t			fill;
    cairo_command_show_glyphs_t			show_glyphs;

    
    cairo_command_intersect_clip_path_t		intersect_clip_path;
} cairo_command_t;

typedef struct _cairo_meta_surface {
    cairo_surface_t base;

    cairo_content_t content;

    


    int width_pixels;
    int height_pixels;

    cairo_array_t commands;
    cairo_surface_t *commands_owner;

    cairo_bool_t is_clipped;
    int replay_start_idx;
} cairo_meta_surface_t;

cairo_private cairo_surface_t *
_cairo_meta_surface_create (cairo_content_t	content,
			    int			width_pixels,
			    int			height_pixels);

cairo_private cairo_status_t
_cairo_meta_surface_replay (cairo_surface_t *surface,
			    cairo_surface_t *target);

cairo_private cairo_bool_t
_cairo_surface_is_meta (const cairo_surface_t *surface);

#endif 
