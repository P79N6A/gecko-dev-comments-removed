









































#ifndef __MMIO_H__
#define __MMIO_H__

#include <sphinxbase/sphinxbase_export.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0

}
#endif




typedef struct mmio_file_s mmio_file_t;





SPHINXBASE_EXPORT
mmio_file_t *mmio_file_read(const char *filename);




SPHINXBASE_EXPORT
void *mmio_file_ptr(mmio_file_t *mf);




SPHINXBASE_EXPORT
void mmio_file_unmap(mmio_file_t *mf);

#ifdef __cplusplus
}
#endif


#endif
