




































#ifndef __LISTELEM_ALLOC_H__
#define __LISTELEM_ALLOC_H__





#ifdef __cplusplus
extern "C" {
#endif
#if 0

}
#endif

#include <stdlib.h>
#ifdef S60
#include <types.h>
#endif


#include <sphinxbase/sphinxbase_export.h>
#include <sphinxbase/prim_type.h>




typedef struct listelem_alloc_s listelem_alloc_t;




SPHINXBASE_EXPORT
listelem_alloc_t * listelem_alloc_init(size_t elemsize);




SPHINXBASE_EXPORT
void listelem_alloc_free(listelem_alloc_t *le);


SPHINXBASE_EXPORT
void *__listelem_malloc__(listelem_alloc_t *le, char *file, int line);




#define listelem_malloc(le)	__listelem_malloc__((le),__FILE__,__LINE__)

SPHINXBASE_EXPORT
void *__listelem_malloc_id__(listelem_alloc_t *le, char *file, int line,
                             int32 *out_id);




#define listelem_malloc_id(le, oid)	__listelem_malloc_id__((le),__FILE__,__LINE__,(oid))




SPHINXBASE_EXPORT
void *listelem_get_item(listelem_alloc_t *le, int32 id);




SPHINXBASE_EXPORT
void __listelem_free__(listelem_alloc_t *le, void *elem, char *file, int line);




#define listelem_free(le,el)	__listelem_free__((le),(el),__FILE__,__LINE__)




SPHINXBASE_EXPORT
void listelem_stats(listelem_alloc_t *le);


#ifdef __cplusplus
}
#endif

#endif
