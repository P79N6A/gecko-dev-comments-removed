




















































































#ifndef _LIBUTIL_CKD_ALLOC_H_
#define _LIBUTIL_CKD_ALLOC_H_

#include <stdlib.h>
#include <setjmp.h>


#include <sphinxbase/sphinxbase_export.h>
#include <sphinxbase/prim_type.h>









#ifdef __cplusplus
extern "C" {
#endif
#if 0

}
#endif
















jmp_buf *ckd_set_jump(jmp_buf *env, int abort);




void ckd_fail(char *format, ...);









SPHINXBASE_EXPORT
void *__ckd_calloc__(size_t n_elem, size_t elem_size,
		     const char *caller_file, int caller_line);

SPHINXBASE_EXPORT
void *__ckd_malloc__(size_t size,
		     const char *caller_file, int caller_line);

SPHINXBASE_EXPORT
void *__ckd_realloc__(void *ptr, size_t new_size,
		      const char *caller_file, int caller_line);





SPHINXBASE_EXPORT
char *__ckd_salloc__(const char *origstr,
		     const char *caller_file, int caller_line);





SPHINXBASE_EXPORT
void *__ckd_calloc_2d__(size_t d1, size_t d2,	
                        size_t elemsize,	
                        const char *caller_file, int caller_line);	





SPHINXBASE_EXPORT
void *__ckd_calloc_3d__(size_t d1, size_t d2, size_t d3,	
                        size_t elemsize,		
                        const char *caller_file, int caller_line);	





SPHINXBASE_EXPORT
void ****__ckd_calloc_4d__(size_t d1,
			   size_t d2,
			   size_t d3,
			   size_t d4,
			   size_t elem_size,
			   char *caller_file,
			   int caller_line);




SPHINXBASE_EXPORT
void * __ckd_alloc_3d_ptr(size_t d1,
                          size_t d2,
                          size_t d3,
                          void *store,
                          size_t elem_size,
                          char *caller_file,
                          int caller_line);




SPHINXBASE_EXPORT
void *__ckd_alloc_2d_ptr(size_t d1,
                         size_t d2,
                         void *store,
                         size_t elem_size,
                         char *caller_file,
                         int caller_line);




SPHINXBASE_EXPORT
void ckd_free(void *ptr);




SPHINXBASE_EXPORT
void ckd_free_2d(void *ptr);




SPHINXBASE_EXPORT
void ckd_free_3d(void *ptr);




SPHINXBASE_EXPORT
void ckd_free_4d(void *ptr);









#define ckd_calloc(n,sz)	__ckd_calloc__((n),(sz),__FILE__,__LINE__)




#define ckd_malloc(sz)		__ckd_malloc__((sz),__FILE__,__LINE__)




#define ckd_realloc(ptr,sz)	__ckd_realloc__(ptr,(sz),__FILE__,__LINE__)





#define ckd_salloc(ptr)		__ckd_salloc__(ptr,__FILE__,__LINE__)





#define ckd_calloc_2d(d1,d2,sz)	__ckd_calloc_2d__((d1),(d2),(sz),__FILE__,__LINE__)





#define ckd_calloc_3d(d1,d2,d3,sz) __ckd_calloc_3d__((d1),(d2),(d3),(sz),__FILE__,__LINE__)




#define ckd_calloc_4d(d1, d2, d3, d4, s)  __ckd_calloc_4d__((d1), (d2), (d3), (d4), (s), __FILE__, __LINE__)





#define ckd_alloc_2d_ptr(d1, d2, bf, sz)    __ckd_alloc_2d_ptr((d1), (d2), (bf), (sz), __FILE__, __LINE__)




#define ckd_free_2d_ptr(bf) ckd_free(bf)





#define ckd_alloc_3d_ptr(d1, d2, d3, bf, sz) __ckd_alloc_3d_ptr((d1), (d2), (d3), (bf), (sz), __FILE__, __LINE__)




#define ckd_free_3d_ptr(bf) ckd_free_2d(bf)


#ifdef __cplusplus
}
#endif

#endif
