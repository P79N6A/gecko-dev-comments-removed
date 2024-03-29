



































#ifndef CAIRO_RECORDING_SURFACE_H
#define CAIRO_RECORDING_SURFACE_H

#include "cairoint.h"
#include "cairo-path-fixed-private.h"
#include "cairo-clip-private.h"

typedef enum {
    
    CAIRO_COMMAND_PAINT,
    CAIRO_COMMAND_MASK,
    CAIRO_COMMAND_STROKE,
    CAIRO_COMMAND_FILL,
    CAIRO_COMMAND_SHOW_TEXT_GLYPHS,
} cairo_command_type_t;

typedef enum {
    CAIRO_RECORDING_REGION_ALL,
    CAIRO_RECORDING_REGION_NATIVE,
    CAIRO_RECORDING_REGION_IMAGE_FALLBACK
} cairo_recording_region_type_t;

typedef struct _cairo_command_header {
    cairo_command_type_t	 type;
    cairo_recording_region_type_t     region;
    cairo_operator_t		 op;
    cairo_clip_t		 clip;
} cairo_command_header_t;

typedef struct _cairo_command_paint {
    cairo_command_header_t       header;
    cairo_pattern_union_t	 source;
} cairo_command_paint_t;

typedef struct _cairo_command_mask {
    cairo_command_header_t       header;
    cairo_pattern_union_t	 source;
    cairo_pattern_union_t	 mask;
} cairo_command_mask_t;

typedef struct _cairo_command_stroke {
    cairo_command_header_t       header;
    cairo_pattern_union_t	 source;
    cairo_path_fixed_t		 path;
    cairo_stroke_style_t	 style;
    cairo_matrix_t		 ctm;
    cairo_matrix_t		 ctm_inverse;
    double			 tolerance;
    cairo_antialias_t		 antialias;
} cairo_command_stroke_t;

typedef struct _cairo_command_fill {
    cairo_command_header_t       header;
    cairo_pattern_union_t	 source;
    cairo_path_fixed_t		 path;
    cairo_fill_rule_t		 fill_rule;
    double			 tolerance;
    cairo_antialias_t		 antialias;
} cairo_command_fill_t;

typedef struct _cairo_command_show_text_glyphs {
    cairo_command_header_t       header;
    cairo_pattern_union_t	 source;
    char			*utf8;
    int				 utf8_len;
    cairo_glyph_t		*glyphs;
    unsigned int		 num_glyphs;
    cairo_text_cluster_t	*clusters;
    int				 num_clusters;
    cairo_text_cluster_flags_t   cluster_flags;
    cairo_scaled_font_t		*scaled_font;
} cairo_command_show_text_glyphs_t;

typedef union _cairo_command {
    cairo_command_header_t      header;

    cairo_command_paint_t			paint;
    cairo_command_mask_t			mask;
    cairo_command_stroke_t			stroke;
    cairo_command_fill_t			fill;
    cairo_command_show_text_glyphs_t		show_text_glyphs;
} cairo_command_t;

typedef struct _cairo_recording_surface {
    cairo_surface_t base;

    cairo_content_t content;

    


    cairo_rectangle_t extents_pixels;
    cairo_rectangle_int_t extents;
    cairo_bool_t unbounded;

    cairo_clip_t clip;

    cairo_array_t commands;

    int replay_start_idx;
} cairo_recording_surface_t;

slim_hidden_proto (cairo_recording_surface_create);

cairo_private cairo_int_status_t
_cairo_recording_surface_get_path (cairo_surface_t	 *surface,
				   cairo_path_fixed_t *path);

cairo_private cairo_status_t
_cairo_recording_surface_replay (cairo_surface_t *surface,
				 cairo_surface_t *target);


cairo_private cairo_status_t
_cairo_recording_surface_replay_analyze_recording_pattern (cairo_surface_t *surface,
							   cairo_surface_t *target);

cairo_private cairo_status_t
_cairo_recording_surface_replay_and_create_regions (cairo_surface_t *surface,
						    cairo_surface_t *target);
cairo_private cairo_status_t
_cairo_recording_surface_replay_region (cairo_surface_t			*surface,
					const cairo_rectangle_int_t *surface_extents,
					cairo_surface_t			*target,
					cairo_recording_region_type_t	region);

cairo_private cairo_status_t
_cairo_recording_surface_get_bbox (cairo_recording_surface_t *recording,
				   cairo_box_t *bbox,
				   const cairo_matrix_t *transform);

cairo_private cairo_bool_t
_cairo_surface_is_recording (const cairo_surface_t *surface);

#endif 
