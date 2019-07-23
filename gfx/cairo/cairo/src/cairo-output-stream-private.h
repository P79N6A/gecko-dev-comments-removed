



































#ifndef CAIRO_OUTPUT_STREAM_PRIVATE_H
#define CAIRO_OUTPUT_STREAM_PRIVATE_H

#include "cairo-types-private.h"

typedef cairo_status_t (*cairo_output_stream_write_func_t) (cairo_output_stream_t *output_stream,
							    const unsigned char   *data,
							    unsigned int           length);

typedef cairo_status_t (*cairo_output_stream_close_func_t) (cairo_output_stream_t *output_stream);

struct _cairo_output_stream {
    cairo_output_stream_write_func_t write_func;
    cairo_output_stream_close_func_t close_func;
    unsigned long		     position;
    cairo_status_t		     status;
    cairo_bool_t		     closed;
};

extern const cairo_private cairo_output_stream_t _cairo_output_stream_nil;

cairo_private void
_cairo_output_stream_init (cairo_output_stream_t            *stream,
			   cairo_output_stream_write_func_t  write_func,
			   cairo_output_stream_close_func_t  close_func);

cairo_private cairo_status_t
_cairo_output_stream_fini (cairo_output_stream_t *stream);








typedef cairo_status_t (*cairo_close_func_t) (void *closure);










cairo_private cairo_output_stream_t *
_cairo_output_stream_create (cairo_write_func_t		write_func,
			     cairo_close_func_t		close_func,
			     void			*closure);




cairo_private cairo_status_t
_cairo_output_stream_close (cairo_output_stream_t *stream);




cairo_private cairo_status_t
_cairo_output_stream_destroy (cairo_output_stream_t *stream);

cairo_private void
_cairo_output_stream_write (cairo_output_stream_t *stream,
			    const void *data, size_t length);

cairo_private void
_cairo_output_stream_write_hex_string (cairo_output_stream_t *stream,
				       const char *data,
				       size_t length);

cairo_private void
_cairo_dtostr (char *buffer, size_t size, double d);

cairo_private void
_cairo_output_stream_vprintf (cairo_output_stream_t *stream,
			      const char *fmt, va_list ap);

cairo_private void
_cairo_output_stream_printf (cairo_output_stream_t *stream,
			     const char *fmt, ...);

cairo_private long
_cairo_output_stream_get_position (cairo_output_stream_t *stream);

cairo_private cairo_status_t
_cairo_output_stream_get_status (cairo_output_stream_t *stream);









cairo_private cairo_output_stream_t *
_cairo_output_stream_create_for_filename (const char *filename);








cairo_private cairo_output_stream_t *
_cairo_output_stream_create_for_file (FILE *file);

cairo_private cairo_output_stream_t *
_cairo_memory_stream_create (void);

cairo_private void
_cairo_memory_stream_copy (cairo_output_stream_t *base,
			   cairo_output_stream_t *dest);

cairo_private int
_cairo_memory_stream_length (cairo_output_stream_t *stream);


cairo_private cairo_output_stream_t *
_cairo_base85_stream_create (cairo_output_stream_t *output);


cairo_private cairo_output_stream_t *
_cairo_deflate_stream_create (cairo_output_stream_t *output);

#endif 
