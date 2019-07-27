































































#ifndef _LIBUTIL_HEAP_H_
#define _LIBUTIL_HEAP_H_

#include <stdlib.h>


#include <sphinxbase/sphinxbase_export.h>
#include <sphinxbase/prim_type.h>

  










#ifdef __cplusplus
extern "C" {
#endif
#if 0

}
#endif


typedef struct heap_s heap_t;





SPHINXBASE_EXPORT
heap_t *heap_new(void);






SPHINXBASE_EXPORT
int heap_insert(heap_t *heap,	
                void *data,	
                int32 val	
	);





SPHINXBASE_EXPORT
int heap_top(heap_t *heap,	
             void **data,	
             int32 *val		
	);



SPHINXBASE_EXPORT
int heap_pop(heap_t *heap, void **data, int32 *val);




SPHINXBASE_EXPORT
int heap_remove(heap_t *heap, void *data);




SPHINXBASE_EXPORT
size_t heap_size(heap_t *heap);






SPHINXBASE_EXPORT
int heap_destroy(heap_t *heap);

#ifdef __cplusplus
}
#endif

#endif
