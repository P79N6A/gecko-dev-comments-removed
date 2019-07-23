


































#ifndef CAIRO_PAGINATED_SURFACE_H
#define CAIRO_PAGINATED_SURFACE_H

#include "cairoint.h"

typedef enum {
    CAIRO_PAGINATED_MODE_ANALYZE,	
    CAIRO_PAGINATED_MODE_RENDER		
} cairo_paginated_mode_t;

typedef struct _cairo_paginated_surface_backend {
    








    cairo_int_status_t
    (*start_page)		(void			*surface);

    




    void
    (*set_paginated_mode)	(void			*surface,
				 cairo_paginated_mode_t	 mode);
} cairo_paginated_surface_backend_t;




























































cairo_private cairo_surface_t *
_cairo_paginated_surface_create (cairo_surface_t				*target,
				 cairo_content_t				 content,
				 int						 width,
				 int						 height,
				 const cairo_paginated_surface_backend_t	*backend);

cairo_private cairo_surface_t *
_cairo_paginated_surface_get_target (cairo_surface_t *surface);

cairo_private cairo_bool_t
_cairo_surface_is_paginated (cairo_surface_t *surface);

#endif 
