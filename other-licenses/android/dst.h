











#ifndef DST_H
#define DST_H

#ifndef HAS_DST_KEY
typedef struct dst_key {
	char	*dk_key_name;   
	int	dk_key_size;    
	int	dk_proto;       
	int	dk_alg;         
	u_int32_t dk_flags;     
	u_int16_t dk_id;        
} DST_KEY;
#endif 




#define	dst_bsafe_init		__dst_bsafe_init
#define	dst_buffer_to_key	__dst_buffer_to_key
#define	dst_check_algorithm	__dst_check_algorithm
#define	dst_compare_keys	__dst_compare_keys
#define	dst_cylink_init		__dst_cylink_init
#define	dst_dnskey_to_key	__dst_dnskey_to_key
#define	dst_eay_dss_init	__dst_eay_dss_init
#define	dst_free_key		__dst_free_key
#define	dst_generate_key	__dst_generate_key
#define	dst_hmac_md5_init	__dst_hmac_md5_init
#define	dst_init		__dst_init
#define	dst_key_to_buffer	__dst_key_to_buffer
#define	dst_key_to_dnskey	__dst_key_to_dnskey
#define	dst_read_key		__dst_read_key
#define	dst_rsaref_init		__dst_rsaref_init
#define	dst_s_build_filename	__dst_s_build_filename
#define	dst_s_calculate_bits	__dst_s_calculate_bits
#define	dst_s_conv_bignum_b64_to_u8	__dst_s_conv_bignum_b64_to_u8
#define	dst_s_conv_bignum_u8_to_b64	__dst_s_conv_bignum_u8_to_b64
#define	dst_s_dns_key_id	__dst_s_dns_key_id
#define	dst_s_dump		__dst_s_dump
#define	dst_s_filename_length	__dst_s_filename_length
#define	dst_s_fopen		__dst_s_fopen
#define	dst_s_get_int16		__dst_s_get_int16
#define	dst_s_get_int32		__dst_s_get_int32
#define	dst_s_id_calc		__dst_s_id_calc
#define	dst_s_put_int16		__dst_s_put_int16
#define	dst_s_put_int32		__dst_s_put_int32
#define	dst_s_quick_random	__dst_s_quick_random
#define	dst_s_quick_random_set	__dst_s_quick_random_set
#define	dst_s_random		__dst_s_random
#define	dst_s_semi_random	__dst_s_semi_random
#define	dst_s_verify_str	__dst_s_verify_str
#define	dst_sig_size		__dst_sig_size
#define	dst_sign_data		__dst_sign_data
#define	dst_verify_data		__dst_verify_data
#define	dst_write_key		__dst_write_key




void     dst_init(void);
int      dst_check_algorithm(const int);

int dst_sign_data(const int,	 	
		  DST_KEY *,	 	
		  void **,	 	
		  const u_char *,	
		  const int,	 	
		  u_char *,	 	
		  const int);	 	

int dst_verify_data(const int,	 	
		    DST_KEY *,	 	
		    void **,	 	
		    const u_char *,	
		    const int,	 	
		    const u_char *,	
		    const int);	 	


DST_KEY *dst_read_key(const char *,	
		      const u_int16_t,	
		      const int,	
		      const int);	

int      dst_write_key(const DST_KEY *,	
		       const int); 	

DST_KEY *dst_dnskey_to_key(const char *,	
			   const u_char *,	
			   const int);		


int      dst_key_to_dnskey(const DST_KEY *,	
			   u_char *,		
			   const int);		


DST_KEY *dst_buffer_to_key(const char *,  	
			   const int,	  	
			   const int,	  	
			   const int,	  	
			   const u_char *, 	
			   const int);	  	


int     dst_key_to_buffer(DST_KEY *, u_char *, int);

DST_KEY *dst_generate_key(const char *,    	
			  const int,       	
			  const int,      	
			  const int,       	
			  const int,     	
			  const int);		

DST_KEY *dst_free_key(DST_KEY *);
int      dst_compare_keys(const DST_KEY *, const DST_KEY *);

int	dst_sig_size(DST_KEY *);



u_int16_t dst_s_dns_key_id(const u_char *, const int);
u_int16_t dst_s_id_calc(const u_char *, const int);


#define RAW_KEY_SIZE    8192        /* large enough to store any key */



#define SIG_MODE_INIT		1  /* initialize digest */
#define SIG_MODE_UPDATE		2  /* add data to digest */
#define SIG_MODE_FINAL		4  /* generate/verify signature */
#define SIG_MODE_ALL		(SIG_MODE_INIT|SIG_MODE_UPDATE|SIG_MODE_FINAL)


#define DST_FORCE_READ		0x1000000
#define DST_CAN_SIGN		0x010F
#define DST_NO_AUTHEN		0x8000
#define DST_EXTEND_FLAG         0x1000
#define DST_STANDARD		0
#define DST_PRIVATE             0x2000000
#define DST_PUBLIC              0x4000000
#define DST_RAND_SEMI           1
#define DST_RAND_STD            2
#define DST_RAND_KEY            3
#define DST_RAND_DSS            4



#define KEY_RSA			1
#define KEY_DH			2
#define KEY_DSA			3
#define KEY_PRIVATE		254
#define KEY_EXPAND		255
#define KEY_HMAC_MD5		157
#define KEY_HMAC_SHA1		158
#define UNKNOWN_KEYALG		0
#define DST_MAX_ALGS            KEY_HMAC_SHA1


#define DST_FLAGS_SIZE		2
#define DST_KEY_PROT		2
#define DST_KEY_ALG		3
#define DST_EXT_FLAG            4
#define DST_KEY_START		4

#ifndef SIGN_F_NOKEY 
#define SIGN_F_NOKEY		0xC000
#endif


#define SIGN_INIT_FAILURE	(-23)
#define SIGN_UPDATE_FAILURE	(-24)
#define SIGN_FINAL_FAILURE	(-25)
#define VERIFY_INIT_FAILURE	(-26)
#define VERIFY_UPDATE_FAILURE	(-27)
#define VERIFY_FINAL_FAILURE	(-28)
#define MISSING_KEY_OR_SIGNATURE (-30)
#define UNSUPPORTED_KEYALG	(-31)

#endif 
