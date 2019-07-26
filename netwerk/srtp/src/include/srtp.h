












































#ifndef SRTP_H
#define SRTP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "crypto_kernel.h" 














#define SRTP_MASTER_KEY_LEN 30




#define SRTP_MAX_KEY_LEN      64





#define SRTP_MAX_TAG_LEN 12 









#define SRTP_MAX_TRAILER_LEN SRTP_MAX_TAG_LEN 














typedef enum {
  sec_serv_none          = 0, 
  sec_serv_conf          = 1, 
  sec_serv_auth          = 2, 
  sec_serv_conf_and_auth = 3  
} sec_serv_t;











typedef struct crypto_policy_t {
  cipher_type_id_t cipher_type;    

  int              cipher_key_len; 

  auth_type_id_t   auth_type;      

  int              auth_key_len;   

  int              auth_tag_len;   

  sec_serv_t       sec_serv;       

} crypto_policy_t;









typedef enum { 
  ssrc_undefined    = 0,  
  ssrc_specific     = 1,  
  ssrc_any_inbound  = 2, 


  ssrc_any_outbound = 3  


} ssrc_type_t;











typedef struct { 
  ssrc_type_t type;   
  unsigned int value; 
} ssrc_t;





typedef struct ekt_policy_ctx_t *ekt_policy_t;





typedef struct ekt_stream_ctx_t *ekt_stream_t;





























typedef struct srtp_policy_t {
  ssrc_t        ssrc;        




  crypto_policy_t rtp;         
  crypto_policy_t rtcp;        
  unsigned char *key;          

  ekt_policy_t ekt;            
 
  unsigned long window_size;   

  int        allow_repeat_tx;  





  struct srtp_policy_t *next;  
} srtp_policy_t;

















typedef struct srtp_ctx_t *srtp_t;














typedef struct srtp_stream_ctx_t *srtp_stream_t;










err_status_t
srtp_init(void);







err_status_t
srtp_shutdown(void);





































err_status_t
srtp_protect(srtp_t ctx, void *rtp_hdr, int *len_ptr);
	     










































err_status_t
srtp_unprotect(srtp_t ctx, void *srtp_hdr, int *len_ptr);
























err_status_t
srtp_create(srtp_t *session, const srtp_policy_t *policy);

















err_status_t
srtp_add_stream(srtp_t session, 
		const srtp_policy_t *policy);























err_status_t
srtp_remove_stream(srtp_t session, unsigned int ssrc);




















void
crypto_policy_set_rtp_default(crypto_policy_t *p);




















void
crypto_policy_set_rtcp_default(crypto_policy_t *p);
















#define crypto_policy_set_aes_cm_128_hmac_sha1_80(p) crypto_policy_set_rtp_default(p)

































void
crypto_policy_set_aes_cm_128_hmac_sha1_32(crypto_policy_t *p);





























void
crypto_policy_set_aes_cm_128_null_auth(crypto_policy_t *p);



























void
crypto_policy_set_null_cipher_hmac_sha1_80(crypto_policy_t *p);


























void crypto_policy_set_aes_cm_256_hmac_sha1_80(crypto_policy_t *p);


































void
crypto_policy_set_aes_cm_256_hmac_sha1_32(crypto_policy_t *p);


















err_status_t
srtp_dealloc(srtp_t s);










typedef enum {
  srtp_profile_reserved           = 0,
  srtp_profile_aes128_cm_sha1_80  = 1,
  srtp_profile_aes128_cm_sha1_32  = 2,
  srtp_profile_aes256_cm_sha1_80  = 3,
  srtp_profile_aes256_cm_sha1_32  = 4,
  srtp_profile_null_sha1_80       = 5,
  srtp_profile_null_sha1_32       = 6,
} srtp_profile_t;























err_status_t
crypto_policy_set_from_profile_for_rtp(crypto_policy_t *policy, 
				       srtp_profile_t profile);

























err_status_t
crypto_policy_set_from_profile_for_rtcp(crypto_policy_t *policy, 
				       srtp_profile_t profile);




unsigned int
srtp_profile_get_master_key_length(srtp_profile_t profile);





unsigned int
srtp_profile_get_master_salt_length(srtp_profile_t profile);













void
append_salt_to_key(unsigned char *key, unsigned int bytes_in_key,
		   unsigned char *salt, unsigned int bytes_in_salt);




























































	     

err_status_t 
srtp_protect_rtcp(srtp_t ctx, void *rtcp_hdr, int *pkt_octet_len);








































err_status_t 
srtp_unprotect_rtcp(srtp_t ctx, void *srtcp_hdr, int *pkt_octet_len);















































typedef enum { 
  event_ssrc_collision,    


  event_key_soft_limit,    


  event_key_hard_limit,    


  event_packet_index_limit 


} srtp_event_t;









typedef struct srtp_event_data_t {
  srtp_t        session;  
  srtp_stream_t stream;   
  srtp_event_t  event;    
} srtp_event_data_t;












typedef void (srtp_event_handler_func_t)(srtp_event_data_t *data);














err_status_t
srtp_install_event_handler(srtp_event_handler_func_t func);





#define SRTCP_E_BIT      0x80000000

#define SRTCP_E_BYTE_BIT 0x80
#define SRTCP_INDEX_MASK 0x7fffffff

#ifdef __cplusplus
}
#endif

#endif
