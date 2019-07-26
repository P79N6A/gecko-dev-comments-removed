




























































#ifndef EKT_H
#define EKT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "srtp_priv.h"

#define EKT_CIPHER_DEFAULT           1
#define EKT_CIPHER_AES_128_ECB       1
#define EKT_CIPHER_AES_192_KEY_WRAP  2
#define EKT_CIPHER_AES_256_KEY_WRAP  3

typedef uint16_t ekt_spi_t;


unsigned
ekt_octets_after_base_tag(ekt_stream_t ekt);









typedef struct ekt_policy_ctx_t {
  ekt_spi_t  spi;     
  uint8_t    ekt_cipher_type;
  uint8_t   *ekt_key;
  struct ekt_policy_ctx_t *next_ekt_policy;
} ekt_policy_ctx_t;







typedef struct ekt_data_t {
  ekt_spi_t spi;
  uint8_t ekt_cipher_type;
  aes_expanded_key_t ekt_enc_key;
  aes_expanded_key_t ekt_dec_key;
  struct ekt_data_t *next_ekt_data;
} ekt_data_t;








typedef struct ekt_stream_ctx_t {
  ekt_data_t *data;    
  uint16_t    isn;     
  uint8_t     encrypted_master_key[SRTP_MAX_KEY_LEN];
} ekt_stream_ctx_t;



err_status_t 
ekt_alloc(ekt_stream_t *stream_data, ekt_policy_t policy);

err_status_t
ekt_stream_init(ekt_stream_t e, 
		ekt_spi_t spi,
		void *ekt_key,
		unsigned ekt_cipher_type);

err_status_t
ekt_stream_init_from_policy(ekt_stream_t e, ekt_policy_t p);
  


err_status_t
srtp_stream_init_from_ekt(srtp_stream_t stream,			  
			  const void *srtcp_hdr,
			  unsigned pkt_octet_len);
		

void
ekt_write_data(ekt_stream_t ekt,
	       uint8_t *base_tag, 
	       unsigned base_tag_len, 
	       int *packet_len,
	       xtd_seq_num_t pkt_index);		










err_status_t
ekt_tag_verification_preproces(uint8_t *pkt_tag, 
			       uint8_t *pkt_tag_copy, 
			       unsigned tag_len);

err_status_t
ekt_tag_verification_postproces(uint8_t *pkt_tag,
				uint8_t *pkt_tag_copy,
				unsigned tag_len);















err_status_t
srtp_stream_srtcp_auth_tag_generation_preprocess(const srtp_stream_t *s,
						 uint8_t *pkt_tag,
						 unsigned pkt_octet_len);



err_status_t
srtcp_auth_tag_generation_postprocess(void);


#ifdef __cplusplus
}
#endif

#endif
