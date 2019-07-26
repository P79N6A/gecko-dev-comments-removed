












































#ifndef RTP_PRIV_H
#define RTP_PRIV_H

#include "srtp_priv.h"
#include "rtp.h"

typedef srtp_hdr_t rtp_hdr_t;

typedef struct {
  srtp_hdr_t header;        
  char body[RTP_MAX_BUF_LEN];  
} rtp_msg_t;

typedef struct rtp_sender_ctx_t {
  rtp_msg_t message;         
  int socket;
  srtp_ctx_t *srtp_ctx;
  struct sockaddr_in addr;   
} rtp_sender_ctx_t;

typedef struct rtp_receiver_ctx_t {
  rtp_msg_t message;
  int socket;
  srtp_ctx_t *srtp_ctx;
  struct sockaddr_in addr;   
} rtp_receiver_ctx_t;


#endif 
