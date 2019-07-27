




































#ifndef _LIBUTIL_BITVEC_H_
#define _LIBUTIL_BITVEC_H_

#include <string.h>


#include <sphinxbase/sphinxbase_export.h>

#include <sphinxbase/prim_type.h>
#include <sphinxbase/ckd_alloc.h>








#ifdef __cplusplus
extern "C" {
#endif
#if 0

}
#endif

#define BITVEC_BITS 32
typedef uint32 bitvec_t;




#define bitvec_size(n)	        (((n)+BITVEC_BITS-1)/BITVEC_BITS)




#define bitvec_alloc(n)		ckd_calloc(bitvec_size(n), sizeof(bitvec_t))




SPHINXBASE_EXPORT
bitvec_t *bitvec_realloc(bitvec_t *vec,	
			 size_t old_len, 
                         size_t new_len); 



#define bitvec_free(v)		ckd_free(v)







#define bitvec_set(v,b)		(v[(b)/BITVEC_BITS] |= (1UL << ((b) & (BITVEC_BITS-1))))







#define bitvec_set_all(v,n)	memset(v, (bitvec_t)-1, \
                                       (((n)+BITVEC_BITS-1)/BITVEC_BITS) * \
                                       sizeof(bitvec_t))






#define bitvec_clear(v,b)	(v[(b)/BITVEC_BITS] &= ~(1UL << ((b) & (BITVEC_BITS-1))))







#define bitvec_clear_all(v,n)	memset(v, 0, (((n)+BITVEC_BITS-1)/BITVEC_BITS) * \
                                       sizeof(bitvec_t))







#define bitvec_is_set(v,b)	(v[(b)/BITVEC_BITS] & (1UL << ((b) & (BITVEC_BITS-1))))







#define bitvec_is_clear(v,b)	(! (bitvec_is_set(v,b)))









SPHINXBASE_EXPORT
size_t bitvec_count_set(bitvec_t *vec,	
                        size_t len);	

#ifdef __cplusplus
}
#endif

#endif
