
















































































#ifndef _LIBUTIL_GLIST_H_
#define _LIBUTIL_GLIST_H_

#include <stdlib.h>

#include <sphinxbase/sphinxbase_export.h>
#include <sphinxbase/prim_type.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0

}
#endif



typedef struct gnode_s {
	anytype_t data;		
	struct gnode_s *next;	
} gnode_t;
typedef gnode_t *glist_t;	




#define gnode_ptr(g)		((g)->data.ptr)
#define gnode_int32(g)		((g)->data.i)
#define gnode_uint32(g)		((g)->data.ui)
#define gnode_float32(g)	((float32)(g)->data.fl)
#define gnode_float64(g)	((g)->data.fl)
#define gnode_next(g)		((g)->next)







SPHINXBASE_EXPORT
glist_t glist_add_ptr (glist_t g,  
		       void *ptr   
	);



  
SPHINXBASE_EXPORT
glist_t glist_add_int32 (glist_t g, 
			 int32 val  
	);


  
SPHINXBASE_EXPORT
glist_t glist_add_uint32 (glist_t g,  
			  uint32 val  
	);


  
SPHINXBASE_EXPORT
glist_t glist_add_float32 (glist_t g, 
			   float32 val 
	);


  
SPHINXBASE_EXPORT
glist_t glist_add_float64 (glist_t g, 
			   float64 val  
	);








SPHINXBASE_EXPORT
gnode_t *glist_insert_ptr (gnode_t *gn, 
			   void *ptr 
	);


  
SPHINXBASE_EXPORT
gnode_t *glist_insert_int32 (gnode_t *gn, 
			     int32 val 
	);


  
SPHINXBASE_EXPORT
gnode_t *glist_insert_uint32 (gnode_t *gn, 
			      uint32 val 
	);


  
SPHINXBASE_EXPORT
gnode_t *glist_insert_float32 (gnode_t *gn, 
			       float32 val 
	);


  
SPHINXBASE_EXPORT
gnode_t *glist_insert_float64 (gnode_t *gn, 
			       float64 val 
	);







SPHINXBASE_EXPORT
glist_t glist_reverse (glist_t g 
	);






SPHINXBASE_EXPORT
int32 glist_count (glist_t g 
	);





SPHINXBASE_EXPORT
void glist_free (glist_t g);






SPHINXBASE_EXPORT
gnode_t *gnode_free(gnode_t *gn, 
		    gnode_t *pred
	);




SPHINXBASE_EXPORT
gnode_t *glist_tail (glist_t g);

#ifdef __cplusplus
}
#endif

#endif
