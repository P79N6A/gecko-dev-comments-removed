




































#ifndef CAIRO_TYPES_PRIVATE_H
#define CAIRO_TYPES_PRIVATE_H

typedef struct _cairo_array cairo_array_t;
struct _cairo_array {
    unsigned int size;
    unsigned int num_elements;
    unsigned int element_size;
    char **elements;

    cairo_bool_t is_snapshot;
};

typedef cairo_array_t cairo_user_data_array_t;

struct _cairo_font_options {
    cairo_antialias_t antialias;
    cairo_subpixel_order_t subpixel_order;
    cairo_hint_style_t hint_style;
    cairo_hint_metrics_t hint_metrics;
};

typedef struct _cairo_hash_table cairo_hash_table_t;

typedef struct _cairo_cache {
    cairo_hash_table_t *hash_table;

    cairo_destroy_func_t entry_destroy;

    unsigned long max_size;
    unsigned long size;

    int freeze_count;
} cairo_cache_t;



































typedef struct _cairo_hash_entry {
    unsigned long hash;
} cairo_hash_entry_t;


typedef struct _cairo_surface_backend cairo_surface_backend_t;
typedef struct _cairo_clip cairo_clip_t;
typedef struct _cairo_output_stream cairo_output_stream_t;
typedef struct _cairo_scaled_font_subsets cairo_scaled_font_subsets_t;
typedef struct _cairo_paginated_surface_backend cairo_paginated_surface_backend_t;
typedef struct _cairo_scaled_font_backend   cairo_scaled_font_backend_t;
typedef struct _cairo_font_face_backend     cairo_font_face_backend_t;


typedef struct _cairo_xlib_screen_info cairo_xlib_screen_info_t;

typedef enum {
    CAIRO_PAGINATED_MODE_ANALYZE,	
    CAIRO_PAGINATED_MODE_RENDER		
} cairo_paginated_mode_t;

#endif 
