












































#ifndef CIPHER_H
#define CIPHER_H

#include "datatypes.h"          
#include "rdbx.h"               
#include "err.h"                
#include "crypto.h"		
#include "crypto_types.h"	










typedef enum { 
  direction_encrypt, 
  direction_decrypt, 
  direction_any      
} cipher_direction_t;






typedef struct cipher_type_t *cipher_type_pointer_t;
typedef struct cipher_t      *cipher_pointer_t;





typedef err_status_t (*cipher_alloc_func_t)
     (cipher_pointer_t *cp, int key_len);






typedef err_status_t (*cipher_init_func_t)
(void *state, const uint8_t *key, int key_len, cipher_direction_t dir);



typedef err_status_t (*cipher_dealloc_func_t)(cipher_pointer_t cp);



typedef err_status_t (*cipher_set_segment_func_t)
     (void *state, xtd_seq_num_t idx);



typedef err_status_t (*cipher_encrypt_func_t)
     (void *state, uint8_t *buffer, unsigned int *octets_to_encrypt);



typedef err_status_t (*cipher_decrypt_func_t)
     (void *state, uint8_t *buffer, unsigned int *octets_to_decrypt);





typedef err_status_t (*cipher_set_iv_func_t)
     (cipher_pointer_t cp, void *iv);









typedef struct cipher_test_case_t {
  int key_length_octets;                      
  uint8_t *key;                               
  uint8_t *idx;                               
  int plaintext_length_octets;                 
  uint8_t *plaintext;                         
  int ciphertext_length_octets;                
  uint8_t *ciphertext;                        
  struct cipher_test_case_t *next_test_case;  
} cipher_test_case_t;



typedef struct cipher_type_t {
  cipher_alloc_func_t         alloc;
  cipher_dealloc_func_t       dealloc;
  cipher_init_func_t          init;
  cipher_encrypt_func_t       encrypt;
  cipher_encrypt_func_t       decrypt;
  cipher_set_iv_func_t        set_iv;
  char                       *description;
  int                         ref_count;
  cipher_test_case_t         *test_data;
  debug_module_t             *debug;
  cipher_type_id_t            id;
} cipher_type_t;






typedef struct cipher_t {
  cipher_type_t *type;
  void          *state;
  int            key_len;
#ifdef FORCE_64BIT_ALIGN
  int            pad;
#endif
} cipher_t;



#define cipher_type_alloc(ct, c, klen) ((ct)->alloc((c), (klen)))

#define cipher_dealloc(c) (((c)->type)->dealloc(c))

#define cipher_init(c, k, dir) (((c)->type)->init(((c)->state), (k), ((c)->key_len), (dir)))

#define cipher_encrypt(c, buf, len) \
        (((c)->type)->encrypt(((c)->state), (buf), (len)))

#define cipher_decrypt(c, buf, len) \
        (((c)->type)->decrypt(((c)->state), (buf), (len)))

#define cipher_set_iv(c, n)                           \
  ((c) ? (((c)->type)->set_iv(((cipher_pointer_t)(c)->state), (n))) :   \
                                err_status_no_such_op)  

err_status_t
cipher_output(cipher_t *c, uint8_t *buffer, int num_octets_to_output);




int
cipher_get_key_length(const cipher_t *c);








err_status_t
cipher_type_self_test(const cipher_type_t *ct);








err_status_t
cipher_type_test(const cipher_type_t *ct, const cipher_test_case_t *test_data);













uint64_t
cipher_bits_per_second(cipher_t *c, int octets_in_buffer, int num_trials);

#endif 
