




















































#ifndef RTP_H
#define RTP_H

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#elif defined HAVE_WINSOCK2_H
# include <winsock2.h>
#endif

#include "srtp.h"

typedef struct rtp_sender_ctx_t *rtp_sender_t;

typedef struct rtp_receiver_ctx_t *rtp_receiver_t;

int
rtp_sendto(rtp_sender_t sender, const void* msg, int len);

int
rtp_recvfrom(rtp_receiver_t receiver, void *msg, int *len);

int
rtp_receiver_init(rtp_receiver_t rcvr, int sock, 
		  struct sockaddr_in addr, unsigned int ssrc);

int
rtp_sender_init(rtp_sender_t sender, int sock, 
		struct sockaddr_in addr, unsigned int ssrc);





int
srtp_sender_init(rtp_sender_t rtp_ctx,          
		 struct sockaddr_in name,       
		 sec_serv_t security_services,  
		 unsigned char *input_key       
		 );

int
srtp_receiver_init(rtp_receiver_t rtp_ctx,       
		   struct sockaddr_in name, 	 
		   sec_serv_t security_services, 
		   unsigned char *input_key	 
		   );


int
rtp_sender_init_srtp(rtp_sender_t sender, const srtp_policy_t *policy);

int
rtp_sender_deinit_srtp(rtp_sender_t sender);

int
rtp_receiver_init_srtp(rtp_receiver_t sender, const srtp_policy_t *policy);

int
rtp_receiver_deinit_srtp(rtp_receiver_t sender);


rtp_sender_t 
rtp_sender_alloc(void);

void
rtp_sender_dealloc(rtp_sender_t rtp_ctx);

rtp_receiver_t 
rtp_receiver_alloc(void);

void
rtp_receiver_dealloc(rtp_receiver_t rtp_ctx);





#define RTP_HEADER_LEN   12




#define RTP_MAX_BUF_LEN  16384


#endif 
