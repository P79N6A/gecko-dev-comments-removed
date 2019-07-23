


































#ifndef CAIRO_PAGINATED_H
#define CAIRO_PAGINATED_H

#include "cairoint.h"

struct _cairo_paginated_surface_backend {
    








    cairo_warn cairo_int_status_t
    (*start_page)		(void			*surface);

    




    void
    (*set_paginated_mode)	(void			*surface,
				 cairo_paginated_mode_t	 mode);
};




























































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
