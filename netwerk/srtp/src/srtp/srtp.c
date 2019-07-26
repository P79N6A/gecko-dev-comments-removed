












































#include "srtp.h"
#include "ekt.h"             
#include "alloc.h"           

#ifndef SRTP_KERNEL
# include <limits.h>
# ifdef HAVE_NETINET_IN_H
#  include <netinet/in.h>
# elif defined(HAVE_WINSOCK2_H)
#  include <winsock2.h>
# endif
#endif 




debug_module_t mod_srtp = {
  0,                  
  "srtp"              
};

#define octets_in_rtp_header   12
#define uint32s_in_rtp_header  3
#define octets_in_rtcp_header  8
#define uint32s_in_rtcp_header 2


err_status_t
srtp_stream_alloc(srtp_stream_ctx_t **str_ptr,
		  const srtp_policy_t *p) {
  srtp_stream_ctx_t *str;
  err_status_t stat;

  







  
  str = (srtp_stream_ctx_t *) crypto_alloc(sizeof(srtp_stream_ctx_t));
  if (str == NULL)
    return err_status_alloc_fail;
  *str_ptr = str;  
  
  
  stat = crypto_kernel_alloc_cipher(p->rtp.cipher_type, 
				    &str->rtp_cipher, 
				    p->rtp.cipher_key_len); 
  if (stat) {
    crypto_free(str);
    return stat;
  }

  
  stat = crypto_kernel_alloc_auth(p->rtp.auth_type, 
				  &str->rtp_auth,
				  p->rtp.auth_key_len, 
				  p->rtp.auth_tag_len); 
  if (stat) {
    cipher_dealloc(str->rtp_cipher);
    crypto_free(str);
    return stat;
  }
  
  
  str->limit = (key_limit_ctx_t*) crypto_alloc(sizeof(key_limit_ctx_t));
  if (str->limit == NULL) {
    auth_dealloc(str->rtp_auth);
    cipher_dealloc(str->rtp_cipher);
    crypto_free(str); 
    return err_status_alloc_fail;
  }

  



  stat = crypto_kernel_alloc_cipher(p->rtcp.cipher_type, 
				    &str->rtcp_cipher, 
				    p->rtcp.cipher_key_len); 
  if (stat) {
    auth_dealloc(str->rtp_auth);
    cipher_dealloc(str->rtp_cipher);
    crypto_free(str->limit);
    crypto_free(str);
    return stat;
  }

  
  stat = crypto_kernel_alloc_auth(p->rtcp.auth_type, 
				  &str->rtcp_auth,
				  p->rtcp.auth_key_len, 
				  p->rtcp.auth_tag_len); 
  if (stat) {
    cipher_dealloc(str->rtcp_cipher);
    auth_dealloc(str->rtp_auth);
    cipher_dealloc(str->rtp_cipher);
    crypto_free(str->limit);
    crypto_free(str);
   return stat;
  }  

  
  stat = ekt_alloc(&str->ekt, p->ekt);
  if (stat) {
    auth_dealloc(str->rtcp_auth);
    cipher_dealloc(str->rtcp_cipher);
    auth_dealloc(str->rtp_auth);
    cipher_dealloc(str->rtp_cipher);
    crypto_free(str->limit);
    crypto_free(str);
   return stat;    
  }

  return err_status_ok;
}

err_status_t
srtp_stream_dealloc(srtp_t session, srtp_stream_ctx_t *stream) { 
  err_status_t status;
  
  





  
  if (session->stream_template
      && stream->rtp_cipher == session->stream_template->rtp_cipher) {
    
  } else {
    status = cipher_dealloc(stream->rtp_cipher); 
    if (status) 
      return status;
  }

  
  if (session->stream_template
      && stream->rtp_auth == session->stream_template->rtp_auth) {
    
  } else {
    status = auth_dealloc(stream->rtp_auth);
    if (status)
      return status;
  }

  
  if (session->stream_template
      && stream->limit == session->stream_template->limit) {
    
  } else {
    crypto_free(stream->limit);
  }   

  



  if (session->stream_template
      && stream->rtcp_cipher == session->stream_template->rtcp_cipher) {
    
  } else {
    status = cipher_dealloc(stream->rtcp_cipher); 
    if (status) 
      return status;
  }

  



  if (session->stream_template
      && stream->rtcp_auth == session->stream_template->rtcp_auth) {
    
  } else {
    status = auth_dealloc(stream->rtcp_auth);
    if (status)
      return status;
  }

  status = rdbx_dealloc(&stream->rtp_rdbx);
  if (status)
    return status;

  
  
  
  crypto_free(stream);

  return err_status_ok;
}










err_status_t
srtp_stream_clone(const srtp_stream_ctx_t *stream_template, 
		  uint32_t ssrc, 
		  srtp_stream_ctx_t **str_ptr) {
  err_status_t status;
  srtp_stream_ctx_t *str;

  debug_print(mod_srtp, "cloning stream (SSRC: 0x%08x)", ssrc);

  
  str = (srtp_stream_ctx_t *) crypto_alloc(sizeof(srtp_stream_ctx_t));
  if (str == NULL)
    return err_status_alloc_fail;
  *str_ptr = str;  

  
  str->rtp_cipher  = stream_template->rtp_cipher;
  str->rtp_auth    = stream_template->rtp_auth;
  str->rtcp_cipher = stream_template->rtcp_cipher;
  str->rtcp_auth   = stream_template->rtcp_auth;

  
  status = key_limit_clone(stream_template->limit, &str->limit);
  if (status) 
    return status;

  
  status = rdbx_init(&str->rtp_rdbx,
		     rdbx_get_window_size(&stream_template->rtp_rdbx));
  if (status)
    return status;
  rdb_init(&str->rtcp_rdb);
  str->allow_repeat_tx = stream_template->allow_repeat_tx;
  
  
  str->ssrc = ssrc;

  
  str->direction     = stream_template->direction;
  str->rtp_services  = stream_template->rtp_services;
  str->rtcp_services = stream_template->rtcp_services;

  
  str->ekt = stream_template->ekt;

  
  str->next = NULL;

  return err_status_ok;
}


















typedef enum {
  label_rtp_encryption  = 0x00,
  label_rtp_msg_auth    = 0x01,
  label_rtp_salt        = 0x02,
  label_rtcp_encryption = 0x03,
  label_rtcp_msg_auth   = 0x04,
  label_rtcp_salt       = 0x05
} srtp_prf_label;







typedef struct { 
  cipher_t *cipher;      
} srtp_kdf_t;

err_status_t
srtp_kdf_init(srtp_kdf_t *kdf, cipher_type_id_t cipher_id, const uint8_t *key, int length) {

  err_status_t stat;
  stat = crypto_kernel_alloc_cipher(cipher_id, &kdf->cipher, length);
  if (stat)
    return stat;

  stat = cipher_init(kdf->cipher, key, direction_encrypt);
  if (stat) {
    cipher_dealloc(kdf->cipher);
    return stat;
  }

  return err_status_ok;
}

err_status_t
srtp_kdf_generate(srtp_kdf_t *kdf, srtp_prf_label label,
		  uint8_t *key, unsigned length) {

  v128_t nonce;
  err_status_t status;
  
  
  v128_set_to_zero(&nonce);
  nonce.v8[7] = label;
 
  status = cipher_set_iv(kdf->cipher, &nonce);
  if (status)
    return status;
  
  
  octet_string_set_to_zero(key, length);
  status = cipher_encrypt(kdf->cipher, key, &length);
  if (status)
    return status;

  return err_status_ok;
}

err_status_t
srtp_kdf_clear(srtp_kdf_t *kdf) {
  err_status_t status;
  status = cipher_dealloc(kdf->cipher);
  if (status)
    return status;
  kdf->cipher = NULL;

  return err_status_ok;  
}





#define MAX_SRTP_KEY_LEN 256







static inline int base_key_length(const cipher_type_t *cipher, int key_length)
{
  if (cipher->id != AES_ICM)
    return key_length;
  else if (key_length > 16 && key_length < 30)
    return 16;
  return key_length - 14;
}

err_status_t
srtp_stream_init_keys(srtp_stream_ctx_t *srtp, const void *key) {
  err_status_t stat;
  srtp_kdf_t kdf;
  uint8_t tmp_key[MAX_SRTP_KEY_LEN];
  int kdf_keylen = 30, rtp_keylen, rtcp_keylen;
  int rtp_base_key_len, rtp_salt_len;
  int rtcp_base_key_len, rtcp_salt_len;

  
  

  rtp_keylen = cipher_get_key_length(srtp->rtp_cipher);
  if (rtp_keylen > kdf_keylen)
    kdf_keylen = rtp_keylen;

  rtcp_keylen = cipher_get_key_length(srtp->rtcp_cipher);
  if (rtcp_keylen > kdf_keylen)
    kdf_keylen = rtcp_keylen;

  
  stat = srtp_kdf_init(&kdf, AES_ICM, (const uint8_t *)key, kdf_keylen);
  if (stat) {
    return err_status_init_fail;
  }

  rtp_base_key_len = base_key_length(srtp->rtp_cipher->type, rtp_keylen);
  rtp_salt_len = rtp_keylen - rtp_base_key_len;
  
  
  stat = srtp_kdf_generate(&kdf, label_rtp_encryption, 
			   tmp_key, rtp_base_key_len);
  if (stat) {
    
    octet_string_set_to_zero(tmp_key, MAX_SRTP_KEY_LEN);
    return err_status_init_fail;
  }

  



  if (rtp_salt_len > 0) {
    debug_print(mod_srtp, "found rtp_salt_len > 0, generating salt", NULL);

    
    stat = srtp_kdf_generate(&kdf, label_rtp_salt, 
			     tmp_key + rtp_base_key_len, rtp_salt_len);
    if (stat) {
      
      octet_string_set_to_zero(tmp_key, MAX_SRTP_KEY_LEN);
      return err_status_init_fail;
    }
  }
  debug_print(mod_srtp, "cipher key: %s", 
	      octet_string_hex_string(tmp_key, rtp_base_key_len));
  if (rtp_salt_len > 0) {
    debug_print(mod_srtp, "cipher salt: %s",
		octet_string_hex_string(tmp_key + rtp_base_key_len, rtp_salt_len));
  }

  
  stat = cipher_init(srtp->rtp_cipher, tmp_key, direction_any);
  if (stat) {
    
    octet_string_set_to_zero(tmp_key, MAX_SRTP_KEY_LEN);
    return err_status_init_fail;
  }

  
  stat = srtp_kdf_generate(&kdf, label_rtp_msg_auth,
			   tmp_key, auth_get_key_length(srtp->rtp_auth));
  if (stat) {
    
    octet_string_set_to_zero(tmp_key, MAX_SRTP_KEY_LEN);
    return err_status_init_fail;
  }
  debug_print(mod_srtp, "auth key:   %s",
	      octet_string_hex_string(tmp_key, 
				      auth_get_key_length(srtp->rtp_auth))); 

  
  stat = auth_init(srtp->rtp_auth, tmp_key);
  if (stat) {
    
    octet_string_set_to_zero(tmp_key, MAX_SRTP_KEY_LEN);
    return err_status_init_fail;
  }

  



  rtcp_base_key_len = base_key_length(srtp->rtcp_cipher->type, rtcp_keylen);
  rtcp_salt_len = rtcp_keylen - rtcp_base_key_len;
  
  
  stat = srtp_kdf_generate(&kdf, label_rtcp_encryption, 
			   tmp_key, rtcp_base_key_len);
  if (stat) {
    
    octet_string_set_to_zero(tmp_key, MAX_SRTP_KEY_LEN);
    return err_status_init_fail;
  }

  



  if (rtcp_salt_len > 0) {
    debug_print(mod_srtp, "found rtcp_salt_len > 0, generating rtcp salt",
		NULL);

    
    stat = srtp_kdf_generate(&kdf, label_rtcp_salt, 
			     tmp_key + rtcp_base_key_len, rtcp_salt_len);
    if (stat) {
      
      octet_string_set_to_zero(tmp_key, MAX_SRTP_KEY_LEN);
      return err_status_init_fail;
    }
  }
  debug_print(mod_srtp, "rtcp cipher key: %s", 
	      octet_string_hex_string(tmp_key, rtcp_base_key_len));  
  if (rtcp_salt_len > 0) {
    debug_print(mod_srtp, "rtcp cipher salt: %s",
		octet_string_hex_string(tmp_key + rtcp_base_key_len, rtcp_salt_len));
  }

  
  stat = cipher_init(srtp->rtcp_cipher, tmp_key, direction_any);
  if (stat) {
    
    octet_string_set_to_zero(tmp_key, MAX_SRTP_KEY_LEN);
    return err_status_init_fail;
  }

  
  stat = srtp_kdf_generate(&kdf, label_rtcp_msg_auth,
			   tmp_key, auth_get_key_length(srtp->rtcp_auth));
  if (stat) {
    
    octet_string_set_to_zero(tmp_key, MAX_SRTP_KEY_LEN);
    return err_status_init_fail;
  }

  debug_print(mod_srtp, "rtcp auth key:   %s",
	      octet_string_hex_string(tmp_key, 
		     auth_get_key_length(srtp->rtcp_auth))); 

  
  stat = auth_init(srtp->rtcp_auth, tmp_key);
  if (stat) {
    
    octet_string_set_to_zero(tmp_key, MAX_SRTP_KEY_LEN);
    return err_status_init_fail;
  }

  
  stat = srtp_kdf_clear(&kdf);
  octet_string_set_to_zero(tmp_key, MAX_SRTP_KEY_LEN);  
  if (stat)
    return err_status_init_fail;

  return err_status_ok;
}

err_status_t
srtp_stream_init(srtp_stream_ctx_t *srtp, 
		  const srtp_policy_t *p) {
  err_status_t err;

   debug_print(mod_srtp, "initializing stream (SSRC: 0x%08x)", 
	       p->ssrc.value);

   
   



   if (p->window_size != 0 && (p->window_size < 64 || p->window_size >= 0x8000))
     return err_status_bad_param;

   if (p->window_size != 0)
     err = rdbx_init(&srtp->rtp_rdbx, p->window_size);
   else
     err = rdbx_init(&srtp->rtp_rdbx, 128);
   if (err) return err;

   
#ifdef NO_64BIT_MATH
{
   uint64_t temp;
   temp = make64(UINT_MAX,UINT_MAX);
   key_limit_set(srtp->limit, temp);
}
#else
   key_limit_set(srtp->limit, 0xffffffffffffLL);
#endif

   
   srtp->ssrc = htonl(p->ssrc.value);

   
   srtp->rtp_services  = p->rtp.sec_serv;
   srtp->rtcp_services = p->rtcp.sec_serv;

   




   srtp->direction = dir_unknown;

   
   rdb_init(&srtp->rtcp_rdb);

   
   
   if (p->allow_repeat_tx != 0 && p->allow_repeat_tx != 1) {
     rdbx_dealloc(&srtp->rtp_rdbx);
     return err_status_bad_param;
   }
   srtp->allow_repeat_tx = p->allow_repeat_tx;

   

   
   err = srtp_stream_init_keys(srtp, p->key);
   if (err) {
     rdbx_dealloc(&srtp->rtp_rdbx);
     return err;
   }

   



   err = ekt_stream_init_from_policy(srtp->ekt, p->ekt);
   if (err) {
     rdbx_dealloc(&srtp->rtp_rdbx);
     return err;
   }

   return err_status_ok;  
 }


 




 void
 srtp_event_reporter(srtp_event_data_t *data) {

   err_report(err_level_warning, "srtp: in stream 0x%x: ", 
	      data->stream->ssrc);

   switch(data->event) {
   case event_ssrc_collision:
     err_report(err_level_warning, "\tSSRC collision\n");
     break;
   case event_key_soft_limit:
     err_report(err_level_warning, "\tkey usage soft limit reached\n");
     break;
   case event_key_hard_limit:
     err_report(err_level_warning, "\tkey usage hard limit reached\n");
     break;
   case event_packet_index_limit:
     err_report(err_level_warning, "\tpacket index limit reached\n");
     break;
   default:
     err_report(err_level_warning, "\tunknown event reported to handler\n");
   }
 }

 









 static srtp_event_handler_func_t *srtp_event_handler = srtp_event_reporter;

 err_status_t
 srtp_install_event_handler(srtp_event_handler_func_t func) {

   





   
   srtp_event_handler = func;
   return err_status_ok;
 }

 err_status_t
 srtp_protect(srtp_ctx_t *ctx, void *rtp_hdr, int *pkt_octet_len) {
   srtp_hdr_t *hdr = (srtp_hdr_t *)rtp_hdr;
   uint32_t *enc_start;        
   uint32_t *auth_start;       
   unsigned enc_octet_len = 0; 
   xtd_seq_num_t est;          
   int delta;                  
   uint8_t *auth_tag = NULL;   
   err_status_t status;   
   int tag_len;
   srtp_stream_ctx_t *stream;
   int prefix_len;

   debug_print(mod_srtp, "function srtp_protect", NULL);

  

   
   if (*pkt_octet_len < octets_in_rtp_header)
     return err_status_bad_param;

   






   stream = srtp_get_stream(ctx, hdr->ssrc);
   if (stream == NULL) {
     if (ctx->stream_template != NULL) {
       srtp_stream_ctx_t *new_stream;

       
       status = srtp_stream_clone(ctx->stream_template, 
				  hdr->ssrc, &new_stream); 
       if (status)
	 return status;

       
       new_stream->next = ctx->stream_list;
       ctx->stream_list = new_stream;

       
       new_stream->direction = dir_srtp_sender;

       
       stream = new_stream;
     } else {
       
       return err_status_no_ctx;
     } 
   }

   





   if (stream->direction != dir_srtp_sender) {
     if (stream->direction == dir_unknown) {
       stream->direction = dir_srtp_sender;
     } else {
       srtp_handle_event(ctx, stream, event_ssrc_collision);
     }
   }

  




  switch(key_limit_update(stream->limit)) {
  case key_event_normal:
    break;
  case key_event_soft_limit: 
    srtp_handle_event(ctx, stream, event_key_soft_limit);
    break; 
  case key_event_hard_limit:
    srtp_handle_event(ctx, stream, event_key_hard_limit);
	return err_status_key_expired;
  default:
    break;
  }

   
   tag_len = auth_get_tag_length(stream->rtp_auth); 

   







   if (stream->rtp_services & sec_serv_conf) {
     enc_start = (uint32_t *)hdr + uint32s_in_rtp_header + hdr->cc;  
     if (hdr->x == 1) {
       srtp_hdr_xtnd_t *xtn_hdr = (srtp_hdr_xtnd_t *)enc_start;
       enc_start += (ntohs(xtn_hdr->length) + 1);
     }
     enc_octet_len = (unsigned int)(*pkt_octet_len 
				    - ((enc_start - (uint32_t *)hdr) << 2));
   } else {
     enc_start = NULL;
   }

   




   if (stream->rtp_services & sec_serv_auth) {
     auth_start = (uint32_t *)hdr;
     auth_tag = (uint8_t *)hdr + *pkt_octet_len;
   } else {
     auth_start = NULL;
     auth_tag = NULL;
   }

   



   delta = rdbx_estimate_index(&stream->rtp_rdbx, &est, ntohs(hdr->seq));
   status = rdbx_check(&stream->rtp_rdbx, delta);
   if (status) {
     if (status != err_status_replay_fail || !stream->allow_repeat_tx)
       return status;  
   }
   else
     rdbx_add_index(&stream->rtp_rdbx, delta);

#ifdef NO_64BIT_MATH
   debug_print2(mod_srtp, "estimated packet index: %08x%08x", 
		high32(est),low32(est));
#else
   debug_print(mod_srtp, "estimated packet index: %016llx", est);
#endif

   


   if (stream->rtp_cipher->type->id == AES_ICM) {
     v128_t iv;

     iv.v32[0] = 0;
     iv.v32[1] = hdr->ssrc;
#ifdef NO_64BIT_MATH
     iv.v64[1] = be64_to_cpu(make64((high32(est) << 16) | (low32(est) >> 16),
								 low32(est) << 16));
#else
     iv.v64[1] = be64_to_cpu(est << 16);
#endif
     status = cipher_set_iv(stream->rtp_cipher, &iv);

   } else {  
     v128_t iv;

       
#ifdef NO_64BIT_MATH
     iv.v32[0] = 0;
     iv.v32[1] = 0;
#else
     iv.v64[0] = 0;
#endif
     iv.v64[1] = be64_to_cpu(est);
     status = cipher_set_iv(stream->rtp_cipher, &iv);
   }
   if (status)
     return err_status_cipher_fail;

   
#ifdef NO_64BIT_MATH
   est = be64_to_cpu(make64((high32(est) << 16) |
						 (low32(est) >> 16),
						 low32(est) << 16));
#else
   est = be64_to_cpu(est << 16);
#endif
   
   



   if (auth_start) {
     
    prefix_len = auth_get_prefix_length(stream->rtp_auth);    
    if (prefix_len) {
      status = cipher_output(stream->rtp_cipher, auth_tag, prefix_len);
      if (status)
	return err_status_cipher_fail;
      debug_print(mod_srtp, "keystream prefix: %s", 
		  octet_string_hex_string(auth_tag, prefix_len));
    }
  }

  
  if (enc_start) {
    status = cipher_encrypt(stream->rtp_cipher, 
			    (uint8_t *)enc_start, &enc_octet_len);
    if (status)
      return err_status_cipher_fail;
  }

  



  if (auth_start) {        

    
    status = auth_start(stream->rtp_auth);
    if (status) return status;

    
    status = auth_update(stream->rtp_auth, 
			 (uint8_t *)auth_start, *pkt_octet_len);
    if (status) return status;
    
    
    debug_print(mod_srtp, "estimated packet index: %016llx", est);
    status = auth_compute(stream->rtp_auth, (uint8_t *)&est, 4, auth_tag); 
    debug_print(mod_srtp, "srtp auth tag:    %s", 
		octet_string_hex_string(auth_tag, tag_len));
    if (status)
      return err_status_auth_fail;   

  }

  if (auth_tag) {

    
    *pkt_octet_len += tag_len;
  }

  return err_status_ok;  
}


err_status_t
srtp_unprotect(srtp_ctx_t *ctx, void *srtp_hdr, int *pkt_octet_len) {
  srtp_hdr_t *hdr = (srtp_hdr_t *)srtp_hdr;
  uint32_t *enc_start;      
  uint32_t *auth_start;     
  unsigned enc_octet_len = 0;
  uint8_t *auth_tag = NULL; 
  xtd_seq_num_t est;        
  int delta;                
  v128_t iv;
  err_status_t status;
  srtp_stream_ctx_t *stream;
  uint8_t tmp_tag[SRTP_MAX_TAG_LEN];
  int tag_len, prefix_len;

  debug_print(mod_srtp, "function srtp_unprotect", NULL);

  

  
  if (*pkt_octet_len < octets_in_rtp_header)
    return err_status_bad_param;

  






  stream = srtp_get_stream(ctx, hdr->ssrc);
  if (stream == NULL) {
    if (ctx->stream_template != NULL) {
      stream = ctx->stream_template;
      debug_print(mod_srtp, "using provisional stream (SSRC: 0x%08x)",
		  hdr->ssrc);
      
      



#ifdef NO_64BIT_MATH
      est = (xtd_seq_num_t) make64(0,ntohs(hdr->seq));
      delta = low32(est);
#else
      est = (xtd_seq_num_t) ntohs(hdr->seq);
      delta = (int)est;
#endif
    } else {
      
      



      return err_status_no_ctx;
    }
  } else {
  
    
    delta = rdbx_estimate_index(&stream->rtp_rdbx, &est, ntohs(hdr->seq));
    
    
    status = rdbx_check(&stream->rtp_rdbx, delta);
    if (status)
      return status;
  }

#ifdef NO_64BIT_MATH
  debug_print2(mod_srtp, "estimated u_packet index: %08x%08x", high32(est),low32(est));
#else
  debug_print(mod_srtp, "estimated u_packet index: %016llx", est);
#endif

  
  tag_len = auth_get_tag_length(stream->rtp_auth); 

  



  if (stream->rtp_cipher->type->id == AES_ICM) {

    
    iv.v32[0] = 0;
    iv.v32[1] = hdr->ssrc;  
#ifdef NO_64BIT_MATH
    iv.v64[1] = be64_to_cpu(make64((high32(est) << 16) | (low32(est) >> 16),
			         low32(est) << 16));
#else
    iv.v64[1] = be64_to_cpu(est << 16);
#endif
    status = cipher_set_iv(stream->rtp_cipher, &iv);
  } else {  
    
      
#ifdef NO_64BIT_MATH
    iv.v32[0] = 0;
    iv.v32[1] = 0;
#else
    iv.v64[0] = 0;
#endif
    iv.v64[1] = be64_to_cpu(est);
    status = cipher_set_iv(stream->rtp_cipher, &iv);
  }
  if (status)
    return err_status_cipher_fail;

  
#ifdef NO_64BIT_MATH
  est = be64_to_cpu(make64((high32(est) << 16) |
					    (low32(est) >> 16),
					    low32(est) << 16));
#else
  est = be64_to_cpu(est << 16);
#endif

  







  if (stream->rtp_services & sec_serv_conf) {
    enc_start = (uint32_t *)hdr + uint32s_in_rtp_header + hdr->cc;  
    if (hdr->x == 1) {
      srtp_hdr_xtnd_t *xtn_hdr = (srtp_hdr_xtnd_t *)enc_start;
      enc_start += (ntohs(xtn_hdr->length) + 1);
    }  
    enc_octet_len = (uint32_t)(*pkt_octet_len - tag_len 
			       - ((enc_start - (uint32_t *)hdr) << 2));
  } else {
    enc_start = NULL;
  }

  




  if (stream->rtp_services & sec_serv_auth) {
    auth_start = (uint32_t *)hdr;
    auth_tag = (uint8_t *)hdr + *pkt_octet_len - tag_len;
  } else {
    auth_start = NULL;
    auth_tag = NULL;
  } 

  



  if (auth_start) {        

    





  
    if (stream->rtp_auth->prefix_len != 0) {
      
      prefix_len = auth_get_prefix_length(stream->rtp_auth);    
      status = cipher_output(stream->rtp_cipher, tmp_tag, prefix_len);
      debug_print(mod_srtp, "keystream prefix: %s", 
		  octet_string_hex_string(tmp_tag, prefix_len));
      if (status)
	return err_status_cipher_fail;
    } 

    
    status = auth_start(stream->rtp_auth);
    if (status) return status;
 
    
    status = auth_update(stream->rtp_auth, (uint8_t *)auth_start,  
			 *pkt_octet_len - tag_len);

    
    status = auth_compute(stream->rtp_auth, (uint8_t *)&est, 4, tmp_tag);  

    debug_print(mod_srtp, "computed auth tag:    %s", 
		octet_string_hex_string(tmp_tag, tag_len));
    debug_print(mod_srtp, "packet auth tag:      %s", 
		octet_string_hex_string(auth_tag, tag_len));
    if (status)
      return err_status_auth_fail;   

    if (octet_string_is_eq(tmp_tag, auth_tag, tag_len))
      return err_status_auth_fail;
  }

  




  switch(key_limit_update(stream->limit)) {
  case key_event_normal:
    break;
  case key_event_soft_limit: 
    srtp_handle_event(ctx, stream, event_key_soft_limit);
    break; 
  case key_event_hard_limit:
    srtp_handle_event(ctx, stream, event_key_hard_limit);
    return err_status_key_expired;
  default:
    break;
  }

  
  if (enc_start) {
    status = cipher_decrypt(stream->rtp_cipher, 
			    (uint8_t *)enc_start, &enc_octet_len);
    if (status)
      return err_status_cipher_fail;
  }

  









  if (stream->direction != dir_srtp_receiver) {
    if (stream->direction == dir_unknown) {
      stream->direction = dir_srtp_receiver;
    } else {
      srtp_handle_event(ctx, stream, event_ssrc_collision);
    }
  }

  




  if (stream == ctx->stream_template) {  
    srtp_stream_ctx_t *new_stream;

    






    status = srtp_stream_clone(ctx->stream_template, hdr->ssrc, &new_stream); 
    if (status)
      return status;
    
    
    new_stream->next = ctx->stream_list;
    ctx->stream_list = new_stream;
    
    
    stream = new_stream;
  }
  
  



  rdbx_add_index(&stream->rtp_rdbx, delta);

  
  *pkt_octet_len -= tag_len;

  return err_status_ok;  
}

err_status_t
srtp_init() {
  err_status_t status;

  
  status = crypto_kernel_init();
  if (status) 
    return status;

  
  status = crypto_kernel_load_debug_module(&mod_srtp);
  if (status)
    return status;

  return err_status_ok;
}

err_status_t
srtp_shutdown() {
  err_status_t status;

  
  status = crypto_kernel_shutdown();
  if (status) 
    return status;

  

  return err_status_ok;
}






#if 0







int
srtp_get_trailer_length(const srtp_stream_t s) {
  return auth_get_tag_length(s->rtp_auth);
}

#endif








srtp_stream_ctx_t *
srtp_get_stream(srtp_t srtp, uint32_t ssrc) {
  srtp_stream_ctx_t *stream;

  
  stream = srtp->stream_list;
  while (stream != NULL) {
    if (stream->ssrc == ssrc)
      return stream;
    stream = stream->next;
  }
  
  
  return NULL;
}

err_status_t
srtp_dealloc(srtp_t session) {
  srtp_stream_ctx_t *stream;
  err_status_t status;

  





  
  stream = session->stream_list;
  while (stream != NULL) {
    srtp_stream_t next = stream->next;
    status = srtp_stream_dealloc(session, stream);
    if (status)
      return status;
    stream = next;
  }
  
  
  if (session->stream_template != NULL) {
    status = auth_dealloc(session->stream_template->rtcp_auth); 
    if (status) 
      return status; 
    status = cipher_dealloc(session->stream_template->rtcp_cipher); 
    if (status) 
      return status; 
    crypto_free(session->stream_template->limit);
    status = cipher_dealloc(session->stream_template->rtp_cipher); 
    if (status) 
      return status; 
    status = auth_dealloc(session->stream_template->rtp_auth);
    if (status)
      return status;
    status = rdbx_dealloc(&session->stream_template->rtp_rdbx);
    if (status)
      return status;
    crypto_free(session->stream_template);
  }

  
  crypto_free(session);

  return err_status_ok;
}


err_status_t
srtp_add_stream(srtp_t session, 
		const srtp_policy_t *policy)  {
  err_status_t status;
  srtp_stream_t tmp;

  
  if ((session == NULL) || (policy == NULL) || (policy->key == NULL))
    return err_status_bad_param;

  
  status = srtp_stream_alloc(&tmp, policy);
  if (status) {
    return status;
  }
  
  
  status = srtp_stream_init(tmp, policy);
  if (status) {
    crypto_free(tmp);
    return status;
  }
  
  







  switch (policy->ssrc.type) {
  case (ssrc_any_outbound):
    if (session->stream_template) {
      return err_status_bad_param;
    }
    session->stream_template = tmp;
    session->stream_template->direction = dir_srtp_sender;
    break;
  case (ssrc_any_inbound):
    if (session->stream_template) {
      return err_status_bad_param;
    }
    session->stream_template = tmp;
    session->stream_template->direction = dir_srtp_receiver;
    break;
  case (ssrc_specific):
    tmp->next = session->stream_list;
    session->stream_list = tmp;
    break;
  case (ssrc_undefined):
  default:
    crypto_free(tmp);
    return err_status_bad_param;
  }
    
  return err_status_ok;
}


err_status_t
srtp_create(srtp_t *session,                
	    const srtp_policy_t *policy) { 
  err_status_t stat;
  srtp_ctx_t *ctx;

  
  if (session == NULL)
    return err_status_bad_param;

  
  ctx = (srtp_ctx_t *) crypto_alloc(sizeof(srtp_ctx_t));
  if (ctx == NULL)
    return err_status_alloc_fail;
  *session = ctx;

  



  ctx->stream_template = NULL;
  ctx->stream_list = NULL;
  while (policy != NULL) {    

    stat = srtp_add_stream(ctx, policy);
    if (stat) {
      
      srtp_dealloc(*session);
      return stat;
    }    

    
    policy = policy->next;
  }

  return err_status_ok;
}


err_status_t
srtp_remove_stream(srtp_t session, uint32_t ssrc) {
  srtp_stream_ctx_t *stream, *last_stream;
  err_status_t status;

  
  if (session == NULL)
    return err_status_bad_param;
  
  
  last_stream = stream = session->stream_list;
  while ((stream != NULL) && (ssrc != stream->ssrc)) {
    last_stream = stream;
    stream = stream->next;
  }
  if (stream == NULL)
    return err_status_no_ctx;

  
  if (last_stream == stream)
    
    session->stream_list = stream->next;
  else
    last_stream->next = stream->next;

  
  status = srtp_stream_dealloc(session, stream);
  if (status)
    return status;

  return err_status_ok;
}
















void
crypto_policy_set_rtp_default(crypto_policy_t *p) {

  p->cipher_type     = AES_ICM;           
  p->cipher_key_len  = 30;                
  p->auth_type       = HMAC_SHA1;             
  p->auth_key_len    = 20;                
  p->auth_tag_len    = 10;                
  p->sec_serv        = sec_serv_conf_and_auth;
  
}

void
crypto_policy_set_rtcp_default(crypto_policy_t *p) {

  p->cipher_type     = AES_ICM;           
  p->cipher_key_len  = 30;                 
  p->auth_type       = HMAC_SHA1;             
  p->auth_key_len    = 20;                 
  p->auth_tag_len    = 10;                 
  p->sec_serv        = sec_serv_conf_and_auth;
  
}

void
crypto_policy_set_aes_cm_128_hmac_sha1_32(crypto_policy_t *p) {

  





  p->cipher_type     = AES_ICM;           
  p->cipher_key_len  = 30;                
  p->auth_type       = HMAC_SHA1;             
  p->auth_key_len    = 20;                
  p->auth_tag_len    = 4;                 
  p->sec_serv        = sec_serv_conf_and_auth;
  
}


void
crypto_policy_set_aes_cm_128_null_auth(crypto_policy_t *p) {

  





  p->cipher_type     = AES_ICM;           
  p->cipher_key_len  = 30;                
  p->auth_type       = NULL_AUTH;             
  p->auth_key_len    = 0; 
  p->auth_tag_len    = 0; 
  p->sec_serv        = sec_serv_conf;
  
}


void
crypto_policy_set_null_cipher_hmac_sha1_80(crypto_policy_t *p) {

  



  p->cipher_type     = NULL_CIPHER;           
  p->cipher_key_len  = 0;
  p->auth_type       = HMAC_SHA1;             
  p->auth_key_len    = 20; 
  p->auth_tag_len    = 10; 
  p->sec_serv        = sec_serv_auth;
  
}


void
crypto_policy_set_aes_cm_256_hmac_sha1_80(crypto_policy_t *p) {

  



  p->cipher_type     = AES_ICM;           
  p->cipher_key_len  = 46;
  p->auth_type       = HMAC_SHA1;             
  p->auth_key_len    = 20;                
  p->auth_tag_len    = 10;                
  p->sec_serv        = sec_serv_conf_and_auth;
}


void
crypto_policy_set_aes_cm_256_hmac_sha1_32(crypto_policy_t *p) {

  





  p->cipher_type     = AES_ICM;           
  p->cipher_key_len  = 46;
  p->auth_type       = HMAC_SHA1;             
  p->auth_key_len    = 20;                
  p->auth_tag_len    = 4;                 
  p->sec_serv        = sec_serv_conf_and_auth;
}






err_status_t 
srtp_protect_rtcp(srtp_t ctx, void *rtcp_hdr, int *pkt_octet_len) {
  srtcp_hdr_t *hdr = (srtcp_hdr_t *)rtcp_hdr;
  uint32_t *enc_start;      
  uint32_t *auth_start;     
  uint32_t *trailer;        
  unsigned enc_octet_len = 0;
  uint8_t *auth_tag = NULL; 
  err_status_t status;   
  int tag_len;
  srtp_stream_ctx_t *stream;
  int prefix_len;
  uint32_t seq_num;

  
  






  stream = srtp_get_stream(ctx, hdr->ssrc);
  if (stream == NULL) {
    if (ctx->stream_template != NULL) {
      srtp_stream_ctx_t *new_stream;
      
      
      status = srtp_stream_clone(ctx->stream_template,
				 hdr->ssrc, &new_stream); 
      if (status)
	return status;
      
      
      new_stream->next = ctx->stream_list;
      ctx->stream_list = new_stream;
      
      
      stream = new_stream;
    } else {
      
      return err_status_no_ctx;
    } 
  }
  
  





  if (stream->direction != dir_srtp_sender) {
    if (stream->direction == dir_unknown) {
      stream->direction = dir_srtp_sender;
    } else {
      srtp_handle_event(ctx, stream, event_ssrc_collision);
    }
  }  

  
  tag_len = auth_get_tag_length(stream->rtcp_auth); 

  



  enc_start = (uint32_t *)hdr + uint32s_in_rtcp_header;  
  enc_octet_len = *pkt_octet_len - octets_in_rtcp_header;

  
  

  

  trailer = (uint32_t *) ((char *)enc_start + enc_octet_len);

  if (stream->rtcp_services & sec_serv_conf) {
    *trailer = htonl(SRTCP_E_BIT);         
  } else {
    enc_start = NULL;
    enc_octet_len = 0;
	
    *trailer = 0x00000000;         
  }

  



  
  auth_start = (uint32_t *)hdr;
  auth_tag = (uint8_t *)hdr + *pkt_octet_len + sizeof(srtcp_trailer_t); 

  
  ekt_write_data(stream->ekt, auth_tag, tag_len, pkt_octet_len, 
		 rdbx_get_packet_index(&stream->rtp_rdbx));

  



  status = rdb_increment(&stream->rtcp_rdb);
  if (status)
    return status;
  seq_num = rdb_get_value(&stream->rtcp_rdb);
  *trailer |= htonl(seq_num);
  debug_print(mod_srtp, "srtcp index: %x", seq_num);

  


  if (stream->rtcp_cipher->type->id == AES_ICM) {
    v128_t iv;
    
    iv.v32[0] = 0;
    iv.v32[1] = hdr->ssrc;  
    iv.v32[2] = htonl(seq_num >> 16);
    iv.v32[3] = htonl(seq_num << 16);
    status = cipher_set_iv(stream->rtcp_cipher, &iv);

  } else {  
    v128_t iv;
    
      
    iv.v32[0] = 0;
    iv.v32[1] = 0;
    iv.v32[2] = 0;
    iv.v32[3] = htonl(seq_num);
    status = cipher_set_iv(stream->rtcp_cipher, &iv);
  }
  if (status)
    return err_status_cipher_fail;

  



  
  
  if (auth_start) {

    
    prefix_len = auth_get_prefix_length(stream->rtcp_auth);    
    status = cipher_output(stream->rtcp_cipher, auth_tag, prefix_len);

    debug_print(mod_srtp, "keystream prefix: %s", 
		octet_string_hex_string(auth_tag, prefix_len));

    if (status)
      return err_status_cipher_fail;
  }

  
  if (enc_start) {
    status = cipher_encrypt(stream->rtcp_cipher, 
			    (uint8_t *)enc_start, &enc_octet_len);
    if (status)
      return err_status_cipher_fail;
  }

  
  auth_start(stream->rtcp_auth);

  



  status = auth_compute(stream->rtcp_auth, 
			(uint8_t *)auth_start, 
			(*pkt_octet_len) + sizeof(srtcp_trailer_t), 
			auth_tag);
  debug_print(mod_srtp, "srtcp auth tag:    %s", 
	      octet_string_hex_string(auth_tag, tag_len));
  if (status)
    return err_status_auth_fail;   
    
  
  *pkt_octet_len += (tag_len + sizeof(srtcp_trailer_t));
    
  return err_status_ok;  
}


err_status_t 
srtp_unprotect_rtcp(srtp_t ctx, void *srtcp_hdr, int *pkt_octet_len) {
  srtcp_hdr_t *hdr = (srtcp_hdr_t *)srtcp_hdr;
  uint32_t *enc_start;      
  uint32_t *auth_start;     
  uint32_t *trailer;        
  unsigned enc_octet_len = 0;
  uint8_t *auth_tag = NULL; 
  uint8_t tmp_tag[SRTP_MAX_TAG_LEN];
  uint8_t tag_copy[SRTP_MAX_TAG_LEN];
  err_status_t status;   
  unsigned auth_len;
  int tag_len;
  srtp_stream_ctx_t *stream;
  int prefix_len;
  uint32_t seq_num;

  
  






  stream = srtp_get_stream(ctx, hdr->ssrc);
  if (stream == NULL) {
    if (ctx->stream_template != NULL) {
      stream = ctx->stream_template;

      








 
      if (stream->ekt != NULL) {
	status = srtp_stream_init_from_ekt(stream, srtcp_hdr, *pkt_octet_len);
	if (status)
	  return status;
      }

      debug_print(mod_srtp, "srtcp using provisional stream (SSRC: 0x%08x)", 
		  hdr->ssrc);
    } else {
      
      return err_status_no_ctx;
    } 
  }
  
  
  tag_len = auth_get_tag_length(stream->rtcp_auth); 

  


  enc_octet_len = *pkt_octet_len - 
                  (octets_in_rtcp_header + tag_len + sizeof(srtcp_trailer_t));
  

  

  
  



  trailer = (uint32_t *) ((char *) hdr +
		     *pkt_octet_len -(tag_len + sizeof(srtcp_trailer_t)));
  if (*((unsigned char *) trailer) & SRTCP_E_BYTE_BIT) {
    enc_start = (uint32_t *)hdr + uint32s_in_rtcp_header;  
  } else {
    enc_octet_len = 0;
    enc_start = NULL; 
  }

  



  auth_start = (uint32_t *)hdr;
  auth_len = *pkt_octet_len - tag_len;
  auth_tag = (uint8_t *)hdr + auth_len;

  






  if (stream->ekt) {
    auth_tag -= ekt_octets_after_base_tag(stream->ekt);
    memcpy(tag_copy, auth_tag, tag_len);
    octet_string_set_to_zero(auth_tag, tag_len);
    auth_tag = tag_copy;
    auth_len += tag_len;
  }

  


  
  seq_num = ntohl(*trailer) & SRTCP_INDEX_MASK;
  debug_print(mod_srtp, "srtcp index: %x", seq_num);
  status = rdb_check(&stream->rtcp_rdb, seq_num);
  if (status)
    return status;

  


  if (stream->rtcp_cipher->type->id == AES_ICM) {
    v128_t iv;

    iv.v32[0] = 0;
    iv.v32[1] = hdr->ssrc; 
    iv.v32[2] = htonl(seq_num >> 16);
    iv.v32[3] = htonl(seq_num << 16);
    status = cipher_set_iv(stream->rtcp_cipher, &iv);

  } else {  
    v128_t iv;
    
      
    iv.v32[0] = 0;
    iv.v32[1] = 0;
    iv.v32[2] = 0;
    iv.v32[3] = htonl(seq_num);
    status = cipher_set_iv(stream->rtcp_cipher, &iv);

  }
  if (status)
    return err_status_cipher_fail;

  
  auth_start(stream->rtcp_auth);

  
  status = auth_compute(stream->rtcp_auth, (uint8_t *)auth_start,  
			auth_len, tmp_tag);
  debug_print(mod_srtp, "srtcp computed tag:       %s", 
	      octet_string_hex_string(tmp_tag, tag_len));
  if (status)
    return err_status_auth_fail;   
  
  
  debug_print(mod_srtp, "srtcp tag from packet:    %s", 
	      octet_string_hex_string(auth_tag, tag_len));  
  if (octet_string_is_eq(tmp_tag, auth_tag, tag_len))
    return err_status_auth_fail;

  



  prefix_len = auth_get_prefix_length(stream->rtcp_auth);    
  if (prefix_len) {
    status = cipher_output(stream->rtcp_cipher, auth_tag, prefix_len);
    debug_print(mod_srtp, "keystream prefix: %s", 
		octet_string_hex_string(auth_tag, prefix_len));
    if (status)
      return err_status_cipher_fail;
  }

  
  if (enc_start) {
    status = cipher_decrypt(stream->rtcp_cipher, 
			    (uint8_t *)enc_start, &enc_octet_len);
    if (status)
      return err_status_cipher_fail;
  }

  
  *pkt_octet_len -= (tag_len + sizeof(srtcp_trailer_t));

  



  *pkt_octet_len -= ekt_octets_after_base_tag(stream->ekt);

  









  if (stream->direction != dir_srtp_receiver) {
    if (stream->direction == dir_unknown) {
      stream->direction = dir_srtp_receiver;
    } else {
      srtp_handle_event(ctx, stream, event_ssrc_collision);
    }
  }

  




  if (stream == ctx->stream_template) {  
    srtp_stream_ctx_t *new_stream;

    






    status = srtp_stream_clone(ctx->stream_template, hdr->ssrc, &new_stream); 
    if (status)
      return status;
    
    
    new_stream->next = ctx->stream_list;
    ctx->stream_list = new_stream;
    
    
    stream = new_stream;
  }

  
  rdb_add_index(&stream->rtcp_rdb, seq_num);
    
    
  return err_status_ok;  
}







err_status_t
crypto_policy_set_from_profile_for_rtp(crypto_policy_t *policy, 
				       srtp_profile_t profile) {

  
  switch(profile) {
  case srtp_profile_aes128_cm_sha1_80:
    crypto_policy_set_aes_cm_128_hmac_sha1_80(policy);
    crypto_policy_set_aes_cm_128_hmac_sha1_80(policy);
    break;
  case srtp_profile_aes128_cm_sha1_32:
    crypto_policy_set_aes_cm_128_hmac_sha1_32(policy);
    crypto_policy_set_aes_cm_128_hmac_sha1_80(policy);
    break;
  case srtp_profile_null_sha1_80:
    crypto_policy_set_null_cipher_hmac_sha1_80(policy);
    crypto_policy_set_null_cipher_hmac_sha1_80(policy);
    break;
  case srtp_profile_aes256_cm_sha1_80:
    crypto_policy_set_aes_cm_256_hmac_sha1_80(policy);
    crypto_policy_set_aes_cm_256_hmac_sha1_80(policy);
    break;
  case srtp_profile_aes256_cm_sha1_32:
    crypto_policy_set_aes_cm_256_hmac_sha1_32(policy);
    crypto_policy_set_aes_cm_256_hmac_sha1_80(policy);
    break;
    
  case srtp_profile_null_sha1_32:
  default:
    return err_status_bad_param;
  }

  return err_status_ok;
}

err_status_t
crypto_policy_set_from_profile_for_rtcp(crypto_policy_t *policy, 
					srtp_profile_t profile) {

  
  switch(profile) {
  case srtp_profile_aes128_cm_sha1_80:
    crypto_policy_set_aes_cm_128_hmac_sha1_80(policy);
    break;
  case srtp_profile_aes128_cm_sha1_32:
    crypto_policy_set_aes_cm_128_hmac_sha1_80(policy);
    break;
  case srtp_profile_null_sha1_80:
    crypto_policy_set_null_cipher_hmac_sha1_80(policy);
    break;
  case srtp_profile_aes256_cm_sha1_80:
    crypto_policy_set_aes_cm_256_hmac_sha1_80(policy);
    break;
  case srtp_profile_aes256_cm_sha1_32:
    crypto_policy_set_aes_cm_256_hmac_sha1_80(policy);
    break;
    
  case srtp_profile_null_sha1_32:
  default:
    return err_status_bad_param;
  }

  return err_status_ok;
}

void
append_salt_to_key(uint8_t *key, unsigned int bytes_in_key,
		   uint8_t *salt, unsigned int bytes_in_salt) {

  memcpy(key + bytes_in_key, salt, bytes_in_salt);

}

unsigned int
srtp_profile_get_master_key_length(srtp_profile_t profile) {

  switch(profile) {
  case srtp_profile_aes128_cm_sha1_80:
    return 16;
    break;
  case srtp_profile_aes128_cm_sha1_32:
    return 16;
    break;
  case srtp_profile_null_sha1_80:
    return 16;
    break;
  case srtp_profile_aes256_cm_sha1_80:
    return 32;
    break;
  case srtp_profile_aes256_cm_sha1_32:
    return 32;
    break;
    
  case srtp_profile_null_sha1_32:
  default:
    return 0;  
  }
}

unsigned int
srtp_profile_get_master_salt_length(srtp_profile_t profile) {

  switch(profile) {
  case srtp_profile_aes128_cm_sha1_80:
    return 14;
    break;
  case srtp_profile_aes128_cm_sha1_32:
    return 14;
    break;
  case srtp_profile_null_sha1_80:
    return 14;
    break;
  case srtp_profile_aes256_cm_sha1_80:
    return 14;
    break;
  case srtp_profile_aes256_cm_sha1_32:
    return 14;
    break;
    
  case srtp_profile_null_sha1_32:
  default:
    return 0;  
  }
}
