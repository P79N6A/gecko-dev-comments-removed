




































#ifndef CAIRO_TYPES_PRIVATE_H
#define CAIRO_TYPES_PRIVATE_H



#include "cairo.h"

typedef struct _cairo_array cairo_array_t;
typedef struct _cairo_hash_table cairo_hash_table_t;
typedef struct _cairo_cache cairo_cache_t;
typedef struct _cairo_hash_entry cairo_hash_entry_t;
typedef struct _cairo_surface_backend cairo_surface_backend_t;
typedef struct _cairo_clip cairo_clip_t;
typedef struct _cairo_output_stream cairo_output_stream_t;
typedef struct _cairo_scaled_font_subsets cairo_scaled_font_subsets_t;
typedef struct _cairo_paginated_surface_backend cairo_paginated_surface_backend_t;
typedef struct _cairo_scaled_font_backend   cairo_scaled_font_backend_t;
typedef struct _cairo_font_face_backend     cairo_font_face_backend_t;
typedef struct _cairo_xlib_screen_info cairo_xlib_screen_info_t;
typedef enum _cairo_paginated_mode cairo_paginated_mode_t;
typedef cairo_array_t cairo_user_data_array_t;



































struct _cairo_hash_entry {
    unsigned long hash;
};

struct _cairo_array {
    unsigned int size;
    unsigned int num_elements;
    unsigned int element_size;
    char **elements;

    cairo_bool_t is_snapshot;
};

struct _cairo_font_options {
    cairo_antialias_t antialias;
    cairo_subpixel_order_t subpixel_order;
    cairo_hint_style_t hint_style;
    cairo_hint_metrics_t hint_metrics;
};

struct _cairo_cache {
    cairo_hash_table_t *hash_table;

    cairo_destroy_func_t entry_destroy;

    unsigned long max_size;
    unsigned long size;

    int freeze_count;
};

enum _cairo_paginated_mode {
    CAIRO_PAGINATED_MODE_ANALYZE,	
    CAIRO_PAGINATED_MODE_RENDER		
};




typedef enum _cairo_int_status {
    CAIRO_INT_STATUS_DEGENERATE = 1000,
    CAIRO_INT_STATUS_UNSUPPORTED,
    CAIRO_INT_STATUS_NOTHING_TO_DO,
    CAIRO_INT_STATUS_CACHE_EMPTY,
    CAIRO_INT_STATUS_FLATTEN_TRANSPARENCY,
    CAIRO_INT_STATUS_IMAGE_FALLBACK,
    CAIRO_INT_STATUS_ANALYZE_META_SURFACE_PATTERN,
} cairo_int_status_t;

typedef enum _cairo_internal_surface_type {
    CAIRO_INTERNAL_SURFACE_TYPE_META = 0x1000,
    CAIRO_INTERNAL_SURFACE_TYPE_PAGINATED,
    CAIRO_INTERNAL_SURFACE_TYPE_ANALYSIS,
    CAIRO_INTERNAL_SURFACE_TYPE_TEST_META,
    CAIRO_INTERNAL_SURFACE_TYPE_TEST_FALLBACK,
    CAIRO_INTERNAL_SURFACE_TYPE_TEST_PAGINATED
} cairo_internal_surface_type_t;

#endif 
