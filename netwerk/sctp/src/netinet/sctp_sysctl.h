































#ifdef __FreeBSD__
#include <sys/cdefs.h>
__FBSDID("$FreeBSD: head/sys/netinet/sctp_sysctl.h 252779 2013-07-05 10:08:49Z tuexen $");
#endif

#ifndef _NETINET_SCTP_SYSCTL_H_
#define _NETINET_SCTP_SYSCTL_H_

#include <netinet/sctp_os.h>
#include <netinet/sctp_constants.h>

struct sctp_sysctl {
	uint32_t sctp_sendspace;
	uint32_t sctp_recvspace;
	uint32_t sctp_auto_asconf;
	uint32_t sctp_multiple_asconfs;
	uint32_t sctp_ecn_enable;
	uint32_t sctp_fr_max_burst_default;
	uint32_t sctp_strict_sacks;
#if !(defined(__FreeBSD__) && __FreeBSD_version >= 800000)
#if !defined(SCTP_WITH_NO_CSUM)
	uint32_t sctp_no_csum_on_loopback;
#endif
#endif
	uint32_t sctp_peer_chunk_oh;
	uint32_t sctp_max_burst_default;
	uint32_t sctp_max_chunks_on_queue;
	uint32_t sctp_hashtblsize;
	uint32_t sctp_pcbtblsize;
	uint32_t sctp_min_split_point;
	uint32_t sctp_chunkscale;
	uint32_t sctp_delayed_sack_time_default;
	uint32_t sctp_sack_freq_default;
	uint32_t sctp_system_free_resc_limit;
	uint32_t sctp_asoc_free_resc_limit;
	uint32_t sctp_heartbeat_interval_default;
	uint32_t sctp_pmtu_raise_time_default;
	uint32_t sctp_shutdown_guard_time_default;
	uint32_t sctp_secret_lifetime_default;
	uint32_t sctp_rto_max_default;
	uint32_t sctp_rto_min_default;
	uint32_t sctp_rto_initial_default;
	uint32_t sctp_init_rto_max_default;
	uint32_t sctp_valid_cookie_life_default;
	uint32_t sctp_init_rtx_max_default;
	uint32_t sctp_assoc_rtx_max_default;
	uint32_t sctp_path_rtx_max_default;
	uint32_t sctp_path_pf_threshold;
	uint32_t sctp_add_more_threshold;
	uint32_t sctp_nr_incoming_streams_default;
	uint32_t sctp_nr_outgoing_streams_default;
	uint32_t sctp_cmt_on_off;
	uint32_t sctp_cmt_use_dac;
	
	uint32_t sctp_nr_sack_on_off;
	uint32_t sctp_use_cwnd_based_maxburst;
	uint32_t sctp_asconf_auth_nochk;
	uint32_t sctp_auth_disable;
	uint32_t sctp_nat_friendly;
	uint32_t sctp_L2_abc_variable;
	uint32_t sctp_mbuf_threshold_count;
	uint32_t sctp_do_drain;
	uint32_t sctp_hb_maxburst;
	uint32_t sctp_abort_if_one_2_one_hits_limit;
	uint32_t sctp_strict_data_order;
	uint32_t sctp_min_residual;
	uint32_t sctp_max_retran_chunk;
	uint32_t sctp_logging_level;
	
	uint32_t sctp_default_cc_module;
	
	uint32_t sctp_default_ss_module;
	uint32_t sctp_default_frag_interleave;
	uint32_t sctp_mobility_base;
	uint32_t sctp_mobility_fasthandoff;
	uint32_t sctp_inits_include_nat_friendly;
	uint32_t sctp_rttvar_bw;
	uint32_t sctp_rttvar_rtt;
	uint32_t sctp_rttvar_eqret;
	uint32_t sctp_steady_step;
	uint32_t sctp_use_dccc_ecn;
#if defined(SCTP_LOCAL_TRACE_BUF)
#if defined(__Windows__)
	struct sctp_log *sctp_log;
#else
	struct sctp_log sctp_log;
#endif
#endif
	uint32_t sctp_udp_tunneling_port;
	uint32_t sctp_enable_sack_immediately;
	uint32_t sctp_vtag_time_wait;
	uint32_t sctp_buffer_splitting;
	uint32_t sctp_initial_cwnd;
	uint32_t sctp_blackhole;
#if defined(SCTP_DEBUG)
	uint32_t sctp_debug_on;
#endif
#if defined(__APPLE__)
	uint32_t sctp_ignore_vmware_interfaces;
	uint32_t sctp_main_timer;
	uint32_t sctp_addr_watchdog_limit;
	uint32_t sctp_vtag_watchdog_limit;
#endif
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
	uint32_t sctp_output_unlocked;
#endif
};





#define SCTPCTL_MAXDGRAM_DESC		"Maximum outgoing SCTP buffer size"
#define SCTPCTL_MAXDGRAM_MIN		0
#define SCTPCTL_MAXDGRAM_MAX		0xFFFFFFFF
#define SCTPCTL_MAXDGRAM_DEFAULT	262144	/* 256k */


#define SCTPCTL_RECVSPACE_DESC		"Maximum incoming SCTP buffer size"
#define SCTPCTL_RECVSPACE_MIN		0
#define SCTPCTL_RECVSPACE_MAX		0xFFFFFFFF
#define SCTPCTL_RECVSPACE_DEFAULT	262144	/* 256k */


#define SCTPCTL_AUTOASCONF_DESC		"Enable SCTP Auto-ASCONF"
#define SCTPCTL_AUTOASCONF_MIN		0
#define SCTPCTL_AUTOASCONF_MAX		1
#define SCTPCTL_AUTOASCONF_DEFAULT	SCTP_DEFAULT_AUTO_ASCONF


#define SCTPCTL_MULTIPLEASCONFS_DESC	"Enable SCTP Muliple-ASCONFs"
#define SCTPCTL_MULTIPLEASCONFS_MIN	0
#define SCTPCTL_MULTIPLEASCONFS_MAX	1
#define SCTPCTL_MULTIPLEASCONFS_DEFAULT	SCTP_DEFAULT_MULTIPLE_ASCONFS


#define SCTPCTL_ECN_ENABLE_DESC		"Enable SCTP ECN"
#define SCTPCTL_ECN_ENABLE_MIN		0
#define SCTPCTL_ECN_ENABLE_MAX		1
#define SCTPCTL_ECN_ENABLE_DEFAULT	1


#define SCTPCTL_STRICT_SACKS_DESC	"Enable SCTP Strict SACK checking"
#define SCTPCTL_STRICT_SACKS_MIN	0
#define SCTPCTL_STRICT_SACKS_MAX	1
#define SCTPCTL_STRICT_SACKS_DEFAULT	1


#define SCTPCTL_LOOPBACK_NOCSUM_DESC	"Enable NO Csum on packets sent on loopback"
#define SCTPCTL_LOOPBACK_NOCSUM_MIN	0
#define SCTPCTL_LOOPBACK_NOCSUM_MAX	1
#define SCTPCTL_LOOPBACK_NOCSUM_DEFAULT	1


#define SCTPCTL_PEER_CHKOH_DESC		"Amount to debit peers rwnd per chunk sent"
#define SCTPCTL_PEER_CHKOH_MIN		0
#define SCTPCTL_PEER_CHKOH_MAX		0xFFFFFFFF
#define SCTPCTL_PEER_CHKOH_DEFAULT	256


#define SCTPCTL_MAXBURST_DESC		"Default max burst for sctp endpoints"
#define SCTPCTL_MAXBURST_MIN		0
#define SCTPCTL_MAXBURST_MAX		0xFFFFFFFF
#define SCTPCTL_MAXBURST_DEFAULT	SCTP_DEF_MAX_BURST


#define SCTPCTL_FRMAXBURST_DESC		"Default fr max burst for sctp endpoints"
#define SCTPCTL_FRMAXBURST_MIN		0
#define SCTPCTL_FRMAXBURST_MAX		0xFFFFFFFF
#define SCTPCTL_FRMAXBURST_DEFAULT	SCTP_DEF_FRMAX_BURST



#define SCTPCTL_MAXCHUNKS_DESC		"Default max chunks on queue per asoc"
#define SCTPCTL_MAXCHUNKS_MIN		0
#define SCTPCTL_MAXCHUNKS_MAX		0xFFFFFFFF
#define SCTPCTL_MAXCHUNKS_DEFAULT	SCTP_ASOC_MAX_CHUNKS_ON_QUEUE


#define SCTPCTL_TCBHASHSIZE_DESC	"Tunable for TCB hash table sizes"
#define SCTPCTL_TCBHASHSIZE_MIN		1
#define SCTPCTL_TCBHASHSIZE_MAX		0xFFFFFFFF
#define SCTPCTL_TCBHASHSIZE_DEFAULT	SCTP_TCBHASHSIZE


#define SCTPCTL_PCBHASHSIZE_DESC	"Tunable for PCB hash table sizes"
#define SCTPCTL_PCBHASHSIZE_MIN		1
#define SCTPCTL_PCBHASHSIZE_MAX		0xFFFFFFFF
#define SCTPCTL_PCBHASHSIZE_DEFAULT	SCTP_PCBHASHSIZE


#define SCTPCTL_MIN_SPLIT_POINT_DESC	"Minimum size when splitting a chunk"
#define SCTPCTL_MIN_SPLIT_POINT_MIN	0
#define SCTPCTL_MIN_SPLIT_POINT_MAX	0xFFFFFFFF
#define SCTPCTL_MIN_SPLIT_POINT_DEFAULT	SCTP_DEFAULT_SPLIT_POINT_MIN


#define SCTPCTL_CHUNKSCALE_DESC		"Tunable for Scaling of number of chunks and messages"
#define SCTPCTL_CHUNKSCALE_MIN		1
#define SCTPCTL_CHUNKSCALE_MAX		0xFFFFFFFF
#define SCTPCTL_CHUNKSCALE_DEFAULT	SCTP_CHUNKQUEUE_SCALE


#define SCTPCTL_DELAYED_SACK_TIME_DESC	"Default delayed SACK timer in ms"
#define SCTPCTL_DELAYED_SACK_TIME_MIN	0
#define SCTPCTL_DELAYED_SACK_TIME_MAX	0xFFFFFFFF
#define SCTPCTL_DELAYED_SACK_TIME_DEFAULT	SCTP_RECV_MSEC


#define SCTPCTL_SACK_FREQ_DESC		"Default SACK frequency"
#define SCTPCTL_SACK_FREQ_MIN		0
#define SCTPCTL_SACK_FREQ_MAX		0xFFFFFFFF
#define SCTPCTL_SACK_FREQ_DEFAULT	SCTP_DEFAULT_SACK_FREQ


#define SCTPCTL_SYS_RESOURCE_DESC	"Max number of cached resources in the system"
#define SCTPCTL_SYS_RESOURCE_MIN	0
#define SCTPCTL_SYS_RESOURCE_MAX	0xFFFFFFFF
#define SCTPCTL_SYS_RESOURCE_DEFAULT	SCTP_DEF_SYSTEM_RESC_LIMIT


#define SCTPCTL_ASOC_RESOURCE_DESC	"Max number of cached resources in an asoc"
#define SCTPCTL_ASOC_RESOURCE_MIN	0
#define SCTPCTL_ASOC_RESOURCE_MAX	0xFFFFFFFF
#define SCTPCTL_ASOC_RESOURCE_DEFAULT	SCTP_DEF_ASOC_RESC_LIMIT


#define SCTPCTL_HEARTBEAT_INTERVAL_DESC	"Default heartbeat interval in ms"
#define SCTPCTL_HEARTBEAT_INTERVAL_MIN	0
#define SCTPCTL_HEARTBEAT_INTERVAL_MAX	0xFFFFFFFF
#define SCTPCTL_HEARTBEAT_INTERVAL_DEFAULT	SCTP_HB_DEFAULT_MSEC


#define SCTPCTL_PMTU_RAISE_TIME_DESC	"Default PMTU raise timer in seconds"
#define SCTPCTL_PMTU_RAISE_TIME_MIN	0
#define SCTPCTL_PMTU_RAISE_TIME_MAX	0xFFFFFFFF
#define SCTPCTL_PMTU_RAISE_TIME_DEFAULT	SCTP_DEF_PMTU_RAISE_SEC


#define SCTPCTL_SHUTDOWN_GUARD_TIME_DESC	"Default shutdown guard timer in seconds"
#define SCTPCTL_SHUTDOWN_GUARD_TIME_MIN		0
#define SCTPCTL_SHUTDOWN_GUARD_TIME_MAX		0xFFFFFFFF
#define SCTPCTL_SHUTDOWN_GUARD_TIME_DEFAULT	SCTP_DEF_MAX_SHUTDOWN_SEC


#define SCTPCTL_SECRET_LIFETIME_DESC	"Default secret lifetime in seconds"
#define SCTPCTL_SECRET_LIFETIME_MIN	0
#define SCTPCTL_SECRET_LIFETIME_MAX	0xFFFFFFFF
#define SCTPCTL_SECRET_LIFETIME_DEFAULT	SCTP_DEFAULT_SECRET_LIFE_SEC


#define SCTPCTL_RTO_MAX_DESC		"Default maximum retransmission timeout in ms"
#define SCTPCTL_RTO_MAX_MIN		0
#define SCTPCTL_RTO_MAX_MAX		0xFFFFFFFF
#define SCTPCTL_RTO_MAX_DEFAULT		SCTP_RTO_UPPER_BOUND


#define SCTPCTL_RTO_MIN_DESC		"Default minimum retransmission timeout in ms"
#define SCTPCTL_RTO_MIN_MIN		0
#define SCTPCTL_RTO_MIN_MAX		0xFFFFFFFF
#define SCTPCTL_RTO_MIN_DEFAULT		SCTP_RTO_LOWER_BOUND


#define SCTPCTL_RTO_INITIAL_DESC	"Default initial retransmission timeout in ms"
#define SCTPCTL_RTO_INITIAL_MIN		0
#define SCTPCTL_RTO_INITIAL_MAX		0xFFFFFFFF
#define SCTPCTL_RTO_INITIAL_DEFAULT	SCTP_RTO_INITIAL


#define SCTPCTL_INIT_RTO_MAX_DESC	"Default maximum retransmission timeout during association setup in ms"
#define SCTPCTL_INIT_RTO_MAX_MIN	0
#define SCTPCTL_INIT_RTO_MAX_MAX	0xFFFFFFFF
#define SCTPCTL_INIT_RTO_MAX_DEFAULT	SCTP_RTO_UPPER_BOUND


#define SCTPCTL_VALID_COOKIE_LIFE_DESC	"Default cookie lifetime in seconds"
#define SCTPCTL_VALID_COOKIE_LIFE_MIN	0
#define SCTPCTL_VALID_COOKIE_LIFE_MAX	0xFFFFFFFF
#define SCTPCTL_VALID_COOKIE_LIFE_DEFAULT	SCTP_DEFAULT_COOKIE_LIFE


#define SCTPCTL_INIT_RTX_MAX_DESC	"Default maximum number of retransmission for INIT chunks"
#define SCTPCTL_INIT_RTX_MAX_MIN	0
#define SCTPCTL_INIT_RTX_MAX_MAX	0xFFFFFFFF
#define SCTPCTL_INIT_RTX_MAX_DEFAULT	SCTP_DEF_MAX_INIT


#define SCTPCTL_ASSOC_RTX_MAX_DESC	"Default maximum number of retransmissions per association"
#define SCTPCTL_ASSOC_RTX_MAX_MIN	0
#define SCTPCTL_ASSOC_RTX_MAX_MAX	0xFFFFFFFF
#define SCTPCTL_ASSOC_RTX_MAX_DEFAULT	SCTP_DEF_MAX_SEND


#define SCTPCTL_PATH_RTX_MAX_DESC	"Default maximum of retransmissions per path"
#define SCTPCTL_PATH_RTX_MAX_MIN	0
#define SCTPCTL_PATH_RTX_MAX_MAX	0xFFFFFFFF
#define SCTPCTL_PATH_RTX_MAX_DEFAULT	SCTP_DEF_MAX_PATH_RTX


#define SCTPCTL_PATH_PF_THRESHOLD_DESC		"Default potentially failed threshold"
#define SCTPCTL_PATH_PF_THRESHOLD_MIN		0
#define SCTPCTL_PATH_PF_THRESHOLD_MAX		0xFFFF
#define SCTPCTL_PATH_PF_THRESHOLD_DEFAULT	SCTPCTL_PATH_PF_THRESHOLD_MAX


#define SCTPCTL_ADD_MORE_ON_OUTPUT_DESC	"When space-wise is it worthwhile to try to add more to a socket send buffer"
#define SCTPCTL_ADD_MORE_ON_OUTPUT_MIN	0
#define SCTPCTL_ADD_MORE_ON_OUTPUT_MAX	0xFFFFFFFF
#define SCTPCTL_ADD_MORE_ON_OUTPUT_DEFAULT SCTP_DEFAULT_ADD_MORE


#define SCTPCTL_INCOMING_STREAMS_DESC	"Default number of incoming streams"
#define SCTPCTL_INCOMING_STREAMS_MIN	1
#define SCTPCTL_INCOMING_STREAMS_MAX	65535
#define SCTPCTL_INCOMING_STREAMS_DEFAULT SCTP_ISTREAM_INITIAL


#define SCTPCTL_OUTGOING_STREAMS_DESC	"Default number of outgoing streams"
#define SCTPCTL_OUTGOING_STREAMS_MIN	1
#define SCTPCTL_OUTGOING_STREAMS_MAX	65535
#define SCTPCTL_OUTGOING_STREAMS_DEFAULT SCTP_OSTREAM_INITIAL


#define SCTPCTL_CMT_ON_OFF_DESC		"CMT settings"
#define SCTPCTL_CMT_ON_OFF_MIN		SCTP_CMT_OFF
#define SCTPCTL_CMT_ON_OFF_MAX		SCTP_CMT_MAX
#define SCTPCTL_CMT_ON_OFF_DEFAULT	SCTP_CMT_OFF


#define SCTPCTL_NR_SACK_ON_OFF_DESC	"NR_SACK on/off flag"
#define SCTPCTL_NR_SACK_ON_OFF_MIN	0
#define SCTPCTL_NR_SACK_ON_OFF_MAX	1
#define SCTPCTL_NR_SACK_ON_OFF_DEFAULT	0


#define SCTPCTL_CMT_USE_DAC_DESC	"CMT DAC on/off flag"
#define SCTPCTL_CMT_USE_DAC_MIN		0
#define SCTPCTL_CMT_USE_DAC_MAX		1
#define SCTPCTL_CMT_USE_DAC_DEFAULT    	0


#define SCTPCTL_CWND_MAXBURST_DESC	"Use a CWND adjusting maxburst"
#define SCTPCTL_CWND_MAXBURST_MIN	0
#define SCTPCTL_CWND_MAXBURST_MAX	1
#define SCTPCTL_CWND_MAXBURST_DEFAULT	1


#define SCTPCTL_ASCONF_AUTH_NOCHK_DESC	"Disable SCTP ASCONF AUTH requirement"
#define SCTPCTL_ASCONF_AUTH_NOCHK_MIN	0
#define SCTPCTL_ASCONF_AUTH_NOCHK_MAX	1
#define SCTPCTL_ASCONF_AUTH_NOCHK_DEFAULT	0


#define SCTPCTL_AUTH_DISABLE_DESC	"Disable SCTP AUTH function"
#define SCTPCTL_AUTH_DISABLE_MIN	0
#define SCTPCTL_AUTH_DISABLE_MAX	1
#define SCTPCTL_AUTH_DISABLE_DEFAULT	0


#define SCTPCTL_NAT_FRIENDLY_DESC	"SCTP NAT friendly operation"
#define SCTPCTL_NAT_FRIENDLY_MIN	0
#define SCTPCTL_NAT_FRIENDLY_MAX	1
#define SCTPCTL_NAT_FRIENDLY_DEFAULT	1


#define SCTPCTL_ABC_L_VAR_DESC		"SCTP ABC max increase per SACK (L)"
#define SCTPCTL_ABC_L_VAR_MIN		0
#define SCTPCTL_ABC_L_VAR_MAX		0xFFFFFFFF
#define SCTPCTL_ABC_L_VAR_DEFAULT	2


#define SCTPCTL_MAX_CHAINED_MBUFS_DESC	"Default max number of small mbufs on a chain"
#define SCTPCTL_MAX_CHAINED_MBUFS_MIN	0
#define SCTPCTL_MAX_CHAINED_MBUFS_MAX	0xFFFFFFFF
#define SCTPCTL_MAX_CHAINED_MBUFS_DEFAULT	SCTP_DEFAULT_MBUFS_IN_CHAIN


#define SCTPCTL_DO_SCTP_DRAIN_DESC	"Should SCTP respond to the drain calls"
#define SCTPCTL_DO_SCTP_DRAIN_MIN	0
#define SCTPCTL_DO_SCTP_DRAIN_MAX	1
#define SCTPCTL_DO_SCTP_DRAIN_DEFAULT	1


#define SCTPCTL_HB_MAX_BURST_DESC	"Confirmation Heartbeat max burst"
#define SCTPCTL_HB_MAX_BURST_MIN	1
#define SCTPCTL_HB_MAX_BURST_MAX	0xFFFFFFFF
#define SCTPCTL_HB_MAX_BURST_DEFAULT	SCTP_DEF_HBMAX_BURST


#define SCTPCTL_ABORT_AT_LIMIT_DESC	"When one-2-one hits qlimit abort"
#define SCTPCTL_ABORT_AT_LIMIT_MIN	0
#define SCTPCTL_ABORT_AT_LIMIT_MAX	1
#define SCTPCTL_ABORT_AT_LIMIT_DEFAULT	0


#define SCTPCTL_STRICT_DATA_ORDER_DESC	"Enforce strict data ordering, abort if control inside data"
#define SCTPCTL_STRICT_DATA_ORDER_MIN	0
#define SCTPCTL_STRICT_DATA_ORDER_MAX	1
#define SCTPCTL_STRICT_DATA_ORDER_DEFAULT	0


#define SCTPCTL_MIN_RESIDUAL_DESC	"Minimum residual data chunk in second part of split"
#define SCTPCTL_MIN_RESIDUAL_MIN	20
#define SCTPCTL_MIN_RESIDUAL_MAX	65535
#define SCTPCTL_MIN_RESIDUAL_DEFAULT	1452


#define SCTPCTL_MAX_RETRAN_CHUNK_DESC	"Maximum times an unlucky chunk can be retran'd before assoc abort"
#define SCTPCTL_MAX_RETRAN_CHUNK_MIN	0
#define SCTPCTL_MAX_RETRAN_CHUNK_MAX	65535
#define SCTPCTL_MAX_RETRAN_CHUNK_DEFAULT	30


#define SCTPCTL_LOGGING_LEVEL_DESC	"Ltrace/KTR trace logging level"
#define SCTPCTL_LOGGING_LEVEL_MIN	0
#define SCTPCTL_LOGGING_LEVEL_MAX	0xffffffff
#define SCTPCTL_LOGGING_LEVEL_DEFAULT	0


#define SCTPCTL_DEFAULT_CC_MODULE_DESC		"Default congestion control module"
#define SCTPCTL_DEFAULT_CC_MODULE_MIN		0
#define SCTPCTL_DEFAULT_CC_MODULE_MAX		2
#define SCTPCTL_DEFAULT_CC_MODULE_DEFAULT	0


#define SCTPCTL_DEFAULT_SS_MODULE_DESC		"Default stream scheduling module"
#define SCTPCTL_DEFAULT_SS_MODULE_MIN		0
#define SCTPCTL_DEFAULT_SS_MODULE_MAX		5
#define SCTPCTL_DEFAULT_SS_MODULE_DEFAULT	0


#define SCTPCTL_DEFAULT_FRAG_INTERLEAVE_DESC	"Default fragment interleave level"
#define SCTPCTL_DEFAULT_FRAG_INTERLEAVE_MIN	0
#define SCTPCTL_DEFAULT_FRAG_INTERLEAVE_MAX	2
#define SCTPCTL_DEFAULT_FRAG_INTERLEAVE_DEFAULT	1


#define SCTPCTL_MOBILITY_BASE_DESC	"Enable SCTP base mobility"
#define SCTPCTL_MOBILITY_BASE_MIN	0
#define SCTPCTL_MOBILITY_BASE_MAX	1
#define SCTPCTL_MOBILITY_BASE_DEFAULT	SCTP_DEFAULT_MOBILITY_BASE


#define SCTPCTL_MOBILITY_FASTHANDOFF_DESC	"Enable SCTP fast handoff"
#define SCTPCTL_MOBILITY_FASTHANDOFF_MIN	0
#define SCTPCTL_MOBILITY_FASTHANDOFF_MAX	1
#define SCTPCTL_MOBILITY_FASTHANDOFF_DEFAULT	SCTP_DEFAULT_MOBILITY_FASTHANDOFF


#define SCTPCTL_UDP_TUNNELING_PORT_DESC		"Set the SCTP/UDP tunneling port"
#define SCTPCTL_UDP_TUNNELING_PORT_MIN		0
#define SCTPCTL_UDP_TUNNELING_PORT_MAX		65535
#define SCTPCTL_UDP_TUNNELING_PORT_DEFAULT	SCTP_OVER_UDP_TUNNELING_PORT


#define SCTPCTL_SACK_IMMEDIATELY_ENABLE_DESC	"Enable sending of the SACK-IMMEDIATELY-bit."
#define SCTPCTL_SACK_IMMEDIATELY_ENABLE_MIN	0
#define SCTPCTL_SACK_IMMEDIATELY_ENABLE_MAX	1
#define SCTPCTL_SACK_IMMEDIATELY_ENABLE_DEFAULT	SCTPCTL_SACK_IMMEDIATELY_ENABLE_MIN


#define SCTPCTL_NAT_FRIENDLY_INITS_DESC	"Enable sending of the nat-friendly SCTP option on INITs."
#define SCTPCTL_NAT_FRIENDLY_INITS_MIN	0
#define SCTPCTL_NAT_FRIENDLY_INITS_MAX	1
#define SCTPCTL_NAT_FRIENDLY_INITS_DEFAULT	SCTPCTL_NAT_FRIENDLY_INITS_MIN


#define SCTPCTL_TIME_WAIT_DESC	"Vtag time wait time in seconds, 0 disables it."
#define SCTPCTL_TIME_WAIT_MIN	0
#define SCTPCTL_TIME_WAIT_MAX	0xffffffff
#define SCTPCTL_TIME_WAIT_DEFAULT	SCTP_TIME_WAIT


#define SCTPCTL_BUFFER_SPLITTING_DESC		"Enable send/receive buffer splitting."
#define SCTPCTL_BUFFER_SPLITTING_MIN		0
#define SCTPCTL_BUFFER_SPLITTING_MAX		0x3
#define SCTPCTL_BUFFER_SPLITTING_DEFAULT	SCTPCTL_BUFFER_SPLITTING_MIN


#define SCTPCTL_INITIAL_CWND_DESC	"Initial congestion window in MTUs"
#define SCTPCTL_INITIAL_CWND_MIN	0
#define SCTPCTL_INITIAL_CWND_MAX	0xffffffff
#define SCTPCTL_INITIAL_CWND_DEFAULT	3


#define SCTPCTL_RTTVAR_BW_DESC	"Shift amount for bw smoothing on rtt calc"
#define SCTPCTL_RTTVAR_BW_MIN	0
#define SCTPCTL_RTTVAR_BW_MAX	32
#define SCTPCTL_RTTVAR_BW_DEFAULT	4


#define SCTPCTL_RTTVAR_RTT_DESC	"Shift amount for rtt smoothing on rtt calc"
#define SCTPCTL_RTTVAR_RTT_MIN	0
#define SCTPCTL_RTTVAR_RTT_MAX	32
#define SCTPCTL_RTTVAR_RTT_DEFAULT	5

#define SCTPCTL_RTTVAR_EQRET_DESC	"What to return when rtt and bw are unchanged"
#define SCTPCTL_RTTVAR_EQRET_MIN	0
#define SCTPCTL_RTTVAR_EQRET_MAX	1
#define SCTPCTL_RTTVAR_EQRET_DEFAULT	0

#define SCTPCTL_RTTVAR_STEADYS_DESC	"How many the sames it takes to try step down of cwnd"
#define SCTPCTL_RTTVAR_STEADYS_MIN	0
#define SCTPCTL_RTTVAR_STEADYS_MAX	0xFFFF
#define SCTPCTL_RTTVAR_STEADYS_DEFAULT	20 /* 0 means disable feature */

#define SCTPCTL_RTTVAR_DCCCECN_DESC	"Enable for RTCC CC datacenter ECN"
#define SCTPCTL_RTTVAR_DCCCECN_MIN	0
#define SCTPCTL_RTTVAR_DCCCECN_MAX	1
#define SCTPCTL_RTTVAR_DCCCECN_DEFAULT	1 /* 0 means disable feature */

#define SCTPCTL_BLACKHOLE_DESC		"Enable SCTP blackholing"
#define SCTPCTL_BLACKHOLE_MIN		0
#define SCTPCTL_BLACKHOLE_MAX		2
#define SCTPCTL_BLACKHOLE_DEFAULT	SCTPCTL_BLACKHOLE_MIN

#if defined(SCTP_DEBUG)

#define SCTPCTL_DEBUG_DESC	"Configure debug output"
#define SCTPCTL_DEBUG_MIN	0
#define SCTPCTL_DEBUG_MAX	0xFFFFFFFF
#define SCTPCTL_DEBUG_DEFAULT	0
#endif

#if defined(__APPLE__)
#define SCTPCTL_MAIN_TIMER_DESC		"Main timer interval in ms"
#define SCTPCTL_MAIN_TIMER_MIN		1
#define SCTPCTL_MAIN_TIMER_MAX		0xFFFFFFFF
#define SCTPCTL_MAIN_TIMER_DEFAULT	10

#define SCTPCTL_IGNORE_VMWARE_INTERFACES_DESC		"Ignore VMware Interfaces"
#define SCTPCTL_IGNORE_VMWARE_INTERFACES_MIN		0
#define SCTPCTL_IGNORE_VMWARE_INTERFACES_MAX		1
#define SCTPCTL_IGNORE_VMWARE_INTERFACES_DEFAULT	SCTPCTL_IGNORE_VMWARE_INTERFACES_MAX
#endif

#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
#define SCTPCTL_OUTPUT_UNLOCKED_DESC	"Unlock socket when sending packets down to IP."
#define SCTPCTL_OUTPUT_UNLOCKED_MIN	0
#define SCTPCTL_OUTPUT_UNLOCKED_MAX	1
#define SCTPCTL_OUTPUT_UNLOCKED_DEFAULT	SCTPCTL_OUTPUT_UNLOCKED_MIN
#endif

#if defined(__APPLE__)
#define	SCTPCTL_ADDR_WATCHDOG_LIMIT_MIN	0
#define	SCTPCTL_ADDR_WATCHDOG_LIMIT_MAX	0xFFFFFFFF
#define	SCTPCTL_ADDR_WATCHDOG_LIMIT_DEFAULT	SCTPCTL_ADDR_WATCHDOG_LIMIT_MIN

#define	SCTPCTL_VTAG_WATCHDOG_LIMIT_MIN	0
#define	SCTPCTL_VTAG_WATCHDOG_LIMIT_MAX	0xFFFFFFFF
#define	SCTPCTL_VTAG_WATCHDOG_LIMIT_DEFAULT	SCTPCTL_VTAG_WATCHDOG_LIMIT_MIN
#endif

#if defined(_KERNEL) || defined(__Userspace__)
#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__Userspace__)
#if defined(SYSCTL_DECL)
SYSCTL_DECL(_net_inet_sctp);
#endif
#endif

void sctp_init_sysctls(void);
#if defined(__Windows__)
void sctp_finish_sysctls(void);
#endif

#endif 
#endif 
