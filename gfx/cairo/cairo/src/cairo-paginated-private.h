


































#ifndef CAIRO_PAGINATED_H
#define CAIRO_PAGINATED_H

#include "cairoint.h"

struct _cairo_paginated_surface_backend {
    








    cairo_warn cairo_int_status_t
    (*start_page)		(void			*surface);

    




    void
    (*set_paginated_mode)	(void			*surface,
				 cairo_paginated_mode_t	 mode);

    



    cairo_warn cairo_int_status_t
    (*set_bounding_box)	(void		*surface,
			 cairo_box_t	*bbox);

    



    cairo_warn cairo_int_status_t
    (*set_fallback_images_required) (void	    *surface,
				     cairo_bool_t    fallbacks_required);

    cairo_bool_t
    (*supports_fine_grained_fallbacks) (void		    *surface);
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

cairo_private cairo_status_t
_cairo_paginated_surface_set_size (cairo_surface_t 	*surface,
				   int			 width,
				   int			 height);

#endif 
