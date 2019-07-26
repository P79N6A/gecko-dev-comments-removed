











































#ifndef SRTP_PRIV_H
#define SRTP_PRIV_H

#include "srtp.h"
#include "rdbx.h"
#include "rdb.h"
#include "integers.h"









 
#ifndef WORDS_BIGENDIAN








typedef struct {
  unsigned char cc:4;	
  unsigned char x:1;	
  unsigned char p:1;	
  unsigned char version:2; 
  unsigned char pt:7;	
  unsigned char m:1;	
  uint16_t seq;		
  uint32_t ts;		
  uint32_t ssrc;	
} srtp_hdr_t;

#else 

typedef struct {
  unsigned char version:2; 
  unsigned char p:1;	
  unsigned char x:1;	
  unsigned char cc:4;	
  unsigned char m:1;	
  unsigned pt:7;	
  uint16_t seq;		
  uint32_t ts;		
  uint32_t ssrc;	
} srtp_hdr_t;

#endif

typedef struct {
  uint16_t profile_specific;    
  uint16_t length;              
} srtp_hdr_xtnd_t;









#ifndef WORDS_BIGENDIAN

typedef struct {
  unsigned char rc:5;		
  unsigned char p:1;		
  unsigned char version:2;	
  unsigned char pt:8;		
  uint16_t len;			
  uint32_t ssrc;	       	
} srtcp_hdr_t;

typedef struct {
  unsigned int index:31;    
  unsigned int e:1;         
  
  
} srtcp_trailer_t;


#else 

typedef struct {
  unsigned char version:2;	
  unsigned char p:1;		
  unsigned char rc:5;		
  unsigned char pt:8;		
  uint16_t len;			
  uint32_t ssrc;	       	
} srtcp_hdr_t;

typedef struct {
  unsigned int version:2;  
  unsigned int p:1;        
  unsigned int count:5;    
  unsigned int pt:8;       
  uint16_t length;         
} rtcp_common_t;

typedef struct {
  unsigned int e:1;         
  unsigned int index:31;    
  
  
} srtcp_trailer_t;

#endif











srtp_stream_t 
srtp_get_stream(srtp_t srtp, uint32_t ssrc);








err_status_t
srtp_stream_init_keys(srtp_stream_t srtp, const void *key);





err_status_t
srtp_stream_init(srtp_stream_t srtp, 
		 const srtp_policy_t *p);






typedef enum direction_t { 
  dir_unknown       = 0,
  dir_srtp_sender   = 1, 
  dir_srtp_receiver = 2
} direction_t;









typedef struct srtp_stream_ctx_t {
  uint32_t   ssrc;
  cipher_t  *rtp_cipher;
  auth_t    *rtp_auth;
  rdbx_t     rtp_rdbx;
  sec_serv_t rtp_services;
  cipher_t  *rtcp_cipher;
  auth_t    *rtcp_auth;
  rdb_t      rtcp_rdb;
  sec_serv_t rtcp_services;
  key_limit_ctx_t *limit;
  direction_t direction;
  int        allow_repeat_tx;
  ekt_stream_t ekt; 
  struct srtp_stream_ctx_t *next;   
} srtp_stream_ctx_t;






typedef struct srtp_ctx_t {
  srtp_stream_ctx_t *stream_list;     
  srtp_stream_ctx_t *stream_template; 
} srtp_ctx_t;











#define srtp_handle_event(srtp, strm, evnt)         \
   if(srtp_event_handler) {                         \
      srtp_event_data_t data;                       \
      data.session = srtp;                          \
      data.stream  = strm;                          \
      data.event   = evnt;                          \
      srtp_event_handler(&data);                    \
}   


#endif 
