































#ifdef __FreeBSD__
#include <sys/cdefs.h>
__FBSDID("$FreeBSD: head/sys/netinet/sctp_uio.h 255160 2013-09-02 22:48:41Z tuexen $");
#endif

#ifndef _NETINET_SCTP_UIO_H_
#define _NETINET_SCTP_UIO_H_

#if (defined(__APPLE__) && defined(KERNEL))
#ifndef _KERNEL
#define _KERNEL
#endif
#endif

#if !(defined(__Windows__)) && !defined(__Userspace_os_Windows)
#if ! defined(_KERNEL)
#include <stdint.h>
#endif
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif
#if defined(__Windows__)
#pragma warning(push)
#pragma warning(disable: 4200)
#if defined(_KERNEL)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif
#endif

typedef uint32_t sctp_assoc_t;

#define SCTP_FUTURE_ASSOC  0
#define SCTP_CURRENT_ASSOC 1
#define SCTP_ALL_ASSOC     2

struct sctp_event {
	sctp_assoc_t se_assoc_id;
	uint16_t     se_type;
	uint8_t      se_on;
};


#define sctp_stream_reset_events sctp_stream_reset_event


struct sctp_event_subscribe {
	uint8_t sctp_data_io_event;
	uint8_t sctp_association_event;
	uint8_t sctp_address_event;
	uint8_t sctp_send_failure_event;
	uint8_t sctp_peer_error_event;
	uint8_t sctp_shutdown_event;
	uint8_t sctp_partial_delivery_event;
	uint8_t sctp_adaptation_layer_event;
	uint8_t sctp_authentication_event;
	uint8_t sctp_sender_dry_event;
	uint8_t sctp_stream_reset_event;
};


#define SCTP_INIT	0x0001
#define SCTP_SNDRCV	0x0002
#define SCTP_EXTRCV	0x0003
#define SCTP_SNDINFO    0x0004
#define SCTP_RCVINFO    0x0005
#define SCTP_NXTINFO    0x0006
#define SCTP_PRINFO     0x0007
#define SCTP_AUTHINFO   0x0008
#define SCTP_DSTADDRV4  0x0009
#define SCTP_DSTADDRV6  0x000a




struct sctp_initmsg {
#if defined(__FreeBSD__) && __FreeBSD_version < 800000
	
	uint32_t sinit_num_ostreams;
	uint32_t sinit_max_instreams;
#else
	uint16_t sinit_num_ostreams;
	uint16_t sinit_max_instreams;
#endif
	uint16_t sinit_max_attempts;
	uint16_t sinit_max_init_timeo;
};















#define SCTP_ALIGN_RESV_PAD 92
#define SCTP_ALIGN_RESV_PAD_SHORT 76

struct sctp_sndrcvinfo {
	uint16_t sinfo_stream;
	uint16_t sinfo_ssn;
	uint16_t sinfo_flags;
#if defined(__FreeBSD__) && __FreeBSD_version < 800000
	uint16_t sinfo_pr_policy;
#endif
	uint32_t sinfo_ppid;
	uint32_t sinfo_context;
	uint32_t sinfo_timetolive;
	uint32_t sinfo_tsn;
	uint32_t sinfo_cumtsn;
	sctp_assoc_t sinfo_assoc_id;
	uint16_t sinfo_keynumber;
	uint16_t sinfo_keynumber_valid;
	uint8_t  __reserve_pad[SCTP_ALIGN_RESV_PAD];
};

struct sctp_extrcvinfo {
	uint16_t sinfo_stream;
	uint16_t sinfo_ssn;
	uint16_t sinfo_flags;
#if defined(__FreeBSD__) && __FreeBSD_version < 800000
	uint16_t sinfo_pr_policy;
#endif
	uint32_t sinfo_ppid;
	uint32_t sinfo_context;
	uint32_t sinfo_timetolive;
	uint32_t sinfo_tsn;
	uint32_t sinfo_cumtsn;
	sctp_assoc_t sinfo_assoc_id;
	uint16_t sreinfo_next_flags;
	uint16_t sreinfo_next_stream;
	uint32_t sreinfo_next_aid;
	uint32_t sreinfo_next_length;
	uint32_t sreinfo_next_ppid;
	uint16_t sinfo_keynumber;
	uint16_t sinfo_keynumber_valid;
	uint8_t  __reserve_pad[SCTP_ALIGN_RESV_PAD_SHORT];
};

struct sctp_sndinfo {
	uint16_t snd_sid;
	uint16_t snd_flags;
	uint32_t snd_ppid;
	uint32_t snd_context;
	sctp_assoc_t snd_assoc_id;
};

struct sctp_prinfo {
	uint16_t pr_policy;
	uint32_t pr_value;
};

struct sctp_default_prinfo {
	uint16_t pr_policy;
	uint32_t pr_value;
	sctp_assoc_t pr_assoc_id;
};

struct sctp_authinfo {
	uint16_t auth_keynumber;
};

struct sctp_rcvinfo {
	uint16_t rcv_sid;
	uint16_t rcv_ssn;
	uint16_t rcv_flags;
	uint32_t rcv_ppid;
	uint32_t rcv_tsn;
	uint32_t rcv_cumtsn;
	uint32_t rcv_context;
	sctp_assoc_t rcv_assoc_id;
};

struct sctp_nxtinfo {
	uint16_t nxt_sid;
	uint16_t nxt_flags;
	uint32_t nxt_ppid;
	uint32_t nxt_length;
	sctp_assoc_t nxt_assoc_id;
};

#define SCTP_NO_NEXT_MSG           0x0000
#define SCTP_NEXT_MSG_AVAIL        0x0001
#define SCTP_NEXT_MSG_ISCOMPLETE   0x0002
#define SCTP_NEXT_MSG_IS_UNORDERED 0x0004
#define SCTP_NEXT_MSG_IS_NOTIFICATION 0x0008

struct sctp_recvv_rn {
	struct sctp_rcvinfo recvv_rcvinfo;
	struct sctp_nxtinfo recvv_nxtinfo;
};

#define SCTP_RECVV_NOINFO  0
#define SCTP_RECVV_RCVINFO 1
#define SCTP_RECVV_NXTINFO 2
#define SCTP_RECVV_RN      3

#define SCTP_SENDV_NOINFO   0
#define SCTP_SENDV_SNDINFO  1
#define SCTP_SENDV_PRINFO   2
#define SCTP_SENDV_AUTHINFO 3
#define SCTP_SENDV_SPA      4

struct sctp_sendv_spa {
	uint32_t sendv_flags;
	struct sctp_sndinfo sendv_sndinfo;
	struct sctp_prinfo sendv_prinfo;
	struct sctp_authinfo sendv_authinfo;
};

#define SCTP_SEND_SNDINFO_VALID  0x00000001
#define SCTP_SEND_PRINFO_VALID   0x00000002
#define SCTP_SEND_AUTHINFO_VALID 0x00000004

struct sctp_snd_all_completes {
	uint16_t sall_stream;
	uint16_t sall_flags;
	uint32_t sall_ppid;
	uint32_t sall_context;
	uint32_t sall_num_sent;
	uint32_t sall_num_failed;
};


#define SCTP_NOTIFICATION     0x0010 /* next message is a notification */
#define SCTP_COMPLETE         0x0020 /* next message is complete */
#define SCTP_EOF              0x0100 /* Start shutdown procedures */
#define SCTP_ABORT            0x0200 /* Send an ABORT to peer */
#define SCTP_UNORDERED        0x0400 /* Message is un-ordered */
#define SCTP_ADDR_OVER        0x0800 /* Override the primary-address */
#define SCTP_SENDALL          0x1000 /* Send this on all associations */
#define SCTP_EOR              0x2000 /* end of message signal */
#define SCTP_SACK_IMMEDIATELY 0x4000 /* Set I-Bit */

#define INVALID_SINFO_FLAG(x) (((x) & 0xfffffff0 \
                                    & ~(SCTP_EOF | SCTP_ABORT | SCTP_UNORDERED |\
				        SCTP_ADDR_OVER | SCTP_SENDALL | SCTP_EOR |\
					SCTP_SACK_IMMEDIATELY)) != 0)



#define SCTP_PR_SCTP_NONE 0x0000 /* Reliable transfer */
#define SCTP_PR_SCTP_TTL  0x0001 /* Time based PR-SCTP */
#define SCTP_PR_SCTP_BUF  0x0002 /* Buffer based PR-SCTP */
#define SCTP_PR_SCTP_RTX  0x0003 /* Number of retransmissions based PR-SCTP */

#define PR_SCTP_POLICY(x)         ((x) & 0x0f)
#define PR_SCTP_ENABLED(x)        (PR_SCTP_POLICY(x) != SCTP_PR_SCTP_NONE)
#define PR_SCTP_TTL_ENABLED(x)    (PR_SCTP_POLICY(x) == SCTP_PR_SCTP_TTL)
#define PR_SCTP_BUF_ENABLED(x)    (PR_SCTP_POLICY(x) == SCTP_PR_SCTP_BUF)
#define PR_SCTP_RTX_ENABLED(x)    (PR_SCTP_POLICY(x) == SCTP_PR_SCTP_RTX)
#define PR_SCTP_INVALID_POLICY(x) (PR_SCTP_POLICY(x) > SCTP_PR_SCTP_RTX)

struct sctp_pcbinfo {
	uint32_t ep_count;
	uint32_t asoc_count;
	uint32_t laddr_count;
	uint32_t raddr_count;
	uint32_t chk_count;
	uint32_t readq_count;
	uint32_t free_chunks;
	uint32_t stream_oque;
};

struct sctp_sockstat {
	sctp_assoc_t ss_assoc_id;
	uint32_t ss_total_sndbuf;
	uint32_t ss_total_recv_buf;
};








struct sctp_assoc_change {
	uint16_t sac_type;
	uint16_t sac_flags;
	uint32_t sac_length;
	uint16_t sac_state;
	uint16_t sac_error;
	uint16_t sac_outbound_streams;
	uint16_t sac_inbound_streams;
	sctp_assoc_t sac_assoc_id;
	uint8_t sac_info[];
};


#define SCTP_COMM_UP            0x0001
#define SCTP_COMM_LOST          0x0002
#define SCTP_RESTART            0x0003
#define SCTP_SHUTDOWN_COMP      0x0004
#define SCTP_CANT_STR_ASSOC     0x0005


#define SCTP_ASSOC_SUPPORTS_PR        0x01
#define SCTP_ASSOC_SUPPORTS_AUTH      0x02
#define SCTP_ASSOC_SUPPORTS_ASCONF    0x03
#define SCTP_ASSOC_SUPPORTS_MULTIBUF  0x04
#define SCTP_ASSOC_SUPPORTS_RE_CONFIG 0x05
#define SCTP_ASSOC_SUPPORTS_MAX       0x05



struct sctp_paddr_change {
	uint16_t spc_type;
	uint16_t spc_flags;
	uint32_t spc_length;
	struct sockaddr_storage spc_aaddr;
	uint32_t spc_state;
	uint32_t spc_error;
	sctp_assoc_t spc_assoc_id;
	uint8_t spc_padding[4];
};


#define SCTP_ADDR_AVAILABLE	0x0001
#define SCTP_ADDR_UNREACHABLE	0x0002
#define SCTP_ADDR_REMOVED	0x0003
#define SCTP_ADDR_ADDED		0x0004
#define SCTP_ADDR_MADE_PRIM	0x0005
#define SCTP_ADDR_CONFIRMED	0x0006

#define SCTP_ACTIVE		0x0001	/* SCTP_ADDR_REACHABLE */
#define SCTP_INACTIVE		0x0002	/* neither SCTP_ADDR_REACHABLE
					   nor SCTP_ADDR_UNCONFIRMED */
#define SCTP_UNCONFIRMED	0x0200	/* SCTP_ADDR_UNCONFIRMED */


struct sctp_remote_error {
	uint16_t sre_type;
	uint16_t sre_flags;
	uint32_t sre_length;
	uint16_t sre_error;
	sctp_assoc_t sre_assoc_id;
	uint8_t sre_data[4];
};


struct sctp_send_failed {
	uint16_t ssf_type;
	uint16_t ssf_flags;
	uint32_t ssf_length;
	uint32_t ssf_error;
	struct sctp_sndrcvinfo ssf_info;
	sctp_assoc_t ssf_assoc_id;
	uint8_t ssf_data[];
};


struct sctp_send_failed_event {
	uint16_t ssfe_type;
	uint16_t ssfe_flags;
	uint32_t ssfe_length;
	uint32_t ssfe_error;
	struct sctp_sndinfo ssfe_info;
	sctp_assoc_t ssfe_assoc_id;
	uint8_t  ssfe_data[];
};


#define SCTP_DATA_UNSENT	0x0001	/* inqueue never on wire */
#define SCTP_DATA_SENT		0x0002	/* on wire at failure */


struct sctp_shutdown_event {
	uint16_t sse_type;
	uint16_t sse_flags;
	uint32_t sse_length;
	sctp_assoc_t sse_assoc_id;
};


struct sctp_adaptation_event {
	uint16_t sai_type;
	uint16_t sai_flags;
	uint32_t sai_length;
	uint32_t sai_adaptation_ind;
	sctp_assoc_t sai_assoc_id;
};

struct sctp_setadaptation {
	uint32_t ssb_adaptation_ind;
};


struct sctp_adaption_event {
	uint16_t sai_type;
	uint16_t sai_flags;
	uint32_t sai_length;
	uint32_t sai_adaption_ind;
	sctp_assoc_t sai_assoc_id;
};

struct sctp_setadaption {
	uint32_t ssb_adaption_ind;
};





struct sctp_pdapi_event {
	uint16_t pdapi_type;
	uint16_t pdapi_flags;
	uint32_t pdapi_length;
	uint32_t pdapi_indication;
	uint16_t pdapi_stream;
	uint16_t pdapi_seq;
	sctp_assoc_t pdapi_assoc_id;
};


#define SCTP_PARTIAL_DELIVERY_ABORTED	0x0001





struct sctp_authkey_event {
	uint16_t auth_type;
	uint16_t auth_flags;
	uint32_t auth_length;
	uint16_t auth_keynumber;
	uint16_t auth_altkeynumber;
	uint32_t auth_indication;
	sctp_assoc_t auth_assoc_id;
};


#define SCTP_AUTH_NEW_KEY	0x0001
#define SCTP_AUTH_NEWKEY	SCTP_AUTH_NEW_KEY
#define SCTP_AUTH_NO_AUTH	0x0002
#define SCTP_AUTH_FREE_KEY	0x0003


struct sctp_sender_dry_event {
	uint16_t sender_dry_type;
	uint16_t sender_dry_flags;
	uint32_t sender_dry_length;
	sctp_assoc_t sender_dry_assoc_id;
};





struct sctp_stream_reset_event {
	uint16_t strreset_type;
	uint16_t strreset_flags;
	uint32_t strreset_length;
	sctp_assoc_t strreset_assoc_id;
	uint16_t strreset_stream_list[];
};


#define SCTP_STREAM_RESET_INCOMING_SSN  0x0001
#define SCTP_STREAM_RESET_OUTGOING_SSN  0x0002
#define SCTP_STREAM_RESET_DENIED        0x0004
#define SCTP_STREAM_RESET_FAILED        0x0008




struct sctp_assoc_reset_event {
	uint16_t 	assocreset_type;
	uint16_t	assocreset_flags;
	uint32_t	assocreset_length;
	sctp_assoc_t	assocreset_assoc_id;
	uint32_t	assocreset_local_tsn;
	uint32_t	assocreset_remote_tsn;
};

#define SCTP_ASSOC_RESET_DENIED		0x0004
#define SCTP_ASSOC_RESET_FAILED		0x0008




struct sctp_stream_change_event {
	uint16_t	strchange_type;
	uint16_t	strchange_flags;
	uint32_t	strchange_length;
	sctp_assoc_t	strchange_assoc_id;
	uint16_t	strchange_instrms;
	uint16_t	strchange_outstrms;
};

#define SCTP_STREAM_CHANGE_DENIED	0x0004
#define SCTP_STREAM_CHANGE_FAILED	0x0008



struct sctp_tlv {
	uint16_t sn_type;
	uint16_t sn_flags;
	uint32_t sn_length;
};

union sctp_notification {
	struct sctp_tlv sn_header;
	struct sctp_assoc_change sn_assoc_change;
	struct sctp_paddr_change sn_paddr_change;
	struct sctp_remote_error sn_remote_error;
	struct sctp_send_failed sn_send_failed;
	struct sctp_shutdown_event sn_shutdown_event;
	struct sctp_adaptation_event sn_adaptation_event;
	
	struct sctp_adaption_event sn_adaption_event;
	struct sctp_pdapi_event sn_pdapi_event;
	struct sctp_authkey_event sn_auth_event;
	struct sctp_sender_dry_event sn_sender_dry_event;
	struct sctp_send_failed_event sn_send_failed_event;
	struct sctp_stream_reset_event sn_strreset_event;
	struct sctp_assoc_reset_event  sn_assocreset_event;
	struct sctp_stream_change_event sn_strchange_event;
};


#define SCTP_ASSOC_CHANGE                       0x0001
#define SCTP_PEER_ADDR_CHANGE                   0x0002
#define SCTP_REMOTE_ERROR                       0x0003
#define SCTP_SEND_FAILED                        0x0004
#define SCTP_SHUTDOWN_EVENT                     0x0005
#define SCTP_ADAPTATION_INDICATION              0x0006

#define SCTP_ADAPTION_INDICATION                0x0006
#define SCTP_PARTIAL_DELIVERY_EVENT             0x0007
#define SCTP_AUTHENTICATION_EVENT               0x0008
#define SCTP_STREAM_RESET_EVENT                 0x0009
#define SCTP_SENDER_DRY_EVENT                   0x000a
#define SCTP_NOTIFICATIONS_STOPPED_EVENT        0x000b /* we don't send this*/
#define SCTP_ASSOC_RESET_EVENT                  0x000c
#define SCTP_STREAM_CHANGE_EVENT                0x000d
#define SCTP_SEND_FAILED_EVENT                  0x000e




struct sctp_paddrparams {
	struct sockaddr_storage spp_address;
	sctp_assoc_t spp_assoc_id;
	uint32_t spp_hbinterval;
	uint32_t spp_pathmtu;
	uint32_t spp_flags;
	uint32_t spp_ipv6_flowlabel;
	uint16_t spp_pathmaxrxt;
	uint8_t spp_dscp;
};
#define spp_ipv4_tos spp_dscp

#define SPP_HB_ENABLE		0x00000001
#define SPP_HB_DISABLE		0x00000002
#define SPP_HB_DEMAND		0x00000004
#define SPP_PMTUD_ENABLE	0x00000008
#define SPP_PMTUD_DISABLE	0x00000010
#define SPP_HB_TIME_IS_ZERO     0x00000080
#define SPP_IPV6_FLOWLABEL      0x00000100
#define SPP_DSCP                0x00000200
#define SPP_IPV4_TOS            SPP_DSCP

struct sctp_paddrthlds {
	struct sockaddr_storage spt_address;
	sctp_assoc_t spt_assoc_id;
	uint16_t spt_pathmaxrxt;
	uint16_t spt_pathpfthld;
};

struct sctp_paddrinfo {
	struct sockaddr_storage spinfo_address;
	sctp_assoc_t spinfo_assoc_id;
	int32_t spinfo_state;
	uint32_t spinfo_cwnd;
	uint32_t spinfo_srtt;
	uint32_t spinfo_rto;
	uint32_t spinfo_mtu;
};

struct sctp_rtoinfo {
	sctp_assoc_t srto_assoc_id;
	uint32_t srto_initial;
	uint32_t srto_max;
	uint32_t srto_min;
};

struct sctp_assocparams {
	sctp_assoc_t sasoc_assoc_id;
	uint32_t sasoc_peer_rwnd;
	uint32_t sasoc_local_rwnd;
	uint32_t sasoc_cookie_life;
	uint16_t sasoc_asocmaxrxt;
	uint16_t sasoc_number_peer_destinations;
};

struct sctp_setprim {
	struct sockaddr_storage ssp_addr;
	sctp_assoc_t ssp_assoc_id;
	uint8_t ssp_padding[4];
};

struct sctp_setpeerprim {
	struct sockaddr_storage sspp_addr;
	sctp_assoc_t sspp_assoc_id;
	uint8_t sspp_padding[4];
};

struct sctp_getaddresses {
	sctp_assoc_t sget_assoc_id;
	
	struct sockaddr addr[1];
};

struct sctp_status {
	sctp_assoc_t sstat_assoc_id;
	int32_t sstat_state;
	uint32_t sstat_rwnd;
	uint16_t sstat_unackdata;
	uint16_t sstat_penddata;
	uint16_t sstat_instrms;
	uint16_t sstat_outstrms;
	uint32_t sstat_fragmentation_point;
	struct sctp_paddrinfo sstat_primary;
};





struct sctp_authchunk {
	uint8_t sauth_chunk;
};


struct sctp_authkey {
	sctp_assoc_t sca_assoc_id;
	uint16_t sca_keynumber;
	uint16_t sca_keylength;
	uint8_t sca_key[];
};


struct sctp_hmacalgo {
	uint32_t shmac_number_of_idents;
	uint16_t shmac_idents[];
};


#define SCTP_AUTH_HMAC_ID_RSVD		0x0000
#define SCTP_AUTH_HMAC_ID_SHA1		0x0001	/* default, mandatory */
#define SCTP_AUTH_HMAC_ID_SHA256	0x0003


struct sctp_authkeyid {
	sctp_assoc_t scact_assoc_id;
	uint16_t scact_keynumber;
};


struct sctp_authchunks {
	sctp_assoc_t gauth_assoc_id;
	uint32_t gauth_number_of_chunks;
	uint8_t gauth_chunks[];
};

struct sctp_assoc_value {
	sctp_assoc_t assoc_id;
	uint32_t assoc_value;
};

struct sctp_cc_option {
	int option;
	struct sctp_assoc_value aid_value;
};

struct sctp_stream_value {
	sctp_assoc_t assoc_id;
	uint16_t stream_id;
	uint16_t stream_value;
};

struct sctp_assoc_ids {
	uint32_t gaids_number_of_ids;
	sctp_assoc_t gaids_assoc_id[];
};

struct sctp_sack_info {
	sctp_assoc_t sack_assoc_id;
	uint32_t sack_delay;
	uint32_t sack_freq;
};

struct sctp_timeouts {
	sctp_assoc_t stimo_assoc_id;
	uint32_t stimo_init;
	uint32_t stimo_data;
	uint32_t stimo_sack;
	uint32_t stimo_shutdown;
	uint32_t stimo_heartbeat;
	uint32_t stimo_cookie;
	uint32_t stimo_shutdownack;
};

struct sctp_udpencaps {
	struct sockaddr_storage sue_address;
	sctp_assoc_t sue_assoc_id;
	uint16_t sue_port;
};

struct sctp_cwnd_args {
	struct sctp_nets *net;	 
	uint32_t cwnd_new_value;
	uint32_t pseudo_cumack;
	uint16_t inflight;	
	uint16_t cwnd_augment;	
	uint8_t meets_pseudo_cumack;
	uint8_t need_new_pseudo_cumack;
	uint8_t cnt_in_send;
	uint8_t cnt_in_str;
};

struct sctp_blk_args {
	uint32_t onsb;		
	uint32_t sndlen;	
	uint32_t peer_rwnd;	
	uint16_t send_sent_qcnt;
	uint16_t stream_qcnt;	
	uint16_t chunks_on_oque;
	uint16_t flight_size;   
};







#define SCTP_MAX_EXPLICT_STR_RESET   1000

struct sctp_reset_streams {
	sctp_assoc_t srs_assoc_id;
	uint16_t srs_flags;
	uint16_t srs_number_streams;	
	uint16_t srs_stream_list[];
};

struct sctp_add_streams {
	sctp_assoc_t	sas_assoc_id;
	uint16_t	sas_instrms;
	uint16_t	sas_outstrms;
};

struct sctp_get_nonce_values {
	sctp_assoc_t gn_assoc_id;
	uint32_t gn_peers_tag;
	uint32_t gn_local_tag;
};


struct sctp_str_log {
	void *stcb; 
	uint32_t n_tsn;
	uint32_t e_tsn;
	uint16_t n_sseq;
	uint16_t e_sseq;
	uint16_t strm;
};

struct sctp_sb_log {
	void  *stcb; 
	uint32_t so_sbcc;
	uint32_t stcb_sbcc;
	uint32_t incr;
};

struct sctp_fr_log {
	uint32_t largest_tsn;
	uint32_t largest_new_tsn;
	uint32_t tsn;
};

struct sctp_fr_map {
	uint32_t base;
	uint32_t cum;
	uint32_t high;
};

struct sctp_rwnd_log {
	uint32_t rwnd;
	uint32_t send_size;
	uint32_t overhead;
	uint32_t new_rwnd;
};

struct sctp_mbcnt_log {
	uint32_t total_queue_size;
	uint32_t size_change;
	uint32_t total_queue_mb_size;
	uint32_t mbcnt_change;
};

struct sctp_sack_log {
	uint32_t cumack;
	uint32_t oldcumack;
	uint32_t tsn;
	uint16_t numGaps;
	uint16_t numDups;
};

struct sctp_lock_log {
	void *sock;  
	void *inp; 
	uint8_t tcb_lock;
	uint8_t inp_lock;
	uint8_t info_lock;
	uint8_t sock_lock;
	uint8_t sockrcvbuf_lock;
	uint8_t socksndbuf_lock;
	uint8_t create_lock;
	uint8_t resv;
};

struct sctp_rto_log {
	void * net; 
	uint32_t rtt;
};

struct sctp_nagle_log {
	void  *stcb; 
	uint32_t total_flight;
	uint32_t total_in_queue;
	uint16_t count_in_queue;
	uint16_t count_in_flight;
};

struct sctp_sbwake_log {
	void *stcb; 
	uint16_t send_q;
	uint16_t sent_q;
	uint16_t flight;
	uint16_t wake_cnt;
	uint8_t stream_qcnt;	
	uint8_t chunks_on_oque;
	uint8_t sbflags;
	uint8_t sctpflags;
};

struct sctp_misc_info {
	uint32_t log1;
	uint32_t log2;
	uint32_t log3;
	uint32_t log4;
};

struct sctp_log_closing {
	void *inp; 
	void *stcb;  
	uint32_t sctp_flags;
	uint16_t  state;
	int16_t  loc;
};

struct sctp_mbuf_log {
	struct mbuf *mp; 
	caddr_t  ext;
	caddr_t  data;
	uint16_t size;
	uint8_t  refcnt;
	uint8_t  mbuf_flags;
};

struct sctp_cwnd_log {
	uint64_t time_event;
	uint8_t  from;
	uint8_t  event_type;
	uint8_t  resv[2];
	union {
		struct sctp_log_closing close;
		struct sctp_blk_args blk;
		struct sctp_cwnd_args cwnd;
		struct sctp_str_log strlog;
		struct sctp_fr_log fr;
		struct sctp_fr_map map;
		struct sctp_rwnd_log rwnd;
		struct sctp_mbcnt_log mbcnt;
		struct sctp_sack_log sack;
		struct sctp_lock_log lock;
		struct sctp_rto_log rto;
		struct sctp_sb_log sb;
		struct sctp_nagle_log nagle;
		struct sctp_sbwake_log wake;
		struct sctp_mbuf_log mb;
		struct sctp_misc_info misc;
	}     x;
};

struct sctp_cwnd_log_req {
	int32_t num_in_log;		
	int32_t num_ret;		
	int32_t start_at;		
	int32_t end_at;		        
	struct sctp_cwnd_log log[];
};

struct sctp_timeval {
	uint32_t tv_sec;
	uint32_t tv_usec;
};

struct sctpstat {
	struct sctp_timeval sctps_discontinuitytime; 
	
	uint32_t  sctps_currestab;           
	uint32_t  sctps_activeestab;         
	uint32_t  sctps_restartestab;
	uint32_t  sctps_collisionestab;
	uint32_t  sctps_passiveestab;        
	uint32_t  sctps_aborted;             
	uint32_t  sctps_shutdown;            
	uint32_t  sctps_outoftheblue;        
	uint32_t  sctps_checksumerrors;      
	uint32_t  sctps_outcontrolchunks;    
	uint32_t  sctps_outorderchunks;      
	uint32_t  sctps_outunorderchunks;    
	uint32_t  sctps_incontrolchunks;     
	uint32_t  sctps_inorderchunks;       
	uint32_t  sctps_inunorderchunks;     
	uint32_t  sctps_fragusrmsgs;         
	uint32_t  sctps_reasmusrmsgs;        
	uint32_t  sctps_outpackets;          
	uint32_t  sctps_inpackets;           

	
	uint32_t  sctps_recvpackets;         
	uint32_t  sctps_recvdatagrams;       
	uint32_t  sctps_recvpktwithdata;     
	uint32_t  sctps_recvsacks;           
	uint32_t  sctps_recvdata;            
	uint32_t  sctps_recvdupdata;         
	uint32_t  sctps_recvheartbeat;       
	uint32_t  sctps_recvheartbeatack;    
	uint32_t  sctps_recvecne;            
	uint32_t  sctps_recvauth;            
	uint32_t  sctps_recvauthmissing;     
	uint32_t  sctps_recvivalhmacid;      
	uint32_t  sctps_recvivalkeyid;       
	uint32_t  sctps_recvauthfailed;      
	uint32_t  sctps_recvexpress;         
	uint32_t  sctps_recvexpressm;        
	uint32_t  sctps_recvnocrc;
	uint32_t  sctps_recvswcrc;
	uint32_t  sctps_recvhwcrc;

	
	uint32_t  sctps_sendpackets;         
	uint32_t  sctps_sendsacks;           
	uint32_t  sctps_senddata;            
	uint32_t  sctps_sendretransdata;     
	uint32_t  sctps_sendfastretrans;     
	uint32_t  sctps_sendmultfastretrans; 


	uint32_t  sctps_sendheartbeat;       
	uint32_t  sctps_sendecne;            
	uint32_t  sctps_sendauth;            
	uint32_t  sctps_senderrors;	     
	uint32_t  sctps_sendnocrc;
	uint32_t  sctps_sendswcrc;
	uint32_t  sctps_sendhwcrc;
	
	uint32_t  sctps_pdrpfmbox;           
	uint32_t  sctps_pdrpfehos;           
	uint32_t  sctps_pdrpmbda;            
	uint32_t  sctps_pdrpmbct;            
	uint32_t  sctps_pdrpbwrpt;           
	uint32_t  sctps_pdrpcrupt;           
	uint32_t  sctps_pdrpnedat;           
	uint32_t  sctps_pdrppdbrk;           
	uint32_t  sctps_pdrptsnnf;           
	uint32_t  sctps_pdrpdnfnd;           
	uint32_t  sctps_pdrpdiwnp;           
	uint32_t  sctps_pdrpdizrw;           
	uint32_t  sctps_pdrpbadd;            
	uint32_t  sctps_pdrpmark;            
	
	uint32_t  sctps_timoiterator;        
	uint32_t  sctps_timodata;            
	uint32_t  sctps_timowindowprobe;     
	uint32_t  sctps_timoinit;            
	uint32_t  sctps_timosack;            
	uint32_t  sctps_timoshutdown;        
	uint32_t  sctps_timoheartbeat;       
	uint32_t  sctps_timocookie;          
	uint32_t  sctps_timosecret;          
	uint32_t  sctps_timopathmtu;         
	uint32_t  sctps_timoshutdownack;     
	uint32_t  sctps_timoshutdownguard;   
	uint32_t  sctps_timostrmrst;         
	uint32_t  sctps_timoearlyfr;         
	uint32_t  sctps_timoasconf;          
	uint32_t  sctps_timodelprim;	     
	uint32_t  sctps_timoautoclose;       
	uint32_t  sctps_timoassockill;       
	uint32_t  sctps_timoinpkill;         
	
	uint32_t  sctps_spare[11];
	
	uint32_t  sctps_hdrops;	          
	uint32_t  sctps_badsum;	          
	uint32_t  sctps_noport;           
	uint32_t  sctps_badvtag;          
	uint32_t  sctps_badsid;           
	uint32_t  sctps_nomem;            
	uint32_t  sctps_fastretransinrtt; 
	uint32_t  sctps_markedretrans;
	uint32_t  sctps_naglesent;        
	uint32_t  sctps_naglequeued;      
	uint32_t  sctps_maxburstqueued;   
	uint32_t  sctps_ifnomemqueued;    



	uint32_t  sctps_windowprobed;     
	uint32_t  sctps_lowlevelerr;	


	uint32_t  sctps_lowlevelerrusr;	



	uint32_t  sctps_datadropchklmt;	
	uint32_t  sctps_datadroprwnd;	
	uint32_t  sctps_ecnereducedcwnd;  
	uint32_t  sctps_vtagexpress;	
	uint32_t  sctps_vtagbogus;	
	uint32_t  sctps_primary_randry;	
	uint32_t  sctps_cmt_randry;       
	uint32_t  sctps_slowpath_sack;    
	uint32_t  sctps_wu_sacks_sent;	
	uint32_t  sctps_sends_with_flags; 
	uint32_t  sctps_sends_with_unord; 
	uint32_t  sctps_sends_with_eof; 	
	uint32_t  sctps_sends_with_abort; 
	uint32_t  sctps_protocol_drain_calls;	
	uint32_t  sctps_protocol_drains_done; 	
	uint32_t  sctps_read_peeks;	
	uint32_t  sctps_cached_chk;       
	uint32_t  sctps_cached_strmoq;    
	uint32_t  sctps_left_abandon;     
	uint32_t  sctps_send_burst_avoid; 
	uint32_t  sctps_send_cwnd_avoid;  
	uint32_t  sctps_fwdtsn_map_over;  
	uint32_t  sctps_queue_upd_ecne;  
	uint32_t  sctps_reserved[31];     
};

#define SCTP_STAT_INCR(_x) SCTP_STAT_INCR_BY(_x,1)
#define SCTP_STAT_DECR(_x) SCTP_STAT_DECR_BY(_x,1)
#if defined(__FreeBSD__) && defined(SMP) && defined(SCTP_USE_PERCPU_STAT)
#define SCTP_STAT_INCR_BY(_x,_d) (SCTP_BASE_STATS[PCPU_GET(cpuid)]._x += _d)
#define SCTP_STAT_DECR_BY(_x,_d) (SCTP_BASE_STATS[PCPU_GET(cpuid)]._x -= _d)
#else
#define SCTP_STAT_INCR_BY(_x,_d) atomic_add_int(&SCTP_BASE_STAT(_x), _d)
#define SCTP_STAT_DECR_BY(_x,_d) atomic_subtract_int(&SCTP_BASE_STAT(_x), _d)
#endif

#define SCTP_STAT_INCR_COUNTER32(_x) SCTP_STAT_INCR(_x)
#define SCTP_STAT_INCR_COUNTER64(_x) SCTP_STAT_INCR(_x)
#define SCTP_STAT_INCR_GAUGE32(_x) SCTP_STAT_INCR(_x)
#define SCTP_STAT_DECR_COUNTER32(_x) SCTP_STAT_DECR(_x)
#define SCTP_STAT_DECR_COUNTER64(_x) SCTP_STAT_DECR(_x)
#define SCTP_STAT_DECR_GAUGE32(_x) SCTP_STAT_DECR(_x)

#if defined(__Userspace__)
union sctp_sockstore {
#if defined(INET)
	struct sockaddr_in sin;
#endif
#if defined(INET6)
	struct sockaddr_in6 sin6;
#endif
	struct sockaddr_conn sconn;
	struct sockaddr sa;
};
#else
union sctp_sockstore {
	struct sockaddr_in sin;
	struct sockaddr_in6 sin6;
	struct sockaddr sa;
};
#endif






#ifndef __APPLE__
#ifndef __Userspace__
#ifndef ntohll
#if defined(__Userspace_os_Linux)
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#include <endian.h>
#else
#include <sys/endian.h>
#endif
#define ntohll(x) be64toh(x)
#endif

#ifndef htonll
#if defined(__Userspace_os_Linux)
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#include <endian.h>
#else
#include <sys/endian.h>
#endif
#define htonll(x) htobe64(x)
#endif
#endif
#endif



struct xsctp_inpcb {
	uint32_t last;
	uint32_t flags;
#if defined(__FreeBSD__) && __FreeBSD_version < 1000048
	uint32_t features;
#else
	uint64_t features;
#endif
	uint32_t total_sends;
	uint32_t total_recvs;
	uint32_t total_nospaces;
	uint32_t fragmentation_point;
	uint16_t local_port;
	uint16_t qlen;
	uint16_t maxqlen;
#if defined(__Windows__)
	uint16_t padding;
#endif
#if defined(__FreeBSD__) && __FreeBSD_version < 1000048
	uint32_t extra_padding[32]; 
#else
	uint32_t extra_padding[31]; 
#endif
};

struct xsctp_tcb {
	union sctp_sockstore primary_addr;      
	uint32_t last;
	uint32_t heartbeat_interval;            
	uint32_t state;                         
	uint32_t in_streams;                    
	uint32_t out_streams;                   
	uint32_t max_nr_retrans;                
	uint32_t primary_process;               
	uint32_t T1_expireries;                 
	uint32_t T2_expireries;                 
	uint32_t retransmitted_tsns;            
	uint32_t total_sends;
	uint32_t total_recvs;
	uint32_t local_tag;
	uint32_t remote_tag;
	uint32_t initial_tsn;
	uint32_t highest_tsn;
	uint32_t cumulative_tsn;
	uint32_t cumulative_tsn_ack;
	uint32_t mtu;
	uint32_t refcnt;
	uint16_t local_port;                    
	uint16_t remote_port;                   
	struct sctp_timeval start_time;         
	struct sctp_timeval discontinuity_time; 
#if defined(__FreeBSD__)
#if __FreeBSD_version >= 800000
	uint32_t peers_rwnd;
	sctp_assoc_t assoc_id;                  
	uint32_t extra_padding[32];              
#else
#endif
#else
	uint32_t peers_rwnd;
	sctp_assoc_t assoc_id;                  
	uint32_t extra_padding[32];              
#endif
};

struct xsctp_laddr {
	union sctp_sockstore address;    
	uint32_t last;
	struct sctp_timeval start_time;  
	uint32_t extra_padding[32];       
};

struct xsctp_raddr {
	union sctp_sockstore address;      
	uint32_t last;
	uint32_t rto;                      
	uint32_t max_path_rtx;             
	uint32_t rtx;                      
	uint32_t error_counter;            
	uint32_t cwnd;                     
	uint32_t flight_size;              
	uint32_t mtu;                      
	uint8_t active;                    
	uint8_t confirmed;                 
	uint8_t heartbeat_enabled;         
	uint8_t potentially_failed;
	struct sctp_timeval start_time;    
#if defined(__FreeBSD__)
#if __FreeBSD_version >= 800000
	uint32_t rtt;
	uint32_t heartbeat_interval;
	uint32_t extra_padding[31];              
#endif
#else
	uint32_t rtt;
	uint32_t heartbeat_interval;
	uint32_t extra_padding[31];              
#endif
};

#define SCTP_MAX_LOGGING_SIZE 30000
#define SCTP_TRACE_PARAMS 6                /* This number MUST be even   */

struct sctp_log_entry {
	uint64_t timestamp;
	uint32_t subsys;
	uint32_t padding;
	uint32_t params[SCTP_TRACE_PARAMS];
};

struct sctp_log {
	struct sctp_log_entry entry[SCTP_MAX_LOGGING_SIZE];
	uint32_t index;
	uint32_t padding;
};




#if defined(_KERNEL) || defined(__Userspace__)
int
sctp_lower_sosend(struct socket *so,
    struct sockaddr *addr,
    struct uio *uio,
#if defined(__Panda__)
    pakhandle_type i_pak,
    pakhandle_type i_control,
#else
    struct mbuf *i_pak,
    struct mbuf *control,
#endif
    int flags,
    struct sctp_sndrcvinfo *srcv
#if !(defined(__Panda__) || defined(__Userspace__))
#if defined(__FreeBSD__) && __FreeBSD_version >= 500000
    ,struct thread *p
#elif defined(__Windows__)
    , PKTHREAD p
#else
    ,struct proc *p
#endif
#endif
);

int
sctp_sorecvmsg(struct socket *so,
    struct uio *uio,
#if defined(__Panda__)
    particletype **mp,
#else
    struct mbuf **mp,
#endif
    struct sockaddr *from,
    int fromlen,
    int *msg_flags,
    struct sctp_sndrcvinfo *sinfo,
    int filling_sinfo);
#endif




#if !(defined(_KERNEL)) && !(defined(__Userspace__))

__BEGIN_DECLS
#if defined(__FreeBSD__) && __FreeBSD_version < 902000
int	sctp_peeloff __P((int, sctp_assoc_t));
int	sctp_bindx __P((int, struct sockaddr *, int, int));
int	sctp_connectx __P((int, const struct sockaddr *, int, sctp_assoc_t *));
int	sctp_getaddrlen __P((sa_family_t));
int	sctp_getpaddrs __P((int, sctp_assoc_t, struct sockaddr **));
void	sctp_freepaddrs __P((struct sockaddr *));
int	sctp_getladdrs __P((int, sctp_assoc_t, struct sockaddr **));
void	sctp_freeladdrs __P((struct sockaddr *));
int	sctp_opt_info __P((int, sctp_assoc_t, int, void *, socklen_t *));


ssize_t	sctp_sendmsg __P((int, const void *, size_t, const struct sockaddr *,
	    socklen_t, uint32_t, uint32_t, uint16_t, uint32_t, uint32_t));


ssize_t	sctp_send __P((int, const void *, size_t,
	    const struct sctp_sndrcvinfo *, int));


ssize_t	sctp_sendx __P((int, const void *, size_t, struct sockaddr *,
	    int, struct sctp_sndrcvinfo *, int));


ssize_t	sctp_sendmsgx __P((int sd, const void *, size_t, struct sockaddr *,
	    int, uint32_t, uint32_t, uint16_t, uint32_t, uint32_t));

sctp_assoc_t	sctp_getassocid __P((int, struct sockaddr *));


ssize_t	sctp_recvmsg __P((int, void *, size_t, struct sockaddr *, socklen_t *,
	    struct sctp_sndrcvinfo *, int *));

ssize_t	sctp_sendv __P((int, const struct iovec *, int, struct sockaddr *,
	    int, void *, socklen_t, unsigned int, int));

ssize_t	sctp_recvv __P((int, const struct iovec *, int, struct sockaddr *,
	    socklen_t *, void *, socklen_t *, unsigned int *, int *));
#else
int	sctp_peeloff(int, sctp_assoc_t);
int	sctp_bindx(int, struct sockaddr *, int, int);
int	sctp_connectx(int, const struct sockaddr *, int, sctp_assoc_t *);
int	sctp_getaddrlen(sa_family_t);
int	sctp_getpaddrs(int, sctp_assoc_t, struct sockaddr **);
void	sctp_freepaddrs(struct sockaddr *);
int	sctp_getladdrs(int, sctp_assoc_t, struct sockaddr **);
void	sctp_freeladdrs(struct sockaddr *);
int	sctp_opt_info(int, sctp_assoc_t, int, void *, socklen_t *);


ssize_t	sctp_sendmsg(int, const void *, size_t, const struct sockaddr *,
	    socklen_t, uint32_t, uint32_t, uint16_t, uint32_t, uint32_t);


ssize_t	sctp_send(int, const void *, size_t,
	    const struct sctp_sndrcvinfo *, int);


ssize_t	sctp_sendx(int, const void *, size_t, struct sockaddr *,
	    int, struct sctp_sndrcvinfo *, int);


ssize_t	sctp_sendmsgx(int sd, const void *, size_t, struct sockaddr *,
	    int, uint32_t, uint32_t, uint16_t, uint32_t, uint32_t);

sctp_assoc_t	sctp_getassocid(int, struct sockaddr *);


ssize_t	sctp_recvmsg(int, void *, size_t, struct sockaddr *, socklen_t *,
	    struct sctp_sndrcvinfo *, int *);

ssize_t	sctp_sendv(int, const struct iovec *, int, struct sockaddr *,
	    int, void *, socklen_t, unsigned int, int);

ssize_t	sctp_recvv(int, const struct iovec *, int, struct sockaddr *,
	    socklen_t *, void *, socklen_t *, unsigned int *, int *);
#endif
__END_DECLS

#endif				
#endif
