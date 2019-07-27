


























































































































#ifndef _LIBUTIL_HASH_H_
#define _LIBUTIL_HASH_H_


#include <sphinxbase/sphinxbase_export.h>
#include <sphinxbase/prim_type.h>
#include <sphinxbase/glist.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0

}
#endif










typedef struct hash_entry_s {
	const char *key;		


	size_t len;			

	void *val;			
	struct hash_entry_s *next;	
} hash_entry_t;

typedef struct {
	hash_entry_t *table;	
	int32 size;		


	int32 inuse;		
	int32 nocase;		
} hash_table_t;

typedef struct hash_iter_s {
	hash_table_t *ht;  
	hash_entry_t *ent; 
	size_t idx;        
} hash_iter_t;


#define hash_entry_val(e)	((e)->val)
#define hash_entry_key(e)	((e)->key)
#define hash_entry_len(e)	((e)->len)
#define hash_table_inuse(h)	((h)->inuse)
#define hash_table_size(h)	((h)->size)










SPHINXBASE_EXPORT
hash_table_t * hash_table_new(int32 size,	
                              int32 casearg  	


    );

#define HASH_CASE_YES		0
#define HASH_CASE_NO		1





SPHINXBASE_EXPORT
void hash_table_free(hash_table_t *h 
    );








SPHINXBASE_EXPORT
void *hash_table_enter(hash_table_t *h, 
                       const char *key, 

                       void *val	  
    );







#define hash_table_enter_int32(h,k,v) \
    ((int32)(long)hash_table_enter((h),(k),(void *)(long)(v)))














SPHINXBASE_EXPORT
void *hash_table_replace(hash_table_t *h, 
                         const char *key, 

                         void *val	  
    );







#define hash_table_replace_int32(h,k,v) \
    ((int32)(long)hash_table_replace((h),(k),(void *)(long)(v)))






SPHINXBASE_EXPORT
void *hash_table_delete(hash_table_t *h,    

                        const char *key     

	);








SPHINXBASE_EXPORT
void *hash_table_delete_bkey(hash_table_t *h,    

                             const char *key,     

                             size_t len
	);




SPHINXBASE_EXPORT
void hash_table_empty(hash_table_t *h    
    );








SPHINXBASE_EXPORT
void *hash_table_enter_bkey(hash_table_t *h,	

                              const char *key,	
                              size_t len,	
                              void *val		
	);







#define hash_table_enter_bkey_int32(h,k,l,v) \
    ((int32)(long)hash_table_enter_bkey((h),(k),(l),(void *)(long)(v)))








SPHINXBASE_EXPORT
void *hash_table_replace_bkey(hash_table_t *h, 
                              const char *key, 
                              size_t len,	
                              void *val	  
    );







#define hash_table_replace_bkey_int32(h,k,l,v)                          \
    ((int32)(long)hash_table_replace_bkey((h),(k),(l),(void *)(long)(v)))






SPHINXBASE_EXPORT
int32 hash_table_lookup(hash_table_t *h,	
                        const char *key,	
                        void **val	  	

	);







SPHINXBASE_EXPORT
int32 hash_table_lookup_int32(hash_table_t *h,	
                              const char *key,	
                              int32 *val	

	);







SPHINXBASE_EXPORT
int32 hash_table_lookup_bkey(hash_table_t *h,	
                             const char *key,	
                             size_t len,	
                             void **val		

	);







SPHINXBASE_EXPORT
int32 hash_table_lookup_bkey_int32(hash_table_t *h,
                                   const char *key,
                                   size_t len,	
                                   int32 *val	

	);




SPHINXBASE_EXPORT
hash_iter_t *hash_table_iter(hash_table_t *h);









SPHINXBASE_EXPORT
hash_iter_t *hash_table_iter_next(hash_iter_t *itor);




SPHINXBASE_EXPORT
void hash_table_iter_free(hash_iter_t *itor);




SPHINXBASE_EXPORT
glist_t hash_table_tolist(hash_table_t *h,	
                          int32 *count		


	);






SPHINXBASE_EXPORT
void  hash_table_display(hash_table_t *h, 
                         int32 showkey    


	);

#ifdef __cplusplus
}
#endif

#endif
