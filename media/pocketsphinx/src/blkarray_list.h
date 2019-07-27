































































#ifndef __S2_BLKARRAY_LIST_H__
#define __S2_BLKARRAY_LIST_H__


#include <sphinxbase/prim_type.h>









typedef struct blkarray_list_s {
  void ***ptr;		
  int32 maxblks;	
  int32 blksize;	
  int32 n_valid;	
  int32 cur_row;	
  int32 cur_row_free;	
} blkarray_list_t;


#define blkarray_list_ptr(l,r,c)	((l)->ptr[r][c])
#define blkarray_list_maxblks(l)	((l)->maxblks)
#define blkarray_list_blksize(l)	((l)->blksize)
#define blkarray_list_n_valid(l)	((l)->n_valid)
#define blkarray_list_cur_row(l)	((l)->cur_row)
#define blkarray_list_cur_row_free(l)	((l)->cur_row_free)









blkarray_list_t *_blkarray_list_init (int32 maxblks, int32 blksize);






blkarray_list_t *blkarray_list_init ( void );




void blkarray_list_free(blkarray_list_t *bl);









int32 blkarray_list_append (blkarray_list_t *, void *data);






void blkarray_list_reset (blkarray_list_t *);



void * blkarray_list_get(blkarray_list_t *, int32 n);

#endif
