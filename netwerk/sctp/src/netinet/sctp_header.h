































#ifdef __FreeBSD__
#include <sys/cdefs.h>
__FBSDID("$FreeBSD: head/sys/netinet/sctp_header.h 240198 2012-09-07 13:36:42Z tuexen $");
#endif

#ifndef _NETINET_SCTP_HEADER_H_
#define _NETINET_SCTP_HEADER_H_

#if defined(__Windows__) && !defined(__Userspace_os_Windows)
#include <packon.h>
#endif
#if !defined(__Userspace_os_Windows)
#include <sys/time.h>
#endif
#include <netinet/sctp.h>
#include <netinet/sctp_constants.h>

#if !defined(__Userspace_os_Windows)
#define SCTP_PACKED __attribute__((packed))
#else
#pragma pack (push, 1)
#define SCTP_PACKED
#endif




struct sctp_ipv4addr_param {
	struct sctp_paramhdr ph;
	uint32_t addr;		
} SCTP_PACKED;

#define SCTP_V6_ADDR_BYTES 16


struct sctp_ipv6addr_param {
	struct sctp_paramhdr ph;
	uint8_t addr[SCTP_V6_ADDR_BYTES];	
} SCTP_PACKED;


struct sctp_cookie_perserve_param {
	struct sctp_paramhdr ph;
	uint32_t time;		
} SCTP_PACKED;

#define SCTP_ARRAY_MIN_LEN 1

struct sctp_host_name_param {
	struct sctp_paramhdr ph;
	char name[SCTP_ARRAY_MIN_LEN];		
} SCTP_PACKED;





#define SCTP_MAX_ADDR_PARAMS_SIZE 12

struct sctp_supported_addr_param {
	struct sctp_paramhdr ph;
	uint16_t addr_type[2];	
} SCTP_PACKED;


struct sctp_ecn_supported_param {
	struct sctp_paramhdr ph;
} SCTP_PACKED;



struct sctp_heartbeat_info_param {
	struct sctp_paramhdr ph;
	uint32_t time_value_1;
	uint32_t time_value_2;
	uint32_t random_value1;
	uint32_t random_value2;
	uint8_t addr_family;
	uint8_t addr_len;
	
	uint8_t padding[2];
	char address[SCTP_ADDRMAX];
} SCTP_PACKED;




struct sctp_prsctp_supported_param {
	struct sctp_paramhdr ph;
} SCTP_PACKED;



struct sctp_asconf_paramhdr {	
	struct sctp_paramhdr ph;
	uint32_t correlation_id;
} SCTP_PACKED;

struct sctp_asconf_addr_param {	
	struct sctp_asconf_paramhdr aph;	
	struct sctp_ipv6addr_param addrp;	
} SCTP_PACKED;


struct sctp_asconf_tag_param {	
	struct sctp_asconf_paramhdr aph;	
        uint32_t local_vtag;
        uint32_t remote_vtag;
} SCTP_PACKED;


struct sctp_asconf_addrv4_param {	
	struct sctp_asconf_paramhdr aph;	
	struct sctp_ipv4addr_param addrp;	
} SCTP_PACKED;

#define SCTP_MAX_SUPPORTED_EXT 256

struct sctp_supported_chunk_types_param {
	struct sctp_paramhdr ph;
	uint8_t chunk_types[];
} SCTP_PACKED;





struct sctp_data {
	uint32_t tsn;
	uint16_t stream_id;
	uint16_t stream_sequence;
	uint32_t protocol_id;
	
} SCTP_PACKED;

struct sctp_data_chunk {
	struct sctp_chunkhdr ch;
	struct sctp_data dp;
} SCTP_PACKED;






struct sctp_init {
	uint32_t initiate_tag;	
	uint32_t a_rwnd;	
	uint16_t num_outbound_streams;	
	uint16_t num_inbound_streams;	
	uint32_t initial_tsn;	
	
} SCTP_PACKED;
#define SCTP_IDENTIFICATION_SIZE 16
#define SCTP_ADDRESS_SIZE 4
#if defined(__Userspace__)
#define SCTP_RESERVE_SPACE 5
#else
#define SCTP_RESERVE_SPACE 6
#endif

struct sctp_state_cookie {	
	uint8_t identification[SCTP_IDENTIFICATION_SIZE];
	struct timeval time_entered;	
	uint32_t cookie_life;	
	uint32_t tie_tag_my_vtag;	

	uint32_t tie_tag_peer_vtag;	
	uint32_t peers_vtag;	

	uint32_t my_vtag;	
	uint32_t address[SCTP_ADDRESS_SIZE];	
	uint32_t addr_type;	
	uint32_t laddress[SCTP_ADDRESS_SIZE];	
	uint32_t laddr_type;	
	uint32_t scope_id;	

	uint16_t peerport;	
	uint16_t myport;	
	uint8_t ipv4_addr_legal;
	uint8_t ipv6_addr_legal;
#if defined(__Userspace__)
	uint8_t conn_addr_legal;
#endif
	uint8_t local_scope;	
	uint8_t site_scope;	

	uint8_t ipv4_scope;	
	uint8_t loopback_scope;	
	uint8_t reserved[SCTP_RESERVE_SPACE];    
	



} SCTP_PACKED;



struct sctp_missing_nat_state {
	uint16_t cause;
	uint16_t length;
        uint8_t data[];
} SCTP_PACKED;


struct sctp_inv_mandatory_param {
	uint16_t cause;
	uint16_t length;
	uint32_t num_param;
	uint16_t param;
	



	uint16_t resv;
} SCTP_PACKED;

struct sctp_unresolv_addr {
	uint16_t cause;
	uint16_t length;
	uint16_t addr_type;
	uint16_t reserved;	
} SCTP_PACKED;


struct sctp_state_cookie_param {
	struct sctp_paramhdr ph;
	struct sctp_state_cookie cookie;
} SCTP_PACKED;

struct sctp_init_chunk {
	struct sctp_chunkhdr ch;
	struct sctp_init init;
} SCTP_PACKED;

struct sctp_init_msg {
	struct sctphdr sh;
	struct sctp_init_chunk msg;
} SCTP_PACKED;


#define sctp_init_ack		sctp_init
#define sctp_init_ack_chunk	sctp_init_chunk
#define sctp_init_ack_msg	sctp_init_msg



struct sctp_gap_ack_block {
	uint16_t start;		
	uint16_t end;		
} SCTP_PACKED;

struct sctp_sack {
	uint32_t cum_tsn_ack;	
	uint32_t a_rwnd;	
	uint16_t num_gap_ack_blks;	
	uint16_t num_dup_tsns;	
	
	
} SCTP_PACKED;

struct sctp_sack_chunk {
	struct sctp_chunkhdr ch;
	struct sctp_sack sack;
} SCTP_PACKED;

struct sctp_nr_sack {
	uint32_t cum_tsn_ack;	
	uint32_t a_rwnd;	
	uint16_t num_gap_ack_blks;	
	uint16_t num_nr_gap_ack_blks;	
	uint16_t num_dup_tsns;	
	uint16_t reserved;	
	
	
} SCTP_PACKED;

struct sctp_nr_sack_chunk {
	struct sctp_chunkhdr ch;
	struct sctp_nr_sack nr_sack;
} SCTP_PACKED;



struct sctp_heartbeat {
	struct sctp_heartbeat_info_param hb_info;
} SCTP_PACKED;

struct sctp_heartbeat_chunk {
	struct sctp_chunkhdr ch;
	struct sctp_heartbeat heartbeat;
} SCTP_PACKED;


#define sctp_heartbeat_ack		sctp_heartbeat
#define sctp_heartbeat_ack_chunk	sctp_heartbeat_chunk



struct sctp_abort_chunk {
	struct sctp_chunkhdr ch;
	
} SCTP_PACKED;

struct sctp_abort_msg {
	struct sctphdr sh;
	struct sctp_abort_chunk msg;
} SCTP_PACKED;



struct sctp_shutdown_chunk {
	struct sctp_chunkhdr ch;
	uint32_t cumulative_tsn_ack;
} SCTP_PACKED;



struct sctp_shutdown_ack_chunk {
	struct sctp_chunkhdr ch;
} SCTP_PACKED;



struct sctp_error_chunk {
	struct sctp_chunkhdr ch;
	
} SCTP_PACKED;



struct sctp_cookie_echo_chunk {
	struct sctp_chunkhdr ch;
	struct sctp_state_cookie cookie;
} SCTP_PACKED;


struct sctp_cookie_ack_chunk {
	struct sctp_chunkhdr ch;
} SCTP_PACKED;


struct old_sctp_ecne_chunk {
	struct sctp_chunkhdr ch;
	uint32_t tsn;
} SCTP_PACKED;

struct sctp_ecne_chunk {
	struct sctp_chunkhdr ch;
	uint32_t tsn;
	uint32_t num_pkts_since_cwr;
} SCTP_PACKED;


struct sctp_cwr_chunk {
	struct sctp_chunkhdr ch;
	uint32_t tsn;
} SCTP_PACKED;


struct sctp_shutdown_complete_chunk {
	struct sctp_chunkhdr ch;
} SCTP_PACKED;


struct sctp_stale_cookie_msg {
	struct sctp_paramhdr ph;
	uint32_t time_usec;
} SCTP_PACKED;

struct sctp_adaptation_layer_indication {
	struct sctp_paramhdr ph;
	uint32_t indication;
} SCTP_PACKED;

struct sctp_cookie_while_shutting_down {
	struct sctphdr sh;
	struct sctp_chunkhdr ch;
	struct sctp_paramhdr ph;
} SCTP_PACKED;

struct sctp_shutdown_complete_msg {
	struct sctphdr sh;
	struct sctp_shutdown_complete_chunk shut_cmp;
} SCTP_PACKED;





struct sctp_asconf_chunk {
	struct sctp_chunkhdr ch;
	uint32_t serial_number;
	
	
} SCTP_PACKED;


struct sctp_asconf_ack_chunk {
	struct sctp_chunkhdr ch;
	uint32_t serial_number;
	
} SCTP_PACKED;



struct sctp_forward_tsn_chunk {
	struct sctp_chunkhdr ch;
	uint32_t new_cumulative_tsn;
	
} SCTP_PACKED;

struct sctp_strseq {
	uint16_t stream;
	uint16_t sequence;
} SCTP_PACKED;

struct sctp_forward_tsn_msg {
	struct sctphdr sh;
	struct sctp_forward_tsn_chunk msg;
} SCTP_PACKED;



#define SCTP_NUM_DB_TO_VERIFY 31

struct sctp_chunk_desc {
	uint8_t chunk_type;
	uint8_t data_bytes[SCTP_NUM_DB_TO_VERIFY];
	uint32_t tsn_ifany;
} SCTP_PACKED;


struct sctp_pktdrop_chunk {
	struct sctp_chunkhdr ch;
	uint32_t bottle_bw;
	uint32_t current_onq;
	uint16_t trunc_len;
	uint16_t reserved;
	uint8_t data[];
} SCTP_PACKED;



struct sctp_stream_reset_out_request {
	struct sctp_paramhdr ph;
	uint32_t request_seq;	
	uint32_t response_seq;	
	uint32_t send_reset_at_tsn;	
	uint16_t list_of_streams[];	
} SCTP_PACKED;

struct sctp_stream_reset_in_request {
	struct sctp_paramhdr ph;
	uint32_t request_seq;
	uint16_t list_of_streams[];	
} SCTP_PACKED;


struct sctp_stream_reset_tsn_request {
	struct sctp_paramhdr ph;
	uint32_t request_seq;
} SCTP_PACKED;

struct sctp_stream_reset_response {
	struct sctp_paramhdr ph;
	uint32_t response_seq;	
	uint32_t result;
} SCTP_PACKED;

struct sctp_stream_reset_response_tsn {
	struct sctp_paramhdr ph;
	uint32_t response_seq;	
	uint32_t result;
	uint32_t senders_next_tsn;
	uint32_t receivers_next_tsn;
} SCTP_PACKED;

struct sctp_stream_reset_add_strm {
  struct sctp_paramhdr ph;
  uint32_t request_seq;
  uint16_t number_of_streams;
  uint16_t reserved;
} SCTP_PACKED;

#define SCTP_STREAM_RESET_RESULT_NOTHING_TO_DO   0x00000000 /* XXX: unused */
#define SCTP_STREAM_RESET_RESULT_PERFORMED       0x00000001
#define SCTP_STREAM_RESET_RESULT_DENIED          0x00000002
#define SCTP_STREAM_RESET_RESULT_ERR__WRONG_SSN  0x00000003 /* XXX: unused */
#define SCTP_STREAM_RESET_RESULT_ERR_IN_PROGRESS 0x00000004
#define SCTP_STREAM_RESET_RESULT_ERR_BAD_SEQNO   0x00000005
#define SCTP_STREAM_RESET_RESULT_IN_PROGRESS     0x00000006 /* XXX: unused */






struct sctp_stream_reset_tsn_req {
	struct sctp_chunkhdr ch;
	struct sctp_stream_reset_tsn_request sr_req;
} SCTP_PACKED;

struct sctp_stream_reset_resp {
	struct sctp_chunkhdr ch;
	struct sctp_stream_reset_response sr_resp;
} SCTP_PACKED;


struct sctp_stream_reset_resp_tsn {
	struct sctp_chunkhdr ch;
	struct sctp_stream_reset_response_tsn sr_resp;
} SCTP_PACKED;








#define SCTP_RANDOM_MAX_SIZE 256
struct sctp_auth_random {
	struct sctp_paramhdr ph;
	uint8_t random_data[];
} SCTP_PACKED;

struct sctp_auth_chunk_list {
	struct sctp_paramhdr ph;
	uint8_t chunk_types[];
} SCTP_PACKED;

struct sctp_auth_hmac_algo {
	struct sctp_paramhdr ph;
	uint16_t hmac_ids[];
} SCTP_PACKED;

struct sctp_auth_chunk {
	struct sctp_chunkhdr ch;
	uint16_t shared_key_id;
	uint16_t hmac_id;
	uint8_t hmac[];
} SCTP_PACKED;

struct sctp_auth_invalid_hmac {
	struct sctp_paramhdr ph;
	uint16_t hmac_id;
	uint16_t padding;
} SCTP_PACKED;









#ifndef SCTP_MAX_OVERHEAD
#ifdef INET6
#define SCTP_MAX_OVERHEAD (sizeof(struct sctp_data_chunk) + \
			   sizeof(struct sctphdr) + \
			   sizeof(struct sctp_ecne_chunk) + \
			   sizeof(struct sctp_sack_chunk) + \
			   sizeof(struct ip6_hdr))

#define SCTP_MED_OVERHEAD (sizeof(struct sctp_data_chunk) + \
			   sizeof(struct sctphdr) + \
			   sizeof(struct ip6_hdr))


#define SCTP_MIN_OVERHEAD (sizeof(struct ip6_hdr) + \
			   sizeof(struct sctphdr))

#else
#define SCTP_MAX_OVERHEAD (sizeof(struct sctp_data_chunk) + \
			   sizeof(struct sctphdr) + \
			   sizeof(struct sctp_ecne_chunk) + \
			   sizeof(struct sctp_sack_chunk) + \
			   sizeof(struct ip))

#define SCTP_MED_OVERHEAD (sizeof(struct sctp_data_chunk) + \
			   sizeof(struct sctphdr) + \
			   sizeof(struct ip))


#define SCTP_MIN_OVERHEAD (sizeof(struct ip) + \
			   sizeof(struct sctphdr))

#endif 
#endif 

#define SCTP_MED_V4_OVERHEAD (sizeof(struct sctp_data_chunk) + \
			      sizeof(struct sctphdr) + \
			      sizeof(struct ip))

#define SCTP_MIN_V4_OVERHEAD (sizeof(struct ip) + \
			      sizeof(struct sctphdr))

#if defined(__Windows__)
#include <packoff.h>
#endif
#if defined(__Userspace_os_Windows)
#pragma pack ()
#endif
#undef SCTP_PACKED
#endif				
