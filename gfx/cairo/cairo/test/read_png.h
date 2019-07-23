


























#ifndef READ_PNG_H
#define READ_PNG_H

typedef enum {
    READ_PNG_SUCCESS = 0,
    READ_PNG_FILE_NOT_FOUND,
    READ_PNG_FILE_NOT_PNG,
    READ_PNG_NO_MEMORY
} read_png_status_t;

read_png_status_t
read_png_argb32 (const char         *filename,
		 unsigned char      **data,
		 unsigned int       *width,
		 unsigned int       *height,
		 unsigned int	    *stride);

#endif
