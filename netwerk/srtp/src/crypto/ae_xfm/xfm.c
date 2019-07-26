











































#include "cryptoalg.h"
#include "aes_cbc.h"
#include "hmac.h"
#include "crypto_kernel.h"   

#define KEY_LEN     16
#define ENC_KEY_LEN 16
#define MAC_KEY_LEN 16
#define IV_LEN      16
#define TAG_LEN     12
#define MAX_EXPAND  27

err_status_t
aes_128_cbc_hmac_sha1_96_func(void *key,            
			      void *clear,          
			      unsigned clear_len,       
			      void *iv,             
			      void *opaque,         
			      unsigned *opaque_len, 
			      void *auth_tag) {
  aes_cbc_ctx_t aes_ctx;
  hmac_ctx_t hmac_ctx;
  unsigned char enc_key[ENC_KEY_LEN];
  unsigned char mac_key[MAC_KEY_LEN];
  err_status_t status;

  
  if ((iv == NULL) && (opaque == NULL) && (opaque_len == NULL)) {
      
      

  } else if ((iv == NULL) || (opaque == NULL) || (opaque_len == NULL)) {
    
    



    return err_status_fail;

  } else {

    
    status = hmac_init(&hmac_ctx, key, KEY_LEN);
    if (status) return status;
    status = hmac_compute(&hmac_ctx, "ENC", 3, ENC_KEY_LEN, enc_key);
    if (status) return status;

    status = hmac_init(&hmac_ctx, key, KEY_LEN);
    if (status) return status;
    status = hmac_compute(&hmac_ctx, "MAC", 3, MAC_KEY_LEN, mac_key);
    if (status) return status;


    

    
    status = aes_cbc_context_init(&aes_ctx, key, ENC_KEY_LEN, direction_encrypt);
    if (status) return status;

    
    status = crypto_get_random(iv, IV_LEN);  
    if (status) return status; 
    status = aes_cbc_set_iv(&aes_ctx, iv);
    
    
    status = aes_cbc_nist_encrypt(&aes_ctx, opaque, opaque_len);
    if (status) return status;

    
    status = hmac_init(&hmac_ctx, mac_key, MAC_KEY_LEN);
    if (status) return status;

    status = hmac_start(&hmac_ctx);
    if (status) return status;

    status = hmac_update(&hmac_ctx, clear, clear_len);
    if (status) return status;

    status = hmac_compute(&hmac_ctx, opaque, *opaque_len, TAG_LEN, auth_tag);
    if (status) return status;

  }

  return err_status_ok;
}

err_status_t
aes_128_cbc_hmac_sha1_96_inv(void *key,            
			     void *clear,          
			     unsigned clear_len,       
			     void *iv,             
			     void *opaque,         
			     unsigned *opaque_len, 
			     void *auth_tag) {
  aes_cbc_ctx_t aes_ctx;
  hmac_ctx_t hmac_ctx;
  unsigned char enc_key[ENC_KEY_LEN];
  unsigned char mac_key[MAC_KEY_LEN];
  unsigned char tmp_tag[TAG_LEN];
  unsigned char *tag = auth_tag;
  err_status_t status;
  int i;
  
  
  if ((iv == NULL) && (opaque == NULL) && (opaque_len == NULL)) {
      
      

  } else if ((iv == NULL) || (opaque == NULL) || (opaque_len == NULL)) {
    
    



    return err_status_fail;

  } else {

    
    status = hmac_init(&hmac_ctx, key, KEY_LEN);
    if (status) return status;
    status = hmac_compute(&hmac_ctx, "ENC", 3, ENC_KEY_LEN, enc_key);
    if (status) return status;

    status = hmac_init(&hmac_ctx, key, KEY_LEN);
    if (status) return status;
    status = hmac_compute(&hmac_ctx, "MAC", 3, MAC_KEY_LEN, mac_key);
    if (status) return status;

    

    
    status = aes_cbc_context_init(&aes_ctx, key, ENC_KEY_LEN, direction_decrypt);
    if (status) return status;

    
    status = rand_source_get_octet_string(iv, IV_LEN);  
    if (status) return status; 
    status = aes_cbc_set_iv(&aes_ctx, iv);
    
    
    status = aes_cbc_nist_decrypt(&aes_ctx, opaque, opaque_len);
    if (status) return status;

    
    status = hmac_init(&hmac_ctx, mac_key, MAC_KEY_LEN);
    if (status) return status;

    status = hmac_start(&hmac_ctx);
    if (status) return status;

    status = hmac_update(&hmac_ctx, clear, clear_len);
    if (status) return status;

    status = hmac_compute(&hmac_ctx, opaque, *opaque_len, TAG_LEN, tmp_tag);
    if (status) return status;

    
    for (i=0; i < TAG_LEN; i++)
      if (tmp_tag[i] != tag[i]) 
	return err_status_auth_fail; 

  }

  return err_status_ok;
}


#define ENC 1

#define DEBUG 0

err_status_t
aes_128_cbc_hmac_sha1_96_enc(void *key,            
			     const void *clear,          
			     unsigned clear_len,       
			     void *iv,             
			     void *opaque,         
			     unsigned *opaque_len) {
  aes_cbc_ctx_t aes_ctx;
  hmac_ctx_t hmac_ctx;
  unsigned char enc_key[ENC_KEY_LEN];
  unsigned char mac_key[MAC_KEY_LEN];
  unsigned char *auth_tag;
  err_status_t status;

  
  if ((iv == NULL) && (opaque == NULL) && (opaque_len == NULL)) {
      
      

  } else if ((iv == NULL) || (opaque == NULL) || (opaque_len == NULL)) {
    
    



    return err_status_fail;

  } else {

#if DEBUG
    printf("ENC using key %s\n", octet_string_hex_string(key, KEY_LEN));
#endif

    
    status = hmac_init(&hmac_ctx, key, KEY_LEN);
    if (status) return status;
    status = hmac_compute(&hmac_ctx, "ENC", 3, ENC_KEY_LEN, enc_key);
    if (status) return status;

    status = hmac_init(&hmac_ctx, key, KEY_LEN);
    if (status) return status;
    status = hmac_compute(&hmac_ctx, "MAC", 3, MAC_KEY_LEN, mac_key);
    if (status) return status;


    

    
    status = aes_cbc_context_init(&aes_ctx, key, ENC_KEY_LEN, direction_encrypt);
    if (status) return status;

    
    status = rand_source_get_octet_string(iv, IV_LEN);  
    if (status) return status; 
    status = aes_cbc_set_iv(&aes_ctx, iv);
    if (status) return status;

#if DEBUG
    printf("plaintext len:  %d\n", *opaque_len);
    printf("iv:         %s\n", octet_string_hex_string(iv, IV_LEN));
    printf("plaintext:  %s\n", octet_string_hex_string(opaque, *opaque_len));
#endif

#if ENC    
    
    status = aes_cbc_nist_encrypt(&aes_ctx, opaque, opaque_len);
    if (status) return status;
#endif

#if DEBUG
    printf("ciphertext len: %d\n", *opaque_len);
    printf("ciphertext: %s\n", octet_string_hex_string(opaque, *opaque_len));
#endif

    




    status = hmac_init(&hmac_ctx, mac_key, MAC_KEY_LEN);
    if (status) return status;

    status = hmac_start(&hmac_ctx);
    if (status) return status;

    status = hmac_update(&hmac_ctx, clear, clear_len);
    if (status) return status;
#if DEBUG
    printf("hmac input: %s\n", 
	   octet_string_hex_string(clear, clear_len));
#endif
    auth_tag = (unsigned char *)opaque;
    auth_tag += *opaque_len;    
    status = hmac_compute(&hmac_ctx, opaque, *opaque_len, TAG_LEN, auth_tag);
    if (status) return status;
#if DEBUG
    printf("hmac input: %s\n", 
	   octet_string_hex_string(opaque, *opaque_len));
#endif
    
    *opaque_len += TAG_LEN;

#if DEBUG
    printf("prot data len:  %d\n", *opaque_len);
    printf("prot data: %s\n", octet_string_hex_string(opaque, *opaque_len));
#endif
  }

  return err_status_ok;
}

err_status_t
aes_128_cbc_hmac_sha1_96_dec(void *key,            
			     const void *clear,          
			     unsigned clear_len,       
			     void *iv,             
			     void *opaque,         
			     unsigned *opaque_len) {
  aes_cbc_ctx_t aes_ctx;
  hmac_ctx_t hmac_ctx;
  unsigned char enc_key[ENC_KEY_LEN];
  unsigned char mac_key[MAC_KEY_LEN];
  unsigned char tmp_tag[TAG_LEN];
  unsigned char *auth_tag;
  unsigned ciphertext_len;
  err_status_t status;
  int i;
  
  
  if ((iv == NULL) && (opaque == NULL) && (opaque_len == NULL)) {
      
      

  } else if ((iv == NULL) || (opaque == NULL) || (opaque_len == NULL)) {
    
    



    return err_status_fail;

  } else {
#if DEBUG
    printf("DEC using key %s\n", octet_string_hex_string(key, KEY_LEN));
#endif

    
    status = hmac_init(&hmac_ctx, key, KEY_LEN);
    if (status) return status;
    status = hmac_compute(&hmac_ctx, "ENC", 3, ENC_KEY_LEN, enc_key);
    if (status) return status;

    status = hmac_init(&hmac_ctx, key, KEY_LEN);
    if (status) return status;
    status = hmac_compute(&hmac_ctx, "MAC", 3, MAC_KEY_LEN, mac_key);
    if (status) return status;

#if DEBUG
    printf("prot data len:  %d\n", *opaque_len);
    printf("prot data: %s\n", octet_string_hex_string(opaque, *opaque_len));
#endif

    



    ciphertext_len = *opaque_len - TAG_LEN;

#if DEBUG
    printf("ciphertext len: %d\n", ciphertext_len);
#endif    
    

    



    status = hmac_init(&hmac_ctx, mac_key, MAC_KEY_LEN);
    if (status) return status;

    status = hmac_start(&hmac_ctx);
    if (status) return status;

    status = hmac_update(&hmac_ctx, clear, clear_len);
    if (status) return status;

#if DEBUG
    printf("hmac input: %s\n", 
	   octet_string_hex_string(clear, clear_len));
#endif

    status = hmac_compute(&hmac_ctx, opaque, ciphertext_len, TAG_LEN, tmp_tag);
    if (status) return status;

#if DEBUG
    printf("hmac input: %s\n", 
	   octet_string_hex_string(opaque, ciphertext_len));
#endif

    



    auth_tag = (unsigned char *)opaque;
    auth_tag += ciphertext_len;  
#if DEBUG
    printf("auth_tag: %s\n", octet_string_hex_string(auth_tag, TAG_LEN));
    printf("tmp_tag:  %s\n", octet_string_hex_string(tmp_tag, TAG_LEN));
#endif
    for (i=0; i < TAG_LEN; i++) {
      if (tmp_tag[i] != auth_tag[i]) 
	return err_status_auth_fail; 
    }

    
    *opaque_len -= TAG_LEN;

    
    status = aes_cbc_context_init(&aes_ctx, key, ENC_KEY_LEN, direction_decrypt);
    if (status) return status;
    status = aes_cbc_set_iv(&aes_ctx, iv);
    if (status) return status;

#if DEBUG
    printf("ciphertext: %s\n", octet_string_hex_string(opaque, *opaque_len));
    printf("iv:         %s\n", octet_string_hex_string(iv, IV_LEN));
#endif

#if ENC
    status = aes_cbc_nist_decrypt(&aes_ctx, opaque, &ciphertext_len);
    if (status) return status;
#endif

#if DEBUG
    printf("plaintext len:  %d\n", ciphertext_len);
    printf("plaintext:  %s\n", 
	   octet_string_hex_string(opaque, ciphertext_len));
#endif

    
    *opaque_len = ciphertext_len;
  }

  return err_status_ok;
}

cryptoalg_ctx_t cryptoalg_ctx = {
  aes_128_cbc_hmac_sha1_96_enc,
  aes_128_cbc_hmac_sha1_96_dec,
  KEY_LEN,
  IV_LEN,
  TAG_LEN,
  MAX_EXPAND,
};

cryptoalg_t cryptoalg = &cryptoalg_ctx;

#define NULL_TAG_LEN 12

err_status_t
null_enc(void *key,            
	 const void *clear,          
	 unsigned clear_len,       
	 void *iv,             
	 void *opaque,         
	 unsigned *opaque_len) {
  int i;
  unsigned char *auth_tag;
  unsigned char *init_vec = iv;

  
  if ((iv == NULL) && (opaque == NULL) && (opaque_len == NULL)) {
      
      

  } else if ((iv == NULL) || (opaque == NULL) || (opaque_len == NULL)) {
    
    



    return err_status_fail;

  } else {

#if DEBUG
    printf("NULL ENC using key %s\n", octet_string_hex_string(key, KEY_LEN));
    printf("NULL_TAG_LEN:  %d\n", NULL_TAG_LEN);
    printf("plaintext len:  %d\n", *opaque_len);
#endif
    for (i=0; i < IV_LEN; i++)
      init_vec[i] = i + (i * 16);
#if DEBUG
    printf("iv:                %s\n", 
	   octet_string_hex_string(iv, IV_LEN));
    printf("plaintext:         %s\n", 
	   octet_string_hex_string(opaque, *opaque_len));
#endif
    auth_tag = opaque;
    auth_tag += *opaque_len;
    for (i=0; i < NULL_TAG_LEN; i++)
      auth_tag[i] = i + (i * 16);
    *opaque_len += NULL_TAG_LEN;
#if DEBUG
    printf("protected data len: %d\n", *opaque_len);
    printf("protected data:    %s\n", 
	   octet_string_hex_string(opaque, *opaque_len));
#endif

  }

  return err_status_ok;
}

err_status_t
null_dec(void *key,            
	 const void *clear,          
	 unsigned clear_len,       
	 void *iv,             
	 void *opaque,         
	 unsigned *opaque_len) {
  unsigned char *auth_tag;
  
  
  if ((iv == NULL) && (opaque == NULL) && (opaque_len == NULL)) {
      
      

  } else if ((iv == NULL) || (opaque == NULL) || (opaque_len == NULL)) {
    
    



    return err_status_fail;

  } else {

#if DEBUG
    printf("NULL DEC using key %s\n", octet_string_hex_string(key, KEY_LEN));

    printf("protected data len: %d\n", *opaque_len);
    printf("protected data:    %s\n", 
	   octet_string_hex_string(opaque, *opaque_len));
#endif
    auth_tag = opaque;
    auth_tag += (*opaque_len - NULL_TAG_LEN);
#if DEBUG
    printf("iv:         %s\n", octet_string_hex_string(iv, IV_LEN));
#endif
    *opaque_len -= NULL_TAG_LEN;
#if DEBUG
    printf("plaintext len:  %d\n", *opaque_len);
    printf("plaintext:  %s\n", 
	   octet_string_hex_string(opaque, *opaque_len));
#endif
  }

  return err_status_ok;
}

cryptoalg_ctx_t null_cryptoalg_ctx = {
  null_enc,
  null_dec,
  KEY_LEN,
  IV_LEN,
  NULL_TAG_LEN,
  MAX_EXPAND,
};

cryptoalg_t null_cryptoalg = &null_cryptoalg_ctx;

int
cryptoalg_get_id(cryptoalg_t c) {
  if (c == cryptoalg)
    return 1;
  return 0;
}

cryptoalg_t 
cryptoalg_find_by_id(int id) {
  switch(id) {
  case 1:
    return cryptoalg;
  default:
    break;
  }
  return 0;
}
