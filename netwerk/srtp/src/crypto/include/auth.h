












































#ifndef AUTH_H
#define AUTH_H

#include "datatypes.h"          
#include "err.h"                
#include "crypto.h"		
#include "crypto_types.h"	

typedef struct auth_type_t *auth_type_pointer;
typedef struct auth_t      *auth_pointer_t;

typedef err_status_t (*auth_alloc_func)
     (auth_pointer_t *ap, int key_len, int out_len);

typedef err_status_t (*auth_init_func)
     (void *state, const uint8_t *key, int key_len);

typedef err_status_t (*auth_dealloc_func)(auth_pointer_t ap);

typedef err_status_t (*auth_compute_func)
     (void *state, uint8_t *buffer, int octets_to_auth, 
      int tag_len, uint8_t *tag);

typedef err_status_t (*auth_update_func)
     (void *state, uint8_t *buffer, int octets_to_auth);

typedef err_status_t (*auth_start_func)(void *state);
     


#define auth_type_alloc(at, a, klen, outlen)                        \
                 ((at)->alloc((a), (klen), (outlen)))

#define auth_init(a, key)                                           \
                 (((a)->type)->init((a)->state, (key), ((a)->key_len)))

#define auth_compute(a, buf, len, res)                              \
       (((a)->type)->compute((a)->state, (buf), (len), (a)->out_len, (res)))

#define auth_update(a, buf, len)                                    \
       (((a)->type)->update((a)->state, (buf), (len)))

#define auth_start(a)(((a)->type)->start((a)->state))

#define auth_dealloc(c) (((c)->type)->dealloc(c))



int
auth_get_key_length(const struct auth_t *a);

int
auth_get_tag_length(const struct auth_t *a);

int
auth_get_prefix_length(const struct auth_t *a);









typedef struct auth_test_case_t {
  int key_length_octets;                    
  uint8_t *key;                             
  int data_length_octets;                    
  uint8_t *data;                            
  int tag_length_octets;                    
  uint8_t *tag;                             
  struct auth_test_case_t *next_test_case;  
} auth_test_case_t;



typedef struct auth_type_t {
  auth_alloc_func      alloc;
  auth_dealloc_func    dealloc;
  auth_init_func       init;
  auth_compute_func    compute;
  auth_update_func     update;
  auth_start_func      start;
  char                *description;
  int                  ref_count;
  auth_test_case_t    *test_data;
  debug_module_t      *debug;
  auth_type_id_t       id;
} auth_type_t;

typedef struct auth_t {
  auth_type_t *type;
  void        *state;                   
  int          out_len;           
  int          key_len;           
  int          prefix_len;        
} auth_t;







err_status_t
auth_type_self_test(const auth_type_t *at);







err_status_t
auth_type_test(const auth_type_t *at, const auth_test_case_t *test_data);






int
auth_type_get_ref_count(const auth_type_t *at);

#endif 
