

















#ifndef AVUTIL_FILE_H
#define AVUTIL_FILE_H

#include "avutil.h"


















int av_file_map(const char *filename, uint8_t **bufptr, size_t *size,
                int log_offset, void *log_ctx);







void av_file_unmap(uint8_t *bufptr, size_t size);

#endif 
