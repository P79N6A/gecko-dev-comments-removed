































#ifdef __FreeBSD__
#include <sys/cdefs.h>
__FBSDID("$FreeBSD: head/sys/netinet/sctp_output.c 246687 2013-02-11 21:02:49Z tuexen $");
#endif

#include <netinet/sctp_os.h>
#ifdef __FreeBSD__
#include <sys/proc.h>
#endif
#include <netinet/sctp_var.h>
#include <netinet/sctp_sysctl.h>
#include <netinet/sctp_header.h>
#include <netinet/sctp_pcb.h>
#include <netinet/sctputil.h>
#include <netinet/sctp_output.h>
#include <netinet/sctp_uio.h>
#include <netinet/sctputil.h>
#include <netinet/sctp_auth.h>
#include <netinet/sctp_timer.h>
#include <netinet/sctp_asconf.h>
#include <netinet/sctp_indata.h>
#include <netinet/sctp_bsd_addr.h>
#include <netinet/sctp_input.h>
#include <netinet/sctp_crc32.h>
#if defined(__Userspace_os_Linux)
#define __FAVOR_BSD
#endif
#if !defined(__Userspace_os_Windows)
#include <netinet/udp.h>
#endif
#if defined(__APPLE__)
#include <netinet/in.h>
#endif
#if defined(__FreeBSD__)
#if defined(__FreeBSD__) && __FreeBSD_version >= 800000
#include <netinet/udp_var.h>
#endif
#include <machine/in_cksum.h>
#endif
#if defined(__Userspace__) && defined(INET6)
#include <netinet6/sctp6_var.h>
#endif

#if defined(__APPLE__)
#define APPLE_FILE_NO 3
#endif

#if defined(__APPLE__)
#if !(defined(APPLE_LEOPARD) || defined(APPLE_SNOWLEOPARD))
#define SCTP_MAX_LINKHDR 16
#endif
#endif

#define SCTP_MAX_GAPS_INARRAY 4
struct sack_track {
	uint8_t right_edge;	
	uint8_t left_edge;	
	uint8_t num_entries;
	uint8_t spare;
	struct sctp_gap_ack_block gaps[SCTP_MAX_GAPS_INARRAY];
};

struct sack_track sack_array[256] = {
	{0, 0, 0, 0,		
		{{0, 0},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 1, 0,		
		{{0, 0},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 1, 0,		
		{{1, 1},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 1, 0,		
		{{0, 1},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 1, 0,		
		{{2, 2},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 0},
		{2, 2},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 1, 0,		
		{{1, 2},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 1, 0,		
		{{0, 2},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 1, 0,		
		{{3, 3},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 0},
		{3, 3},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{1, 1},
		{3, 3},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 1},
		{3, 3},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 1, 0,		
		{{2, 3},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 0},
		{2, 3},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 1, 0,		
		{{1, 3},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 1, 0,		
		{{0, 3},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 1, 0,		
		{{4, 4},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 0},
		{4, 4},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{1, 1},
		{4, 4},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 1},
		{4, 4},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{2, 2},
		{4, 4},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 3, 0,		
		{{0, 0},
		{2, 2},
		{4, 4},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{1, 2},
		{4, 4},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 2},
		{4, 4},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 1, 0,		
		{{3, 4},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 0},
		{3, 4},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{1, 1},
		{3, 4},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 1},
		{3, 4},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 1, 0,		
		{{2, 4},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 0},
		{2, 4},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 1, 0,		
		{{1, 4},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 1, 0,		
		{{0, 4},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 1, 0,		
		{{5, 5},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 0},
		{5, 5},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{1, 1},
		{5, 5},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 1},
		{5, 5},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{2, 2},
		{5, 5},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 3, 0,		
		{{0, 0},
		{2, 2},
		{5, 5},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{1, 2},
		{5, 5},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 2},
		{5, 5},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{3, 3},
		{5, 5},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 3, 0,		
		{{0, 0},
		{3, 3},
		{5, 5},
		{0, 0}
		}
	},
	{0, 0, 3, 0,		
		{{1, 1},
		{3, 3},
		{5, 5},
		{0, 0}
		}
	},
	{1, 0, 3, 0,		
		{{0, 1},
		{3, 3},
		{5, 5},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{2, 3},
		{5, 5},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 3, 0,		
		{{0, 0},
		{2, 3},
		{5, 5},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{1, 3},
		{5, 5},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 3},
		{5, 5},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 1, 0,		
		{{4, 5},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 0},
		{4, 5},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{1, 1},
		{4, 5},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 1},
		{4, 5},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{2, 2},
		{4, 5},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 3, 0,		
		{{0, 0},
		{2, 2},
		{4, 5},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{1, 2},
		{4, 5},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 2},
		{4, 5},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 1, 0,		
		{{3, 5},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 0},
		{3, 5},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{1, 1},
		{3, 5},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 1},
		{3, 5},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 1, 0,		
		{{2, 5},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 0},
		{2, 5},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 1, 0,		
		{{1, 5},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 1, 0,		
		{{0, 5},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 1, 0,		
		{{6, 6},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 0},
		{6, 6},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{1, 1},
		{6, 6},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 1},
		{6, 6},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{2, 2},
		{6, 6},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 3, 0,		
		{{0, 0},
		{2, 2},
		{6, 6},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{1, 2},
		{6, 6},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 2},
		{6, 6},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{3, 3},
		{6, 6},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 3, 0,		
		{{0, 0},
		{3, 3},
		{6, 6},
		{0, 0}
		}
	},
	{0, 0, 3, 0,		
		{{1, 1},
		{3, 3},
		{6, 6},
		{0, 0}
		}
	},
	{1, 0, 3, 0,		
		{{0, 1},
		{3, 3},
		{6, 6},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{2, 3},
		{6, 6},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 3, 0,		
		{{0, 0},
		{2, 3},
		{6, 6},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{1, 3},
		{6, 6},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 3},
		{6, 6},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{4, 4},
		{6, 6},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 3, 0,		
		{{0, 0},
		{4, 4},
		{6, 6},
		{0, 0}
		}
	},
	{0, 0, 3, 0,		
		{{1, 1},
		{4, 4},
		{6, 6},
		{0, 0}
		}
	},
	{1, 0, 3, 0,		
		{{0, 1},
		{4, 4},
		{6, 6},
		{0, 0}
		}
	},
	{0, 0, 3, 0,		
		{{2, 2},
		{4, 4},
		{6, 6},
		{0, 0}
		}
	},
	{1, 0, 4, 0,		
		{{0, 0},
		{2, 2},
		{4, 4},
		{6, 6}
		}
	},
	{0, 0, 3, 0,		
		{{1, 2},
		{4, 4},
		{6, 6},
		{0, 0}
		}
	},
	{1, 0, 3, 0,		
		{{0, 2},
		{4, 4},
		{6, 6},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{3, 4},
		{6, 6},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 3, 0,		
		{{0, 0},
		{3, 4},
		{6, 6},
		{0, 0}
		}
	},
	{0, 0, 3, 0,		
		{{1, 1},
		{3, 4},
		{6, 6},
		{0, 0}
		}
	},
	{1, 0, 3, 0,		
		{{0, 1},
		{3, 4},
		{6, 6},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{2, 4},
		{6, 6},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 3, 0,		
		{{0, 0},
		{2, 4},
		{6, 6},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{1, 4},
		{6, 6},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 4},
		{6, 6},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 1, 0,		
		{{5, 6},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 0},
		{5, 6},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{1, 1},
		{5, 6},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 1},
		{5, 6},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{2, 2},
		{5, 6},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 3, 0,		
		{{0, 0},
		{2, 2},
		{5, 6},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{1, 2},
		{5, 6},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 2},
		{5, 6},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{3, 3},
		{5, 6},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 3, 0,		
		{{0, 0},
		{3, 3},
		{5, 6},
		{0, 0}
		}
	},
	{0, 0, 3, 0,		
		{{1, 1},
		{3, 3},
		{5, 6},
		{0, 0}
		}
	},
	{1, 0, 3, 0,		
		{{0, 1},
		{3, 3},
		{5, 6},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{2, 3},
		{5, 6},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 3, 0,		
		{{0, 0},
		{2, 3},
		{5, 6},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{1, 3},
		{5, 6},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 3},
		{5, 6},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 1, 0,		
		{{4, 6},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 0},
		{4, 6},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{1, 1},
		{4, 6},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 1},
		{4, 6},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{2, 2},
		{4, 6},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 3, 0,		
		{{0, 0},
		{2, 2},
		{4, 6},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{1, 2},
		{4, 6},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 2},
		{4, 6},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 1, 0,		
		{{3, 6},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 0},
		{3, 6},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 2, 0,		
		{{1, 1},
		{3, 6},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 1},
		{3, 6},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 1, 0,		
		{{2, 6},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 2, 0,		
		{{0, 0},
		{2, 6},
		{0, 0},
		{0, 0}
		}
	},
	{0, 0, 1, 0,		
		{{1, 6},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{1, 0, 1, 0,		
		{{0, 6},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{0, 1, 1, 0,		
		{{7, 7},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 2, 0,		
		{{0, 0},
		{7, 7},
		{0, 0},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{1, 1},
		{7, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 2, 0,		
		{{0, 1},
		{7, 7},
		{0, 0},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{2, 2},
		{7, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 0},
		{2, 2},
		{7, 7},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{1, 2},
		{7, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 2, 0,		
		{{0, 2},
		{7, 7},
		{0, 0},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{3, 3},
		{7, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 0},
		{3, 3},
		{7, 7},
		{0, 0}
		}
	},
	{0, 1, 3, 0,		
		{{1, 1},
		{3, 3},
		{7, 7},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 1},
		{3, 3},
		{7, 7},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{2, 3},
		{7, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 0},
		{2, 3},
		{7, 7},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{1, 3},
		{7, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 2, 0,		
		{{0, 3},
		{7, 7},
		{0, 0},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{4, 4},
		{7, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 0},
		{4, 4},
		{7, 7},
		{0, 0}
		}
	},
	{0, 1, 3, 0,		
		{{1, 1},
		{4, 4},
		{7, 7},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 1},
		{4, 4},
		{7, 7},
		{0, 0}
		}
	},
	{0, 1, 3, 0,		
		{{2, 2},
		{4, 4},
		{7, 7},
		{0, 0}
		}
	},
	{1, 1, 4, 0,		
		{{0, 0},
		{2, 2},
		{4, 4},
		{7, 7}
		}
	},
	{0, 1, 3, 0,		
		{{1, 2},
		{4, 4},
		{7, 7},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 2},
		{4, 4},
		{7, 7},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{3, 4},
		{7, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 0},
		{3, 4},
		{7, 7},
		{0, 0}
		}
	},
	{0, 1, 3, 0,		
		{{1, 1},
		{3, 4},
		{7, 7},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 1},
		{3, 4},
		{7, 7},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{2, 4},
		{7, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 0},
		{2, 4},
		{7, 7},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{1, 4},
		{7, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 2, 0,		
		{{0, 4},
		{7, 7},
		{0, 0},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{5, 5},
		{7, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 0},
		{5, 5},
		{7, 7},
		{0, 0}
		}
	},
	{0, 1, 3, 0,		
		{{1, 1},
		{5, 5},
		{7, 7},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 1},
		{5, 5},
		{7, 7},
		{0, 0}
		}
	},
	{0, 1, 3, 0,		
		{{2, 2},
		{5, 5},
		{7, 7},
		{0, 0}
		}
	},
	{1, 1, 4, 0,		
		{{0, 0},
		{2, 2},
		{5, 5},
		{7, 7}
		}
	},
	{0, 1, 3, 0,		
		{{1, 2},
		{5, 5},
		{7, 7},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 2},
		{5, 5},
		{7, 7},
		{0, 0}
		}
	},
	{0, 1, 3, 0,		
		{{3, 3},
		{5, 5},
		{7, 7},
		{0, 0}
		}
	},
	{1, 1, 4, 0,		
		{{0, 0},
		{3, 3},
		{5, 5},
		{7, 7}
		}
	},
	{0, 1, 4, 0,		
		{{1, 1},
		{3, 3},
		{5, 5},
		{7, 7}
		}
	},
	{1, 1, 4, 0,		
		{{0, 1},
		{3, 3},
		{5, 5},
		{7, 7}
		}
	},
	{0, 1, 3, 0,		
		{{2, 3},
		{5, 5},
		{7, 7},
		{0, 0}
		}
	},
	{1, 1, 4, 0,		
		{{0, 0},
		{2, 3},
		{5, 5},
		{7, 7}
		}
	},
	{0, 1, 3, 0,		
		{{1, 3},
		{5, 5},
		{7, 7},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 3},
		{5, 5},
		{7, 7},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{4, 5},
		{7, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 0},
		{4, 5},
		{7, 7},
		{0, 0}
		}
	},
	{0, 1, 3, 0,		
		{{1, 1},
		{4, 5},
		{7, 7},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 1},
		{4, 5},
		{7, 7},
		{0, 0}
		}
	},
	{0, 1, 3, 0,		
		{{2, 2},
		{4, 5},
		{7, 7},
		{0, 0}
		}
	},
	{1, 1, 4, 0,		
		{{0, 0},
		{2, 2},
		{4, 5},
		{7, 7}
		}
	},
	{0, 1, 3, 0,		
		{{1, 2},
		{4, 5},
		{7, 7},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 2},
		{4, 5},
		{7, 7},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{3, 5},
		{7, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 0},
		{3, 5},
		{7, 7},
		{0, 0}
		}
	},
	{0, 1, 3, 0,		
		{{1, 1},
		{3, 5},
		{7, 7},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 1},
		{3, 5},
		{7, 7},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{2, 5},
		{7, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 0},
		{2, 5},
		{7, 7},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{1, 5},
		{7, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 2, 0,		
		{{0, 5},
		{7, 7},
		{0, 0},
		{0, 0}
		}
	},
	{0, 1, 1, 0,		
		{{6, 7},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 2, 0,		
		{{0, 0},
		{6, 7},
		{0, 0},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{1, 1},
		{6, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 2, 0,		
		{{0, 1},
		{6, 7},
		{0, 0},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{2, 2},
		{6, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 0},
		{2, 2},
		{6, 7},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{1, 2},
		{6, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 2, 0,		
		{{0, 2},
		{6, 7},
		{0, 0},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{3, 3},
		{6, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 0},
		{3, 3},
		{6, 7},
		{0, 0}
		}
	},
	{0, 1, 3, 0,		
		{{1, 1},
		{3, 3},
		{6, 7},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 1},
		{3, 3},
		{6, 7},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{2, 3},
		{6, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 0},
		{2, 3},
		{6, 7},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{1, 3},
		{6, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 2, 0,		
		{{0, 3},
		{6, 7},
		{0, 0},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{4, 4},
		{6, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 0},
		{4, 4},
		{6, 7},
		{0, 0}
		}
	},
	{0, 1, 3, 0,		
		{{1, 1},
		{4, 4},
		{6, 7},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 1},
		{4, 4},
		{6, 7},
		{0, 0}
		}
	},
	{0, 1, 3, 0,		
		{{2, 2},
		{4, 4},
		{6, 7},
		{0, 0}
		}
	},
	{1, 1, 4, 0,		
		{{0, 0},
		{2, 2},
		{4, 4},
		{6, 7}
		}
	},
	{0, 1, 3, 0,		
		{{1, 2},
		{4, 4},
		{6, 7},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 2},
		{4, 4},
		{6, 7},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{3, 4},
		{6, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 0},
		{3, 4},
		{6, 7},
		{0, 0}
		}
	},
	{0, 1, 3, 0,		
		{{1, 1},
		{3, 4},
		{6, 7},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 1},
		{3, 4},
		{6, 7},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{2, 4},
		{6, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 0},
		{2, 4},
		{6, 7},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{1, 4},
		{6, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 2, 0,		
		{{0, 4},
		{6, 7},
		{0, 0},
		{0, 0}
		}
	},
	{0, 1, 1, 0,		
		{{5, 7},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 2, 0,		
		{{0, 0},
		{5, 7},
		{0, 0},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{1, 1},
		{5, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 2, 0,		
		{{0, 1},
		{5, 7},
		{0, 0},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{2, 2},
		{5, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 0},
		{2, 2},
		{5, 7},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{1, 2},
		{5, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 2, 0,		
		{{0, 2},
		{5, 7},
		{0, 0},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{3, 3},
		{5, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 0},
		{3, 3},
		{5, 7},
		{0, 0}
		}
	},
	{0, 1, 3, 0,		
		{{1, 1},
		{3, 3},
		{5, 7},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 1},
		{3, 3},
		{5, 7},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{2, 3},
		{5, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 0},
		{2, 3},
		{5, 7},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{1, 3},
		{5, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 2, 0,		
		{{0, 3},
		{5, 7},
		{0, 0},
		{0, 0}
		}
	},
	{0, 1, 1, 0,		
		{{4, 7},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 2, 0,		
		{{0, 0},
		{4, 7},
		{0, 0},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{1, 1},
		{4, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 2, 0,		
		{{0, 1},
		{4, 7},
		{0, 0},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{2, 2},
		{4, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 3, 0,		
		{{0, 0},
		{2, 2},
		{4, 7},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{1, 2},
		{4, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 2, 0,		
		{{0, 2},
		{4, 7},
		{0, 0},
		{0, 0}
		}
	},
	{0, 1, 1, 0,		
		{{3, 7},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 2, 0,		
		{{0, 0},
		{3, 7},
		{0, 0},
		{0, 0}
		}
	},
	{0, 1, 2, 0,		
		{{1, 1},
		{3, 7},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 2, 0,		
		{{0, 1},
		{3, 7},
		{0, 0},
		{0, 0}
		}
	},
	{0, 1, 1, 0,		
		{{2, 7},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 2, 0,		
		{{0, 0},
		{2, 7},
		{0, 0},
		{0, 0}
		}
	},
	{0, 1, 1, 0,		
		{{1, 7},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	},
	{1, 1, 1, 0,		
		{{0, 7},
		{0, 0},
		{0, 0},
		{0, 0}
		}
	}
};


int
sctp_is_address_in_scope(struct sctp_ifa *ifa,
                         struct sctp_scoping *scope,
                         int do_update)
{
	if ((scope->loopback_scope == 0) &&
	    (ifa->ifn_p) && SCTP_IFN_IS_IFT_LOOP(ifa->ifn_p)) {
		


		return (0);
	}
	switch (ifa->address.sa.sa_family) {
#ifdef INET
	case AF_INET:
		if (scope->ipv4_addr_legal) {
			struct sockaddr_in *sin;

			sin = (struct sockaddr_in *)&ifa->address.sin;
			if (sin->sin_addr.s_addr == 0) {
				
				return (0);
			}
			if ((scope->ipv4_local_scope == 0) &&
			    (IN4_ISPRIVATE_ADDRESS(&sin->sin_addr))) {
				
				return (0);
			}
		} else {
			return (0);
		}
		break;
#endif
#ifdef INET6
	case AF_INET6:
		if (scope->ipv6_addr_legal) {
			struct sockaddr_in6 *sin6;

#if !defined(__Panda__)
			


			if (do_update) {
				sctp_gather_internal_ifa_flags(ifa);
			}
#endif
			if (ifa->localifa_flags & SCTP_ADDR_IFA_UNUSEABLE) {
				return (0);
			}
			
			sin6 = (struct sockaddr_in6 *)&ifa->address.sin6;
			if (IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr)) {
				
				return (0);
			}
			if (		
			    (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr))) {
				return (0);
			}
			if ((scope->site_scope == 0) &&
			    (IN6_IS_ADDR_SITELOCAL(&sin6->sin6_addr))) {
				return (0);
			}
		} else {
			return (0);
		}
		break;
#endif
#if defined(__Userspace__)
	case AF_CONN:
		if (!scope->conn_addr_legal) {
			return (0);
		}
		break;
#endif
	default:
		return (0);
	}
	return (1);
}

static struct mbuf *
sctp_add_addr_to_mbuf(struct mbuf *m, struct sctp_ifa *ifa, uint16_t *len)
{
	struct sctp_paramhdr *parmh;
	struct mbuf *mret;
	uint16_t plen;

	switch (ifa->address.sa.sa_family) {
#ifdef INET
	case AF_INET:
		plen = (uint16_t)sizeof(struct sctp_ipv4addr_param);
		break;
#endif
#ifdef INET6
	case AF_INET6:
		plen = (uint16_t)sizeof(struct sctp_ipv6addr_param);
		break;
#endif
	default:
		return (m);
	}
	if (M_TRAILINGSPACE(m) >= plen) {
		
		parmh = (struct sctp_paramhdr *)(SCTP_BUF_AT(m, SCTP_BUF_LEN(m)));
		mret = m;
	} else {
		
		mret = m;
		while (SCTP_BUF_NEXT(mret) != NULL) {
			mret = SCTP_BUF_NEXT(mret);
		}
		SCTP_BUF_NEXT(mret) = sctp_get_mbuf_for_msg(plen, 0, M_NOWAIT, 1, MT_DATA);
		if (SCTP_BUF_NEXT(mret) == NULL) {
			
			return (m);
		}
		mret = SCTP_BUF_NEXT(mret);
		parmh = mtod(mret, struct sctp_paramhdr *);
	}
	
	switch (ifa->address.sa.sa_family) {
#ifdef INET
	case AF_INET:
	{
		struct sctp_ipv4addr_param *ipv4p;
		struct sockaddr_in *sin;

		sin = (struct sockaddr_in *)&ifa->address.sin;
		ipv4p = (struct sctp_ipv4addr_param *)parmh;
		parmh->param_type = htons(SCTP_IPV4_ADDRESS);
		parmh->param_length = htons(plen);
		ipv4p->addr = sin->sin_addr.s_addr;
		SCTP_BUF_LEN(mret) += plen;
		break;
	}
#endif
#ifdef INET6
	case AF_INET6:
	{
		struct sctp_ipv6addr_param *ipv6p;
		struct sockaddr_in6 *sin6;

		sin6 = (struct sockaddr_in6 *)&ifa->address.sin6;
		ipv6p = (struct sctp_ipv6addr_param *)parmh;
		parmh->param_type = htons(SCTP_IPV6_ADDRESS);
		parmh->param_length = htons(plen);
		memcpy(ipv6p->addr, &sin6->sin6_addr,
		    sizeof(ipv6p->addr));
#if defined(SCTP_EMBEDDED_V6_SCOPE)
		
		in6_clearscope((struct in6_addr *)ipv6p->addr);
#endif
		SCTP_BUF_LEN(mret) += plen;
		break;
	}
#endif
	default:
		return (m);
	}
	if (len != NULL) {
		*len += plen;
	}
	return (mret);
}


struct mbuf *
sctp_add_addresses_to_i_ia(struct sctp_inpcb *inp, struct sctp_tcb *stcb,
                           struct sctp_scoping *scope,
			   struct mbuf *m_at, int cnt_inits_to,
			   uint16_t *padding_len, uint16_t *chunk_len)
{
	struct sctp_vrf *vrf = NULL;
	int cnt, limit_out = 0, total_count;
	uint32_t vrf_id;

	vrf_id = inp->def_vrf_id;
	SCTP_IPI_ADDR_RLOCK();
	vrf = sctp_find_vrf(vrf_id);
	if (vrf == NULL) {
		SCTP_IPI_ADDR_RUNLOCK();
		return (m_at);
	}
	if (inp->sctp_flags & SCTP_PCB_FLAGS_BOUNDALL) {
		struct sctp_ifa *sctp_ifap;
		struct sctp_ifn *sctp_ifnp;

		cnt = cnt_inits_to;
		if (vrf->total_ifa_count > SCTP_COUNT_LIMIT) {
			limit_out = 1;
			cnt = SCTP_ADDRESS_LIMIT;
			goto skip_count;
		}
		LIST_FOREACH(sctp_ifnp, &vrf->ifnlist, next_ifn) {
			if ((scope->loopback_scope == 0) &&
			    SCTP_IFN_IS_IFT_LOOP(sctp_ifnp)) {
				



				continue;
			}
			LIST_FOREACH(sctp_ifap, &sctp_ifnp->ifalist, next_ifa) {
				if (sctp_is_addr_restricted(stcb, sctp_ifap)) {
					continue;
				}
#if defined(__Userspace__)
				if (sctp_ifap->address.sa.sa_family == AF_CONN) {
					continue;
				}
#endif
				if (sctp_is_address_in_scope(sctp_ifap, scope, 1) == 0) {
					continue;
				}
				cnt++;
				if (cnt > SCTP_ADDRESS_LIMIT) {
					break;
				}
			}
			if (cnt > SCTP_ADDRESS_LIMIT) {
				break;
			}
		}
	skip_count:
		if (cnt > 1) {
			total_count = 0;
			LIST_FOREACH(sctp_ifnp, &vrf->ifnlist, next_ifn) {
				cnt = 0;
				if ((scope->loopback_scope == 0) &&
				    SCTP_IFN_IS_IFT_LOOP(sctp_ifnp)) {
					



					continue;
				}
				LIST_FOREACH(sctp_ifap, &sctp_ifnp->ifalist, next_ifa) {
					if (sctp_is_addr_restricted(stcb, sctp_ifap)) {
						continue;
					}
#if defined(__Userspace__)
					if (sctp_ifap->address.sa.sa_family == AF_CONN) {
						continue;
					}
#endif
					if (sctp_is_address_in_scope(sctp_ifap,
								     scope, 0) == 0) {
						continue;
					}
					if ((chunk_len != NULL) &&
					    (padding_len != NULL) &&
					    (*padding_len > 0)) {
						memset(mtod(m_at, caddr_t) + *chunk_len, 0, *padding_len);
						SCTP_BUF_LEN(m_at) += *padding_len;
						*chunk_len += *padding_len;
						*padding_len = 0;
					}
					m_at = sctp_add_addr_to_mbuf(m_at, sctp_ifap, chunk_len);
					if (limit_out) {
						cnt++;
						total_count++;
						if (cnt >= 2) {
							
							break;
						}
						if (total_count > SCTP_ADDRESS_LIMIT) {
							
							break;
						}
					}
				}
			}
		}
	} else {
		struct sctp_laddr *laddr;

		cnt = cnt_inits_to;
		
		LIST_FOREACH(laddr, &inp->sctp_addr_list, sctp_nxt_addr) {
			if (laddr->ifa == NULL) {
				continue;
			}
			if (laddr->ifa->localifa_flags & SCTP_BEING_DELETED)
                                


				continue;
			if (laddr->action == SCTP_DEL_IP_ADDRESS) {
				


				continue;
			}
#if defined(__Userspace__)
			if (laddr->ifa->address.sa.sa_family == AF_CONN) {
				continue;
			}
#endif
			if (sctp_is_address_in_scope(laddr->ifa,
						     scope, 1) == 0) {
				continue;
			}
			cnt++;
		}
		




		if (cnt > 1) {
			cnt = cnt_inits_to;
			LIST_FOREACH(laddr, &inp->sctp_addr_list, sctp_nxt_addr) {
				if (laddr->ifa == NULL) {
					continue;
				}
				if (laddr->ifa->localifa_flags & SCTP_BEING_DELETED) {
					continue;
				}
#if defined(__Userspace__)
				if (laddr->ifa->address.sa.sa_family == AF_CONN) {
					continue;
				}
#endif
				if (sctp_is_address_in_scope(laddr->ifa,
							     scope, 0) == 0) {
					continue;
				}
				if ((chunk_len != NULL) &&
				    (padding_len != NULL) &&
				    (*padding_len > 0)) {
					memset(mtod(m_at, caddr_t) + *chunk_len, 0, *padding_len);
					SCTP_BUF_LEN(m_at) += *padding_len;
					*chunk_len += *padding_len;
					*padding_len = 0;
				}
				m_at = sctp_add_addr_to_mbuf(m_at, laddr->ifa, chunk_len);
				cnt++;
				if (cnt >= SCTP_ADDRESS_LIMIT) {
					break;
				}
			}
		}
	}
	SCTP_IPI_ADDR_RUNLOCK();
	return (m_at);
}

static struct sctp_ifa *
sctp_is_ifa_addr_preferred(struct sctp_ifa *ifa,
			   uint8_t dest_is_loop,
			   uint8_t dest_is_priv,
			   sa_family_t fam)
{
	uint8_t dest_is_global = 0;
	
	

	


























	if (ifa->address.sa.sa_family != fam) {
		
		return (NULL);
	}
	if ((dest_is_priv == 0) && (dest_is_loop == 0)) {
		dest_is_global = 1;
	}
	SCTPDBG(SCTP_DEBUG_OUTPUT2, "Is destination preferred:");
	SCTPDBG_ADDR(SCTP_DEBUG_OUTPUT2, &ifa->address.sa);
	
#ifdef INET6
	if (fam == AF_INET6) {
		
		if (ifa->localifa_flags & SCTP_ADDR_IFA_UNUSEABLE) {
			SCTPDBG(SCTP_DEBUG_OUTPUT3, "NO:1\n");
			return (NULL);
		}
		if (ifa->src_is_priv && !ifa->src_is_loop) {
			if (dest_is_loop) {
				SCTPDBG(SCTP_DEBUG_OUTPUT3, "NO:2\n");
				return (NULL);
			}
		}
		if (ifa->src_is_glob) {
			if (dest_is_loop) {
				SCTPDBG(SCTP_DEBUG_OUTPUT3, "NO:3\n");
				return (NULL);
			}
		}
	}
#endif
	



	SCTPDBG(SCTP_DEBUG_OUTPUT3, "src_loop:%d src_priv:%d src_glob:%d\n",
		ifa->src_is_loop, ifa->src_is_priv, ifa->src_is_glob);
	SCTPDBG(SCTP_DEBUG_OUTPUT3, "dest_loop:%d dest_priv:%d dest_glob:%d\n",
		dest_is_loop, dest_is_priv, dest_is_global);

	if ((ifa->src_is_loop) && (dest_is_priv)) {
		SCTPDBG(SCTP_DEBUG_OUTPUT3, "NO:4\n");
		return (NULL);
	}
	if ((ifa->src_is_glob) && (dest_is_priv)) {
		SCTPDBG(SCTP_DEBUG_OUTPUT3, "NO:5\n");
		return (NULL);
	}
	if ((ifa->src_is_loop) && (dest_is_global)) {
		SCTPDBG(SCTP_DEBUG_OUTPUT3, "NO:6\n");
		return (NULL);
	}
	if ((ifa->src_is_priv) && (dest_is_global)) {
		SCTPDBG(SCTP_DEBUG_OUTPUT3, "NO:7\n");
		return (NULL);
	}
	SCTPDBG(SCTP_DEBUG_OUTPUT3, "YES\n");
	
	return (ifa);
}

static struct sctp_ifa *
sctp_is_ifa_addr_acceptable(struct sctp_ifa *ifa,
			    uint8_t dest_is_loop,
			    uint8_t dest_is_priv,
			    sa_family_t fam)
{
	uint8_t dest_is_global = 0;

	





























	if (ifa->address.sa.sa_family != fam) {
		
		SCTPDBG(SCTP_DEBUG_OUTPUT3, "ifa_fam:%d fam:%d\n",
			ifa->address.sa.sa_family, fam);
		return (NULL);
	}
	
	SCTPDBG_ADDR(SCTP_DEBUG_OUTPUT3, &ifa->address.sa);
	SCTPDBG(SCTP_DEBUG_OUTPUT3, "dst_is_loop:%d dest_is_priv:%d\n",
		dest_is_loop, dest_is_priv);
	if ((dest_is_loop == 0) && (dest_is_priv == 0)) {
		dest_is_global = 1;
	}
#ifdef INET6
	if (fam == AF_INET6) {
		
		if (ifa->localifa_flags & SCTP_ADDR_IFA_UNUSEABLE) {
			return (NULL);
		}
		if (ifa->src_is_priv) {
			
			if (dest_is_loop)
				return (NULL);
		}
	}
#endif
	




	SCTPDBG(SCTP_DEBUG_OUTPUT3, "ifa->src_is_loop:%d dest_is_priv:%d\n",
		ifa->src_is_loop,
		dest_is_priv);
	if ((ifa->src_is_loop == 1) && (dest_is_priv)) {
		return (NULL);
	}
	SCTPDBG(SCTP_DEBUG_OUTPUT3, "ifa->src_is_loop:%d dest_is_glob:%d\n",
		ifa->src_is_loop,
		dest_is_global);
	if ((ifa->src_is_loop == 1) && (dest_is_global)) {
		return (NULL);
	}
	SCTPDBG(SCTP_DEBUG_OUTPUT3, "address is acceptable\n");
	
	return (ifa);
}

int
sctp_is_addr_restricted(struct sctp_tcb *stcb, struct sctp_ifa *ifa)
{
	struct sctp_laddr *laddr;

	if (stcb == NULL) {
		
		return (0);
	}
	LIST_FOREACH(laddr, &stcb->asoc.sctp_restricted_addrs, sctp_nxt_addr) {
		if (laddr->ifa == NULL) {
			SCTPDBG(SCTP_DEBUG_OUTPUT1, "%s: NULL ifa\n",
				__FUNCTION__);
			continue;
		}
		if (laddr->ifa == ifa) {
			
			return (1);
		}
	}
	return (0);
}


int
sctp_is_addr_in_ep(struct sctp_inpcb *inp, struct sctp_ifa *ifa)
{
	struct sctp_laddr *laddr;

	if (ifa == NULL)
		return (0);
	LIST_FOREACH(laddr, &inp->sctp_addr_list, sctp_nxt_addr) {
		if (laddr->ifa == NULL) {
			SCTPDBG(SCTP_DEBUG_OUTPUT1, "%s: NULL ifa\n",
				__FUNCTION__);
			continue;
		}
		if ((laddr->ifa == ifa) && laddr->action == 0)
			
			return (1);
	}
	return (0);
}



static struct sctp_ifa *
sctp_choose_boundspecific_inp(struct sctp_inpcb *inp,
			      sctp_route_t *ro,
			      uint32_t vrf_id,
			      int non_asoc_addr_ok,
			      uint8_t dest_is_priv,
			      uint8_t dest_is_loop,
			      sa_family_t fam)
{
	struct sctp_laddr *laddr, *starting_point;
	void *ifn;
	int resettotop = 0;
	struct sctp_ifn *sctp_ifn;
	struct sctp_ifa *sctp_ifa, *sifa;
	struct sctp_vrf *vrf;
	uint32_t ifn_index;

	vrf = sctp_find_vrf(vrf_id);
	if (vrf == NULL)
		return (NULL);

	ifn = SCTP_GET_IFN_VOID_FROM_ROUTE(ro);
	ifn_index = SCTP_GET_IF_INDEX_FROM_ROUTE(ro);
	sctp_ifn = sctp_find_ifn(ifn, ifn_index);
	




	if (sctp_ifn) {
		
		LIST_FOREACH(sctp_ifa, &sctp_ifn->ifalist, next_ifa) {
			if ((sctp_ifa->localifa_flags & SCTP_ADDR_DEFER_USE) &&
			    (non_asoc_addr_ok == 0))
				continue;
			sifa = sctp_is_ifa_addr_preferred(sctp_ifa,
							  dest_is_loop,
							  dest_is_priv, fam);
			if (sifa == NULL)
				continue;
			if (sctp_is_addr_in_ep(inp, sifa)) {
				atomic_add_int(&sifa->refcount, 1);
				return (sifa);
			}
		}
	}
	





	starting_point = inp->next_addr_touse;
 once_again:
	if (inp->next_addr_touse == NULL) {
		inp->next_addr_touse = LIST_FIRST(&inp->sctp_addr_list);
		resettotop = 1;
	}
	for (laddr = inp->next_addr_touse; laddr;
	     laddr = LIST_NEXT(laddr, sctp_nxt_addr)) {
		if (laddr->ifa == NULL) {
			
			continue;
		}
		if (laddr->action == SCTP_DEL_IP_ADDRESS) {
			
			continue;
		}
		sifa = sctp_is_ifa_addr_preferred(laddr->ifa, dest_is_loop,
						  dest_is_priv, fam);
		if (sifa == NULL)
			continue;
		atomic_add_int(&sifa->refcount, 1);
		return (sifa);
	}
	if (resettotop == 0) {
		inp->next_addr_touse = NULL;
		goto once_again;
	}

	inp->next_addr_touse = starting_point;
	resettotop = 0;
 once_again_too:
	if (inp->next_addr_touse == NULL) {
		inp->next_addr_touse = LIST_FIRST(&inp->sctp_addr_list);
		resettotop = 1;
	}

	
	for (laddr = inp->next_addr_touse; laddr;
	     laddr = LIST_NEXT(laddr, sctp_nxt_addr)) {
		if (laddr->ifa == NULL) {
			
			continue;
		}
		if (laddr->action == SCTP_DEL_IP_ADDRESS) {
			
			continue;
		}
		sifa = sctp_is_ifa_addr_acceptable(laddr->ifa, dest_is_loop,
						   dest_is_priv, fam);
		if (sifa == NULL)
			continue;
		atomic_add_int(&sifa->refcount, 1);
		return (sifa);
	}
	if (resettotop == 0) {
		inp->next_addr_touse = NULL;
		goto once_again_too;
	}

	



	return (NULL);
}



static struct sctp_ifa *
sctp_choose_boundspecific_stcb(struct sctp_inpcb *inp,
			       struct sctp_tcb *stcb,
			       sctp_route_t *ro,
			       uint32_t vrf_id,
			       uint8_t dest_is_priv,
			       uint8_t dest_is_loop,
			       int non_asoc_addr_ok,
			       sa_family_t fam)
{
	struct sctp_laddr *laddr, *starting_point;
	void *ifn;
	struct sctp_ifn *sctp_ifn;
	struct sctp_ifa *sctp_ifa, *sifa;
	uint8_t start_at_beginning = 0;
	struct sctp_vrf *vrf;
	uint32_t ifn_index;

	



	vrf = sctp_find_vrf(vrf_id);
	if (vrf == NULL)
		return (NULL);

	ifn = SCTP_GET_IFN_VOID_FROM_ROUTE(ro);
	ifn_index = SCTP_GET_IF_INDEX_FROM_ROUTE(ro);
	sctp_ifn = sctp_find_ifn( ifn, ifn_index);

	




	if (sctp_ifn) {
		
		LIST_FOREACH(sctp_ifa, &sctp_ifn->ifalist, next_ifa) {
			if ((sctp_ifa->localifa_flags & SCTP_ADDR_DEFER_USE) && (non_asoc_addr_ok == 0))
				continue;
			if (sctp_is_addr_in_ep(inp, sctp_ifa)) {
				sifa = sctp_is_ifa_addr_preferred(sctp_ifa, dest_is_loop, dest_is_priv, fam);
				if (sifa == NULL)
					continue;
				if (((non_asoc_addr_ok == 0) &&
				     (sctp_is_addr_restricted(stcb, sifa))) ||
				    (non_asoc_addr_ok &&
				     (sctp_is_addr_restricted(stcb, sifa)) &&
				     (!sctp_is_addr_pending(stcb, sifa)))) {
					
					continue;
				}
				atomic_add_int(&sifa->refcount, 1);
				return (sifa);
			}
		}
		
		LIST_FOREACH(sctp_ifa, &sctp_ifn->ifalist, next_ifa) {
			if ((sctp_ifa->localifa_flags & SCTP_ADDR_DEFER_USE) && (non_asoc_addr_ok == 0))
				continue;
			if (sctp_is_addr_in_ep(inp, sctp_ifa)) {
				sifa= sctp_is_ifa_addr_acceptable(sctp_ifa, dest_is_loop, dest_is_priv,fam);
				if (sifa == NULL)
					continue;
				if (((non_asoc_addr_ok == 0) &&
				     (sctp_is_addr_restricted(stcb, sifa))) ||
				    (non_asoc_addr_ok &&
				     (sctp_is_addr_restricted(stcb, sifa)) &&
				     (!sctp_is_addr_pending(stcb, sifa)))) {
					
					continue;
				}
				atomic_add_int(&sifa->refcount, 1);
				return (sifa);
			}
		}

	}
	




	starting_point = stcb->asoc.last_used_address;
 sctp_from_the_top:
	if (stcb->asoc.last_used_address == NULL) {
		start_at_beginning = 1;
		stcb->asoc.last_used_address = LIST_FIRST(&inp->sctp_addr_list);
	}
	
	for (laddr = stcb->asoc.last_used_address; laddr;
	     laddr = LIST_NEXT(laddr, sctp_nxt_addr)) {
		if (laddr->ifa == NULL) {
			
			continue;
		}
		if (laddr->action == SCTP_DEL_IP_ADDRESS) {
			
			continue;
		}
		sifa = sctp_is_ifa_addr_preferred(laddr->ifa, dest_is_loop, dest_is_priv, fam);
		if (sifa == NULL)
			continue;
		if (((non_asoc_addr_ok == 0) &&
		     (sctp_is_addr_restricted(stcb, sifa))) ||
		    (non_asoc_addr_ok &&
		     (sctp_is_addr_restricted(stcb, sifa)) &&
		     (!sctp_is_addr_pending(stcb, sifa)))) {
			
			continue;
		}
		stcb->asoc.last_used_address = laddr;
		atomic_add_int(&sifa->refcount, 1);
		return (sifa);
	}
	if (start_at_beginning == 0) {
		stcb->asoc.last_used_address = NULL;
		goto sctp_from_the_top;
	}
	
	stcb->asoc.last_used_address = starting_point;
	start_at_beginning = 0;
 sctp_from_the_top2:
	if (stcb->asoc.last_used_address == NULL) {
		start_at_beginning = 1;
		stcb->asoc.last_used_address = LIST_FIRST(&inp->sctp_addr_list);
	}
	
	for (laddr = stcb->asoc.last_used_address; laddr;
	     laddr = LIST_NEXT(laddr, sctp_nxt_addr)) {
		if (laddr->ifa == NULL) {
			
			continue;
		}
		if (laddr->action == SCTP_DEL_IP_ADDRESS) {
			
			continue;
		}
		sifa = sctp_is_ifa_addr_acceptable(laddr->ifa, dest_is_loop,
						   dest_is_priv, fam);
		if (sifa == NULL)
			continue;
		if (((non_asoc_addr_ok == 0) &&
		     (sctp_is_addr_restricted(stcb, sifa))) ||
		    (non_asoc_addr_ok &&
		     (sctp_is_addr_restricted(stcb, sifa)) &&
		     (!sctp_is_addr_pending(stcb, sifa)))) {
			
			continue;
		}
		stcb->asoc.last_used_address = laddr;
		atomic_add_int(&sifa->refcount, 1);
		return (sifa);
	}
	if (start_at_beginning == 0) {
		stcb->asoc.last_used_address = NULL;
		goto sctp_from_the_top2;
	}
	return (NULL);
}

static struct sctp_ifa *
sctp_select_nth_preferred_addr_from_ifn_boundall(struct sctp_ifn *ifn,
						 struct sctp_tcb *stcb,
						 int non_asoc_addr_ok,
						 uint8_t dest_is_loop,
						 uint8_t dest_is_priv,
						 int addr_wanted,
						 sa_family_t fam,
						 sctp_route_t *ro
						 )
{
	struct sctp_ifa *ifa, *sifa;
	int num_eligible_addr = 0;
#ifdef INET6
#ifdef SCTP_EMBEDDED_V6_SCOPE
	struct sockaddr_in6 sin6, lsa6;

	if (fam == AF_INET6) {
		memcpy(&sin6, &ro->ro_dst, sizeof(struct sockaddr_in6));
#ifdef SCTP_KAME
		(void)sa6_recoverscope(&sin6);
#else
		(void)in6_recoverscope(&sin6, &sin6.sin6_addr, NULL);
#endif  
	}
#endif  
#endif	
	LIST_FOREACH(ifa, &ifn->ifalist, next_ifa) {
		if ((ifa->localifa_flags & SCTP_ADDR_DEFER_USE) &&
		    (non_asoc_addr_ok == 0))
			continue;
		sifa = sctp_is_ifa_addr_preferred(ifa, dest_is_loop,
						  dest_is_priv, fam);
		if (sifa == NULL)
			continue;
#ifdef INET6
		if (fam == AF_INET6 &&
		    dest_is_loop &&
		    sifa->src_is_loop && sifa->src_is_priv) {
			


			continue;
		}
#ifdef SCTP_EMBEDDED_V6_SCOPE
		if (fam == AF_INET6 &&
		    IN6_IS_ADDR_LINKLOCAL(&sifa->address.sin6.sin6_addr) &&
		    IN6_IS_ADDR_LINKLOCAL(&sin6.sin6_addr)) {
			
			memcpy(&lsa6, &sifa->address.sin6, sizeof(struct sockaddr_in6));
#ifdef SCTP_KAME
			(void)sa6_recoverscope(&lsa6);
#else
			(void)in6_recoverscope(&lsa6, &lsa6.sin6_addr, NULL);
#endif  
			if (sin6.sin6_scope_id != lsa6.sin6_scope_id) {
				continue;
			}
		}
#endif  
#endif	

#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__Userspace__)
		





#ifdef INET6
		if (stcb && fam == AF_INET6 &&
		    sctp_is_mobility_feature_on(stcb->sctp_ep, SCTP_MOBILITY_BASE)) {
			if (sctp_v6src_match_nexthop(&sifa->address.sin6, ro)
			    == 0) {
				continue;
			}
		}
#endif
#ifdef INET
		
		if (stcb && fam == AF_INET &&
		    sctp_is_mobility_feature_on(stcb->sctp_ep, SCTP_MOBILITY_BASE)) {
			if (sctp_v4src_match_nexthop(sifa, ro) == 0) {
				continue;
			}
		}
#endif
#endif
		if (stcb) {
			if (sctp_is_address_in_scope(ifa, &stcb->asoc.scope, 0) == 0) {
				continue;
			}
			if (((non_asoc_addr_ok == 0) &&
			     (sctp_is_addr_restricted(stcb, sifa))) ||
			    (non_asoc_addr_ok &&
			     (sctp_is_addr_restricted(stcb, sifa)) &&
			     (!sctp_is_addr_pending(stcb, sifa)))) {
				



				continue;
			}
		}
		if (num_eligible_addr >= addr_wanted) {
			return (sifa);
		}
		num_eligible_addr++;
	}
	return (NULL);
}


static int
sctp_count_num_preferred_boundall(struct sctp_ifn *ifn,
				  struct sctp_tcb *stcb,
				  int non_asoc_addr_ok,
				  uint8_t dest_is_loop,
				  uint8_t dest_is_priv,
				  sa_family_t fam)
{
	struct sctp_ifa *ifa, *sifa;
	int num_eligible_addr = 0;

	LIST_FOREACH(ifa, &ifn->ifalist, next_ifa) {
		if ((ifa->localifa_flags & SCTP_ADDR_DEFER_USE) &&
		    (non_asoc_addr_ok == 0)) {
			continue;
		}
		sifa = sctp_is_ifa_addr_preferred(ifa, dest_is_loop,
						  dest_is_priv, fam);
		if (sifa == NULL) {
			continue;
		}
		if (stcb) {
			if (sctp_is_address_in_scope(ifa, &stcb->asoc.scope, 0) == 0) {
				continue;
			}
			if (((non_asoc_addr_ok == 0) &&
			     (sctp_is_addr_restricted(stcb, sifa))) ||
			    (non_asoc_addr_ok &&
			     (sctp_is_addr_restricted(stcb, sifa)) &&
			     (!sctp_is_addr_pending(stcb, sifa)))) {
				



				continue;
			}
		}
		num_eligible_addr++;
	}
	return (num_eligible_addr);
}

static struct sctp_ifa *
sctp_choose_boundall(struct sctp_tcb *stcb,
		     struct sctp_nets *net,
		     sctp_route_t *ro,
		     uint32_t vrf_id,
		     uint8_t dest_is_priv,
		     uint8_t dest_is_loop,
		     int non_asoc_addr_ok,
		     sa_family_t fam)
{
	int cur_addr_num = 0, num_preferred = 0;
	void *ifn;
	struct sctp_ifn *sctp_ifn, *looked_at = NULL, *emit_ifn;
	struct sctp_ifa *sctp_ifa, *sifa;
	uint32_t ifn_index;
	struct sctp_vrf *vrf;
#ifdef INET
	int retried = 0;
#endif

	













	vrf = sctp_find_vrf(vrf_id);
	if (vrf == NULL)
		return (NULL);

	ifn = SCTP_GET_IFN_VOID_FROM_ROUTE(ro);
	ifn_index = SCTP_GET_IF_INDEX_FROM_ROUTE(ro);
	SCTPDBG(SCTP_DEBUG_OUTPUT2,"ifn from route:%p ifn_index:%d\n", ifn, ifn_index);
	emit_ifn = looked_at = sctp_ifn = sctp_find_ifn(ifn, ifn_index);
	if (sctp_ifn == NULL) {
		
		SCTPDBG(SCTP_DEBUG_OUTPUT2,"No ifn emit interface?\n");
		goto bound_all_plan_b;
	}
	SCTPDBG(SCTP_DEBUG_OUTPUT2,"ifn_index:%d name:%s is emit interface\n",
		ifn_index, sctp_ifn->ifn_name);

	if (net) {
		cur_addr_num = net->indx_of_eligible_next_to_use;
	}
	num_preferred = sctp_count_num_preferred_boundall(sctp_ifn,
							  stcb,
							  non_asoc_addr_ok,
							  dest_is_loop,
							  dest_is_priv, fam);
	SCTPDBG(SCTP_DEBUG_OUTPUT2, "Found %d preferred source addresses for intf:%s\n",
		num_preferred, sctp_ifn->ifn_name);
	if (num_preferred == 0) {
		



		goto bound_all_plan_b;
	}
	




	if (cur_addr_num >= num_preferred) {
		cur_addr_num = 0;
	}
	



	SCTPDBG(SCTP_DEBUG_OUTPUT2, "cur_addr_num:%d\n", cur_addr_num);

	sctp_ifa = sctp_select_nth_preferred_addr_from_ifn_boundall(sctp_ifn, stcb, non_asoc_addr_ok, dest_is_loop,
                                                                    dest_is_priv, cur_addr_num, fam, ro);

	
	if (sctp_ifa) {
		atomic_add_int(&sctp_ifa->refcount, 1);
		if (net) {
			
			net->indx_of_eligible_next_to_use = cur_addr_num + 1;
		}
		return (sctp_ifa);
	}
	



 bound_all_plan_b:
	SCTPDBG(SCTP_DEBUG_OUTPUT2, "Trying Plan B\n");
	LIST_FOREACH(sctp_ifn, &vrf->ifnlist, next_ifn) {
		SCTPDBG(SCTP_DEBUG_OUTPUT2, "Examine interface %s\n",
			sctp_ifn->ifn_name);
		if (dest_is_loop == 0 && SCTP_IFN_IS_IFT_LOOP(sctp_ifn)) {
			
			SCTPDBG(SCTP_DEBUG_OUTPUT2, "skip\n");
			continue;
		}
		if ((sctp_ifn == looked_at) && looked_at) {
			
			SCTPDBG(SCTP_DEBUG_OUTPUT2, "already seen\n");
			continue;
		}
		num_preferred = sctp_count_num_preferred_boundall(sctp_ifn, stcb, non_asoc_addr_ok,
                                                                  dest_is_loop, dest_is_priv, fam);
		SCTPDBG(SCTP_DEBUG_OUTPUT2,
			"Found ifn:%p %d preferred source addresses\n",
			ifn, num_preferred);
		if (num_preferred == 0) {
			
			SCTPDBG(SCTP_DEBUG_OUTPUT2, "No prefered -- skipping to next\n");
			continue;
		}
		SCTPDBG(SCTP_DEBUG_OUTPUT2,
			"num preferred:%d on interface:%p cur_addr_num:%d\n",
			num_preferred, (void *)sctp_ifn, cur_addr_num);

		




		if (cur_addr_num >= num_preferred) {
			cur_addr_num = 0;
		}
		sifa = sctp_select_nth_preferred_addr_from_ifn_boundall(sctp_ifn, stcb, non_asoc_addr_ok, dest_is_loop,
                                                                        dest_is_priv, cur_addr_num, fam, ro);
		if (sifa == NULL)
			continue;
		if (net) {
			net->indx_of_eligible_next_to_use = cur_addr_num + 1;
			SCTPDBG(SCTP_DEBUG_OUTPUT2, "we selected %d\n",
				cur_addr_num);
			SCTPDBG(SCTP_DEBUG_OUTPUT2, "Source:");
			SCTPDBG_ADDR(SCTP_DEBUG_OUTPUT2, &sifa->address.sa);
			SCTPDBG(SCTP_DEBUG_OUTPUT2, "Dest:");
			SCTPDBG_ADDR(SCTP_DEBUG_OUTPUT2, &net->ro._l_addr.sa);
		}
		atomic_add_int(&sifa->refcount, 1);
		return (sifa);
	}
#ifdef INET
again_with_private_addresses_allowed:
#endif
	
	sifa = NULL;
	SCTPDBG(SCTP_DEBUG_OUTPUT2,"Trying Plan C: find acceptable on interface\n");
	if (emit_ifn == NULL) {
		SCTPDBG(SCTP_DEBUG_OUTPUT2,"Jump to Plan D - no emit_ifn\n");
		goto plan_d;
	}
	LIST_FOREACH(sctp_ifa, &emit_ifn->ifalist, next_ifa) {
		SCTPDBG(SCTP_DEBUG_OUTPUT2, "ifa:%p\n", (void *)sctp_ifa);
		if ((sctp_ifa->localifa_flags & SCTP_ADDR_DEFER_USE) &&
		    (non_asoc_addr_ok == 0)) {
			SCTPDBG(SCTP_DEBUG_OUTPUT2,"Defer\n");
			continue;
		}
		sifa = sctp_is_ifa_addr_acceptable(sctp_ifa, dest_is_loop,
						   dest_is_priv, fam);
		if (sifa == NULL) {
			SCTPDBG(SCTP_DEBUG_OUTPUT2, "IFA not acceptable\n");
			continue;
		}
		if (stcb) {
			if (sctp_is_address_in_scope(sifa, &stcb->asoc.scope, 0) == 0) {
				SCTPDBG(SCTP_DEBUG_OUTPUT2, "NOT in scope\n");
				sifa = NULL;
				continue;
			}
			if (((non_asoc_addr_ok == 0) &&
			     (sctp_is_addr_restricted(stcb, sifa))) ||
			    (non_asoc_addr_ok &&
			     (sctp_is_addr_restricted(stcb, sifa)) &&
			     (!sctp_is_addr_pending(stcb, sifa)))) {
				



				SCTPDBG(SCTP_DEBUG_OUTPUT2, "Its resticted\n");
				sifa = NULL;
				continue;
			}
		} else {
			SCTP_PRINTF("Stcb is null - no print\n");
		}
		atomic_add_int(&sifa->refcount, 1);
		goto out;
	}
 plan_d:
	





	SCTPDBG(SCTP_DEBUG_OUTPUT2, "Trying Plan D looked_at is %p\n", (void *)looked_at);
	LIST_FOREACH(sctp_ifn, &vrf->ifnlist, next_ifn) {
		if (dest_is_loop == 0 && SCTP_IFN_IS_IFT_LOOP(sctp_ifn)) {
			
			continue;
		}
		LIST_FOREACH(sctp_ifa, &sctp_ifn->ifalist, next_ifa) {
			if ((sctp_ifa->localifa_flags & SCTP_ADDR_DEFER_USE) &&
			    (non_asoc_addr_ok == 0))
				continue;
			sifa = sctp_is_ifa_addr_acceptable(sctp_ifa,
							   dest_is_loop,
							   dest_is_priv, fam);
			if (sifa == NULL)
				continue;
			if (stcb) {
				if (sctp_is_address_in_scope(sifa, &stcb->asoc.scope, 0) == 0) {
					sifa = NULL;
					continue;
				}
				if (((non_asoc_addr_ok == 0) &&
				     (sctp_is_addr_restricted(stcb, sifa))) ||
				    (non_asoc_addr_ok &&
				     (sctp_is_addr_restricted(stcb, sifa)) &&
				     (!sctp_is_addr_pending(stcb, sifa)))) {
					



					sifa = NULL;
					continue;
				}
			}
			goto out;
		}
	}
#ifdef INET
	if ((retried == 0) && (stcb->asoc.scope.ipv4_local_scope == 0)) {
		stcb->asoc.scope.ipv4_local_scope = 1;
		retried = 1;
		goto again_with_private_addresses_allowed;
	} else if (retried == 1) {
		stcb->asoc.scope.ipv4_local_scope = 0;
	}
#endif
out:
#ifdef INET
	if (sifa) {
		if (retried == 1) {
			LIST_FOREACH(sctp_ifn, &vrf->ifnlist, next_ifn) {
				if (dest_is_loop == 0 && SCTP_IFN_IS_IFT_LOOP(sctp_ifn)) {
					
					continue;
				}
				LIST_FOREACH(sctp_ifa, &sctp_ifn->ifalist, next_ifa) {
					struct sctp_ifa *tmp_sifa;

					if ((sctp_ifa->localifa_flags & SCTP_ADDR_DEFER_USE) &&
					    (non_asoc_addr_ok == 0))
						continue;
					tmp_sifa = sctp_is_ifa_addr_acceptable(sctp_ifa,
					                                       dest_is_loop,
					                                       dest_is_priv, fam);
					if (tmp_sifa == NULL) {
						continue;
					}
					if (tmp_sifa == sifa) {
						continue;
					}
					if (stcb) {
						if (sctp_is_address_in_scope(tmp_sifa,
						                             &stcb->asoc.scope, 0) == 0) {
							continue;
						}
						if (((non_asoc_addr_ok == 0) &&
						     (sctp_is_addr_restricted(stcb, tmp_sifa))) ||
						    (non_asoc_addr_ok &&
						     (sctp_is_addr_restricted(stcb, tmp_sifa)) &&
						     (!sctp_is_addr_pending(stcb, tmp_sifa)))) {
							



							continue;
						}
					}
					if ((tmp_sifa->address.sin.sin_family == AF_INET) &&
					    (IN4_ISPRIVATE_ADDRESS(&(tmp_sifa->address.sin.sin_addr)))) {
						sctp_add_local_addr_restricted(stcb, tmp_sifa);
					}
				}
			}
		}
		atomic_add_int(&sifa->refcount, 1);
	}
#endif
	return (sifa);
}




struct sctp_ifa *
sctp_source_address_selection(struct sctp_inpcb *inp,
			      struct sctp_tcb *stcb,
			      sctp_route_t *ro,
			      struct sctp_nets *net,
			      int non_asoc_addr_ok, uint32_t vrf_id)
{
	struct sctp_ifa *answer;
	uint8_t dest_is_priv, dest_is_loop;
	sa_family_t fam;
#ifdef INET
	struct sockaddr_in *to = (struct sockaddr_in *)&ro->ro_dst;
#endif
#ifdef INET6
	struct sockaddr_in6 *to6 = (struct sockaddr_in6 *)&ro->ro_dst;
#endif

	





























































	if (ro->ro_rt == NULL) {
		


		SCTP_RTALLOC(ro, vrf_id);
	}
	if (ro->ro_rt == NULL) {
		return (NULL);
	}
	fam = ro->ro_dst.sa_family;
	dest_is_priv = dest_is_loop = 0;
	
	switch (fam) {
#ifdef INET
	case AF_INET:
		
		if (IN4_ISLOOPBACK_ADDRESS(&to->sin_addr)) {
			dest_is_loop = 1;
			if (net != NULL) {
				
				net->addr_is_local = 1;
			}
		} else if ((IN4_ISPRIVATE_ADDRESS(&to->sin_addr))) {
			dest_is_priv = 1;
		}
		break;
#endif
#ifdef INET6
	case AF_INET6:
		
#if defined(__Userspace_os_Windows)
		if (IN6_IS_ADDR_LOOPBACK(&to6->sin6_addr)) {
#else
		if (IN6_IS_ADDR_LOOPBACK(&to6->sin6_addr) ||
		    SCTP_ROUTE_IS_REAL_LOOP(ro)) {
#endif






			dest_is_loop = 1;
			if (net != NULL) {
				
				net->addr_is_local = 1;
			}
		} else if (IN6_IS_ADDR_LINKLOCAL(&to6->sin6_addr)) {
			dest_is_priv = 1;
		}
		break;
#endif
	}
	SCTPDBG(SCTP_DEBUG_OUTPUT2, "Select source addr for:");
	SCTPDBG_ADDR(SCTP_DEBUG_OUTPUT2, (struct sockaddr *)&ro->ro_dst);
	SCTP_IPI_ADDR_RLOCK();
	if (inp->sctp_flags & SCTP_PCB_FLAGS_BOUNDALL) {
		


		answer = sctp_choose_boundall(stcb, net, ro, vrf_id,
					      dest_is_priv, dest_is_loop,
					      non_asoc_addr_ok, fam);
		SCTP_IPI_ADDR_RUNLOCK();
		return (answer);
	}
	


	if (stcb) {
		answer = sctp_choose_boundspecific_stcb(inp, stcb, ro,
							vrf_id,	dest_is_priv,
							dest_is_loop,
							non_asoc_addr_ok, fam);
	} else {
		answer = sctp_choose_boundspecific_inp(inp, ro, vrf_id,
						       non_asoc_addr_ok,
						       dest_is_priv,
						       dest_is_loop, fam);
	}
	SCTP_IPI_ADDR_RUNLOCK();
	return (answer);
}

static int
sctp_find_cmsg(int c_type, void *data, struct mbuf *control, size_t cpsize)
{
#if defined(__Userspace_os_Windows)
	WSACMSGHDR cmh;
#else
	struct cmsghdr cmh;
#endif
	int tlen, at, found;
	struct sctp_sndinfo sndinfo;
	struct sctp_prinfo prinfo;
	struct sctp_authinfo authinfo;

	tlen = SCTP_BUF_LEN(control);
	at = 0;
	found = 0;
	



	while (at < tlen) {
		if ((tlen - at) < (int)CMSG_ALIGN(sizeof(cmh))) {
			
			return (found);
		}
		m_copydata(control, at, sizeof(cmh), (caddr_t)&cmh);
		if (cmh.cmsg_len < CMSG_ALIGN(sizeof(cmh))) {
			
			return (found);
		}
		if (((int)cmh.cmsg_len + at) > tlen) {
			
			return (found);
		}
		if ((cmh.cmsg_level == IPPROTO_SCTP) &&
		    ((c_type == cmh.cmsg_type) ||
		     ((c_type == SCTP_SNDRCV) &&
		      ((cmh.cmsg_type == SCTP_SNDINFO) ||
		       (cmh.cmsg_type == SCTP_PRINFO) ||
		       (cmh.cmsg_type == SCTP_AUTHINFO))))) {
			if (c_type == cmh.cmsg_type) {
				if ((size_t)(cmh.cmsg_len - CMSG_ALIGN(sizeof(cmh))) < cpsize) {
					return (found);
				}
				
				m_copydata(control, at + CMSG_ALIGN(sizeof(cmh)), cpsize, (caddr_t)data);
				return (1);
			} else {
				struct sctp_sndrcvinfo *sndrcvinfo;

				sndrcvinfo = (struct sctp_sndrcvinfo *)data;
				if (found == 0) {
					if (cpsize < sizeof(struct sctp_sndrcvinfo)) {
						return (found);
					}
					memset(sndrcvinfo, 0, sizeof(struct sctp_sndrcvinfo));
				}
				switch (cmh.cmsg_type) {
				case SCTP_SNDINFO:
					if ((size_t)(cmh.cmsg_len - CMSG_ALIGN(sizeof(cmh))) < sizeof(struct sctp_sndinfo)) {
						return (found);
					}
					m_copydata(control, at + CMSG_ALIGN(sizeof(cmh)), sizeof(struct sctp_sndinfo), (caddr_t)&sndinfo);
					sndrcvinfo->sinfo_stream = sndinfo.snd_sid;
					sndrcvinfo->sinfo_flags = sndinfo.snd_flags;
					sndrcvinfo->sinfo_ppid = sndinfo.snd_ppid;
					sndrcvinfo->sinfo_context = sndinfo.snd_context;
					sndrcvinfo->sinfo_assoc_id = sndinfo.snd_assoc_id;
					break;
				case SCTP_PRINFO:
					if ((size_t)(cmh.cmsg_len - CMSG_ALIGN(sizeof(cmh))) < sizeof(struct sctp_prinfo)) {
						return (found);
					}
					m_copydata(control, at + CMSG_ALIGN(sizeof(cmh)), sizeof(struct sctp_prinfo), (caddr_t)&prinfo);
					sndrcvinfo->sinfo_timetolive = prinfo.pr_value;
					sndrcvinfo->sinfo_flags |= prinfo.pr_policy;
					break;
				case SCTP_AUTHINFO:
					if ((size_t)(cmh.cmsg_len - CMSG_ALIGN(sizeof(cmh))) < sizeof(struct sctp_authinfo)) {
						return (found);
					}
					m_copydata(control, at + CMSG_ALIGN(sizeof(cmh)), sizeof(struct sctp_authinfo), (caddr_t)&authinfo);
					sndrcvinfo->sinfo_keynumber_valid = 1;
					sndrcvinfo->sinfo_keynumber = authinfo.auth_keynumber;
					break;
				default:
					return (found);
				}
				found = 1;
			}
		}
		at += CMSG_ALIGN(cmh.cmsg_len);
	}
	return (found);
}

static int
sctp_process_cmsgs_for_init(struct sctp_tcb *stcb, struct mbuf *control, int *error)
{
#if defined(__Userspace_os_Windows)
	WSACMSGHDR cmh;
#else
	struct cmsghdr cmh;
#endif
	int tlen, at;
	struct sctp_initmsg initmsg;
#ifdef INET
	struct sockaddr_in sin;
#endif
#ifdef INET6
	struct sockaddr_in6 sin6;
#endif

	tlen = SCTP_BUF_LEN(control);
	at = 0;
	while (at < tlen) {
		if ((tlen - at) < (int)CMSG_ALIGN(sizeof(cmh))) {
			
			*error = EINVAL;
			return (1);
		}
		m_copydata(control, at, sizeof(cmh), (caddr_t)&cmh);
		if (cmh.cmsg_len < CMSG_ALIGN(sizeof(cmh))) {
			
			*error = EINVAL;
			return (1);
		}
		if (((int)cmh.cmsg_len + at) > tlen) {
			
			*error = EINVAL;
			return (1);
		}
		if (cmh.cmsg_level == IPPROTO_SCTP) {
			switch (cmh.cmsg_type) {
			case SCTP_INIT:
				if ((size_t)(cmh.cmsg_len - CMSG_ALIGN(sizeof(cmh))) < sizeof(struct sctp_initmsg)) {
					*error = EINVAL;
					return (1);
				}
				m_copydata(control, at + CMSG_ALIGN(sizeof(cmh)), sizeof(struct sctp_initmsg), (caddr_t)&initmsg);
				if (initmsg.sinit_max_attempts)
					stcb->asoc.max_init_times = initmsg.sinit_max_attempts;
				if (initmsg.sinit_num_ostreams)
					stcb->asoc.pre_open_streams = initmsg.sinit_num_ostreams;
				if (initmsg.sinit_max_instreams)
					stcb->asoc.max_inbound_streams = initmsg.sinit_max_instreams;
				if (initmsg.sinit_max_init_timeo)
					stcb->asoc.initial_init_rto_max = initmsg.sinit_max_init_timeo;
				if (stcb->asoc.streamoutcnt < stcb->asoc.pre_open_streams) {
					struct sctp_stream_out *tmp_str;
					unsigned int i;

					
					SCTPDBG(SCTP_DEBUG_OUTPUT1, "Ok, default:%d pre_open:%d\n",
						stcb->asoc.streamoutcnt, stcb->asoc.pre_open_streams);
					SCTP_TCB_UNLOCK(stcb);
					SCTP_MALLOC(tmp_str,
					            struct sctp_stream_out *,
					            (stcb->asoc.pre_open_streams * sizeof(struct sctp_stream_out)),
					            SCTP_M_STRMO);
					SCTP_TCB_LOCK(stcb);
					if (tmp_str != NULL) {
						SCTP_FREE(stcb->asoc.strmout, SCTP_M_STRMO);
						stcb->asoc.strmout = tmp_str;
						stcb->asoc.strm_realoutsize = stcb->asoc.streamoutcnt = stcb->asoc.pre_open_streams;
					} else {
						stcb->asoc.pre_open_streams = stcb->asoc.streamoutcnt;
					}
					for (i = 0; i < stcb->asoc.streamoutcnt; i++) {
						TAILQ_INIT(&stcb->asoc.strmout[i].outqueue);
						stcb->asoc.strmout[i].chunks_on_queues = 0;
						stcb->asoc.strmout[i].next_sequence_send = 0;
						stcb->asoc.strmout[i].stream_no = i;
						stcb->asoc.strmout[i].last_msg_incomplete = 0;
						stcb->asoc.ss_functions.sctp_ss_init_stream(&stcb->asoc.strmout[i], NULL);
					}
				}
				break;
#ifdef INET
			case SCTP_DSTADDRV4:
				if ((size_t)(cmh.cmsg_len - CMSG_ALIGN(sizeof(cmh))) < sizeof(struct in_addr)) {
					*error = EINVAL;
					return (1);
				}
				memset(&sin, 0, sizeof(struct sockaddr_in));
				sin.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
				sin.sin_len = sizeof(struct sockaddr_in);
#endif
				sin.sin_port = stcb->rport;
				m_copydata(control, at + CMSG_ALIGN(sizeof(cmh)), sizeof(struct in_addr), (caddr_t)&sin.sin_addr);
				if ((sin.sin_addr.s_addr == INADDR_ANY) ||
				    (sin.sin_addr.s_addr == INADDR_BROADCAST) ||
				    IN_MULTICAST(ntohl(sin.sin_addr.s_addr))) {
					*error = EINVAL;
					return (1);
				}
				if (sctp_add_remote_addr(stcb, (struct sockaddr *)&sin, NULL,
				                         SCTP_DONOT_SETSCOPE, SCTP_ADDR_IS_CONFIRMED)) {
					*error = ENOBUFS;
					return (1);
				}
				break;
#endif
#ifdef INET6
			case SCTP_DSTADDRV6:
				if ((size_t)(cmh.cmsg_len - CMSG_ALIGN(sizeof(cmh))) < sizeof(struct in6_addr)) {
					*error = EINVAL;
					return (1);
				}
				memset(&sin6, 0, sizeof(struct sockaddr_in6));
				sin6.sin6_family = AF_INET6;
#ifdef HAVE_SIN6_LEN
				sin6.sin6_len = sizeof(struct sockaddr_in6);
#endif
				sin6.sin6_port = stcb->rport;
				m_copydata(control, at + CMSG_ALIGN(sizeof(cmh)), sizeof(struct in6_addr), (caddr_t)&sin6.sin6_addr);
				if (IN6_IS_ADDR_UNSPECIFIED(&sin6.sin6_addr) ||
				    IN6_IS_ADDR_MULTICAST(&sin6.sin6_addr)) {
					*error = EINVAL;
					return (1);
				}
#ifdef INET
				if (IN6_IS_ADDR_V4MAPPED(&sin6.sin6_addr)) {
					in6_sin6_2_sin(&sin, &sin6);
					if ((sin.sin_addr.s_addr == INADDR_ANY) ||
					    (sin.sin_addr.s_addr == INADDR_BROADCAST) ||
					    IN_MULTICAST(ntohl(sin.sin_addr.s_addr))) {
						*error = EINVAL;
						return (1);
					}
					if (sctp_add_remote_addr(stcb, (struct sockaddr *)&sin, NULL,
					                         SCTP_DONOT_SETSCOPE, SCTP_ADDR_IS_CONFIRMED)) {
						*error = ENOBUFS;
						return (1);
					}
				} else
#endif
					if (sctp_add_remote_addr(stcb, (struct sockaddr *)&sin6, NULL,
					                         SCTP_DONOT_SETSCOPE, SCTP_ADDR_IS_CONFIRMED)) {
						*error = ENOBUFS;
						return (1);
					}
				break;
#endif
			default:
				break;
			}
		}
		at += CMSG_ALIGN(cmh.cmsg_len);
	}
	return (0);
}

static struct sctp_tcb *
sctp_findassociation_cmsgs(struct sctp_inpcb **inp_p,
                           in_port_t port,
                           struct mbuf *control,
                           struct sctp_nets **net_p,
                           int *error)
{
#if defined(__Userspace_os_Windows)
	WSACMSGHDR cmh;
#else
	struct cmsghdr cmh;
#endif
	int tlen, at;
	struct sctp_tcb *stcb;
	struct sockaddr *addr;
#ifdef INET
	struct sockaddr_in sin;
#endif
#ifdef INET6
	struct sockaddr_in6 sin6;
#endif

	tlen = SCTP_BUF_LEN(control);
	at = 0;
	while (at < tlen) {
		if ((tlen - at) < (int)CMSG_ALIGN(sizeof(cmh))) {
			
			*error = EINVAL;
			return (NULL);
		}
		m_copydata(control, at, sizeof(cmh), (caddr_t)&cmh);
		if (cmh.cmsg_len < CMSG_ALIGN(sizeof(cmh))) {
			
			*error = EINVAL;
			return (NULL);
		}
		if (((int)cmh.cmsg_len + at) > tlen) {
			
			*error = EINVAL;
			return (NULL);
		}
		if (cmh.cmsg_level == IPPROTO_SCTP) {
			switch (cmh.cmsg_type) {
#ifdef INET
			case SCTP_DSTADDRV4:
				if ((size_t)(cmh.cmsg_len - CMSG_ALIGN(sizeof(cmh))) < sizeof(struct in_addr)) {
					*error = EINVAL;
					return (NULL);
				}
				memset(&sin, 0, sizeof(struct sockaddr_in));
				sin.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
				sin.sin_len = sizeof(struct sockaddr_in);
#endif
				sin.sin_port = port;
				m_copydata(control, at + CMSG_ALIGN(sizeof(cmh)), sizeof(struct in_addr), (caddr_t)&sin.sin_addr);
				addr = (struct sockaddr *)&sin;
				break;
#endif
#ifdef INET6
			case SCTP_DSTADDRV6:
				if ((size_t)(cmh.cmsg_len - CMSG_ALIGN(sizeof(cmh))) < sizeof(struct in6_addr)) {
					*error = EINVAL;
					return (NULL);
				}
				memset(&sin6, 0, sizeof(struct sockaddr_in6));
				sin6.sin6_family = AF_INET6;
#ifdef HAVE_SIN6_LEN
				sin6.sin6_len = sizeof(struct sockaddr_in6);
#endif
				sin6.sin6_port = port;
				m_copydata(control, at + CMSG_ALIGN(sizeof(cmh)), sizeof(struct in6_addr), (caddr_t)&sin6.sin6_addr);
#ifdef INET
				if (IN6_IS_ADDR_V4MAPPED(&sin6.sin6_addr)) {
					in6_sin6_2_sin(&sin, &sin6);
					addr = (struct sockaddr *)&sin;
				} else
#endif
					addr = (struct sockaddr *)&sin6;
				break;
#endif
			default:
				addr = NULL;
				break;
			}
			if (addr) {
				stcb = sctp_findassociation_ep_addr(inp_p, addr, net_p, NULL, NULL);
				if (stcb != NULL) {
					return (stcb);
				}
			}
		}
		at += CMSG_ALIGN(cmh.cmsg_len);
	}
	return (NULL);
}

static struct mbuf *
sctp_add_cookie(struct mbuf *init, int init_offset,
    struct mbuf *initack, int initack_offset, struct sctp_state_cookie *stc_in, uint8_t **signature)
{
	struct mbuf *copy_init, *copy_initack, *m_at, *sig, *mret;
	struct sctp_state_cookie *stc;
	struct sctp_paramhdr *ph;
	uint8_t *foo;
	int sig_offset;
	uint16_t cookie_sz;

	mret = NULL;
	mret = sctp_get_mbuf_for_msg((sizeof(struct sctp_state_cookie) +
				      sizeof(struct sctp_paramhdr)), 0,
				     M_NOWAIT, 1, MT_DATA);
	if (mret == NULL) {
		return (NULL);
	}
	copy_init = SCTP_M_COPYM(init, init_offset, M_COPYALL, M_NOWAIT);
	if (copy_init == NULL) {
		sctp_m_freem(mret);
		return (NULL);
	}
#ifdef SCTP_MBUF_LOGGING
	if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_MBUF_LOGGING_ENABLE) {
		struct mbuf *mat;

		for (mat = copy_init; mat; mat = SCTP_BUF_NEXT(mat)) {
			if (SCTP_BUF_IS_EXTENDED(mat)) {
				sctp_log_mb(mat, SCTP_MBUF_ICOPY);
			}
		}
	}
#endif
	copy_initack = SCTP_M_COPYM(initack, initack_offset, M_COPYALL,
	    M_NOWAIT);
	if (copy_initack == NULL) {
		sctp_m_freem(mret);
		sctp_m_freem(copy_init);
		return (NULL);
	}
#ifdef SCTP_MBUF_LOGGING
	if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_MBUF_LOGGING_ENABLE) {
		struct mbuf *mat;

		for (mat = copy_initack; mat; mat = SCTP_BUF_NEXT(mat)) {
			if (SCTP_BUF_IS_EXTENDED(mat)) {
				sctp_log_mb(mat, SCTP_MBUF_ICOPY);
			}
		}
	}
#endif
	
	ph = mtod(mret, struct sctp_paramhdr *);
	SCTP_BUF_LEN(mret) = sizeof(struct sctp_state_cookie) +
	    sizeof(struct sctp_paramhdr);
	stc = (struct sctp_state_cookie *)((caddr_t)ph +
	    sizeof(struct sctp_paramhdr));
	ph->param_type = htons(SCTP_STATE_COOKIE);
	ph->param_length = 0;	
	
	memcpy(stc, stc_in, sizeof(struct sctp_state_cookie));

	
	cookie_sz = 0;
	for (m_at = mret; m_at; m_at = SCTP_BUF_NEXT(m_at)) {
		cookie_sz += SCTP_BUF_LEN(m_at);
		if (SCTP_BUF_NEXT(m_at) == NULL) {
			SCTP_BUF_NEXT(m_at) = copy_init;
			break;
		}
	}
	for (m_at = copy_init; m_at; m_at = SCTP_BUF_NEXT(m_at)) {
		cookie_sz += SCTP_BUF_LEN(m_at);
		if (SCTP_BUF_NEXT(m_at) == NULL) {
			SCTP_BUF_NEXT(m_at) = copy_initack;
			break;
		}
	}
	for (m_at = copy_initack; m_at; m_at = SCTP_BUF_NEXT(m_at)) {
		cookie_sz += SCTP_BUF_LEN(m_at);
		if (SCTP_BUF_NEXT(m_at) == NULL) {
			break;
		}
	}
	sig = sctp_get_mbuf_for_msg(SCTP_SECRET_SIZE, 0, M_NOWAIT, 1, MT_DATA);
	if (sig == NULL) {
		
		sctp_m_freem(mret);
		return (NULL);
	}
	SCTP_BUF_LEN(sig) = 0;
	SCTP_BUF_NEXT(m_at) = sig;
	sig_offset = 0;
	foo = (uint8_t *) (mtod(sig, caddr_t) + sig_offset);
	memset(foo, 0, SCTP_SIGNATURE_SIZE);
	*signature = foo;
	SCTP_BUF_LEN(sig) += SCTP_SIGNATURE_SIZE;
	cookie_sz += SCTP_SIGNATURE_SIZE;
	ph->param_length = htons(cookie_sz);
	return (mret);
}


static uint8_t
sctp_get_ect(struct sctp_tcb *stcb)
{
	if ((stcb != NULL) && (stcb->asoc.ecn_allowed == 1)) {
		return (SCTP_ECT0_BIT);
	} else {
		return (0);
	}
}

#if defined(INET) || defined(INET6)
static void
sctp_handle_no_route(struct sctp_tcb *stcb,
                     struct sctp_nets *net,
                     int so_locked)
{
	SCTPDBG(SCTP_DEBUG_OUTPUT1, "dropped packet - no valid source addr\n");

	if (net) {
		SCTPDBG(SCTP_DEBUG_OUTPUT1, "Destination was ");
		SCTPDBG_ADDR(SCTP_DEBUG_OUTPUT1, &net->ro._l_addr.sa);
		if (net->dest_state & SCTP_ADDR_CONFIRMED) {
			if ((net->dest_state & SCTP_ADDR_REACHABLE) && stcb) {
				SCTPDBG(SCTP_DEBUG_OUTPUT1, "no route takes interface %p down\n", (void *)net);
				sctp_ulp_notify(SCTP_NOTIFY_INTERFACE_DOWN,
			                        stcb, 0,
			                        (void *)net,
			                        so_locked);
				net->dest_state &= ~SCTP_ADDR_REACHABLE;
				net->dest_state &= ~SCTP_ADDR_PF;
			}
		}
		if (stcb) {
			if (net == stcb->asoc.primary_destination) {
				
				struct sctp_nets *alt;

				alt = sctp_find_alternate_net(stcb, net, 0);
				if (alt != net) {
					if (stcb->asoc.alternate) {
						sctp_free_remote_addr(stcb->asoc.alternate);
					}
					stcb->asoc.alternate = alt;
					atomic_add_int(&stcb->asoc.alternate->ref_count, 1);
					if (net->ro._s_addr) {
						sctp_free_ifa(net->ro._s_addr);
						net->ro._s_addr = NULL;
					}
					net->src_addr_selected = 0;
				}
			}
		}
	}
}
#endif

static int
sctp_lowlevel_chunk_output(struct sctp_inpcb *inp,
    struct sctp_tcb *stcb,	
    struct sctp_nets *net,
    struct sockaddr *to,
    struct mbuf *m,
    uint32_t auth_offset,
    struct sctp_auth_chunk *auth,
    uint16_t auth_keyid,
    int nofragment_flag,
    int ecn_ok,
    int out_of_asoc_ok,
    uint16_t src_port,
    uint16_t dest_port,
    uint32_t v_tag,
    uint16_t port,
    union sctp_sockstore *over_addr,
#if defined(__FreeBSD__)
    uint8_t use_mflowid, uint32_t mflowid,
#endif
#if !defined(__APPLE__) && !defined(SCTP_SO_LOCK_TESTING)
    int so_locked SCTP_UNUSED
#else
    int so_locked
#endif
    )

{
	











	
#ifdef __Panda__
	pakhandle_type o_pak;
#endif
	struct mbuf *newm;
	struct sctphdr *sctphdr;
	int packet_length;
	int ret;
	uint32_t vrf_id;
#if defined(INET) || defined(INET6)
#if !defined(__Panda__)
	struct mbuf *o_pak;
#endif
	sctp_route_t *ro = NULL;
	struct udphdr *udp = NULL;
#endif
	uint8_t tos_value;
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
	struct socket *so = NULL;
#endif

#if defined(__APPLE__)
	if (so_locked) {
		sctp_lock_assert(SCTP_INP_SO(inp));
		SCTP_TCB_LOCK_ASSERT(stcb);
	} else {
		sctp_unlock_assert(SCTP_INP_SO(inp));
	}
#endif
	if ((net) && (net->dest_state & SCTP_ADDR_OUT_OF_SCOPE)) {
		SCTP_LTRACE_ERR_RET_PKT(m, inp, stcb, net, SCTP_FROM_SCTP_OUTPUT, EFAULT);
		sctp_m_freem(m);
		return (EFAULT);
	}
	if (stcb) {
		vrf_id = stcb->asoc.vrf_id;
	} else {
		vrf_id = inp->def_vrf_id;
	}

	
	if ((auth != NULL) && (stcb != NULL)) {
		sctp_fill_hmac_digest_m(m, auth_offset, auth, stcb, auth_keyid);
	}

	if (net) {
		tos_value = net->dscp;
	} else if (stcb) {
		tos_value = stcb->asoc.default_dscp;
	} else {
		tos_value = inp->sctp_ep.default_dscp;
	}

	switch (to->sa_family) {
#ifdef INET
	case AF_INET:
	{
		struct ip *ip = NULL;
		sctp_route_t iproute;
		int len;

		len = sizeof(struct ip) + sizeof(struct sctphdr);
		if (port) {
			len += sizeof(struct udphdr);
		}
		newm = sctp_get_mbuf_for_msg(len, 1, M_NOWAIT, 1, MT_DATA);
		if (newm == NULL) {
			sctp_m_freem(m);
			SCTP_LTRACE_ERR_RET(inp, stcb, NULL, SCTP_FROM_SCTP_OUTPUT, ENOMEM);
			return (ENOMEM);
		}
		SCTP_ALIGN_TO_END(newm, len);
		SCTP_BUF_LEN(newm) = len;
		SCTP_BUF_NEXT(newm) = m;
		m = newm;
#if defined(__FreeBSD__)
		if (net != NULL) {
#ifdef INVARIANTS
			if (net->flowidset == 0) {
				panic("Flow ID not set");
			}
#endif
			m->m_pkthdr.flowid = net->flowid;
			m->m_flags |= M_FLOWID;
		} else {
			if (use_mflowid != 0) {
				m->m_pkthdr.flowid = mflowid;
				m->m_flags |= M_FLOWID;
			}
		}
#endif
		packet_length = sctp_calculate_len(m);
		ip = mtod(m, struct ip *);
		ip->ip_v = IPVERSION;
		ip->ip_hl = (sizeof(struct ip) >> 2);
		if (tos_value == 0) {
			



#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__Panda__) || defined(__Windows__) || defined(__Userspace__)
			tos_value = inp->ip_inp.inp.inp_ip_tos;
#else
			tos_value = inp->inp_ip_tos;
#endif
		}
		tos_value &= 0xfc;
		if (ecn_ok) {
			tos_value |= sctp_get_ect(stcb);
		}
                if ((nofragment_flag) && (port == 0)) {
#if defined(__FreeBSD__)
#if __FreeBSD_version >= 1000000
			ip->ip_off = htons(IP_DF);
#else
			ip->ip_off = IP_DF;
#endif
#elif defined(WITH_CONVERT_IP_OFF) || defined(__APPLE__) || defined(__Userspace__)
			ip->ip_off = IP_DF;
#else
			ip->ip_off = htons(IP_DF);
#endif
		} else {
#if defined(__FreeBSD__) && __FreeBSD_version >= 1000000
			ip->ip_off = htons(0);
#else
			ip->ip_off = 0;
#endif
		}
#if defined(__FreeBSD__)
		
		ip->ip_id = ip_newid();
#elif defined(RANDOM_IP_ID)
		
		ip->ip_id = htons(ip_randomid());
#elif defined(__Userspace__)
                ip->ip_id = htons(SCTP_IP_ID(inp)++);
#else
		ip->ip_id = SCTP_IP_ID(inp)++;
#endif

#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__Panda__) || defined(__Windows__) || defined(__Userspace__)
		ip->ip_ttl = inp->ip_inp.inp.inp_ip_ttl;
#else
		ip->ip_ttl = inp->inp_ip_ttl;
#endif
#if defined(__FreeBSD__) && __FreeBSD_version >= 1000000
		ip->ip_len = htons(packet_length);
#else
		ip->ip_len = packet_length;
#endif
		ip->ip_tos = tos_value;
		if (port) {
			ip->ip_p = IPPROTO_UDP;
		} else {
			ip->ip_p = IPPROTO_SCTP;
		}
		ip->ip_sum = 0;
		if (net == NULL) {
			ro = &iproute;
			memset(&iproute, 0, sizeof(iproute));
#ifdef HAVE_SA_LEN
			memcpy(&ro->ro_dst, to, to->sa_len);
#else
			memcpy(&ro->ro_dst, to, sizeof(struct sockaddr_in));
#endif
		} else {
			ro = (sctp_route_t *)&net->ro;
		}
		
		ip->ip_dst.s_addr = ((struct sockaddr_in *)to)->sin_addr.s_addr;

		
		if (net && out_of_asoc_ok == 0) {
			if (net->ro._s_addr && (net->ro._s_addr->localifa_flags & (SCTP_BEING_DELETED|SCTP_ADDR_IFA_UNUSEABLE))) {
				sctp_free_ifa(net->ro._s_addr);
				net->ro._s_addr = NULL;
				net->src_addr_selected = 0;
				if (ro->ro_rt) {
					RTFREE(ro->ro_rt);
					ro->ro_rt = NULL;
				}
			}
			if (net->src_addr_selected == 0) {
				
				net->ro._s_addr = sctp_source_address_selection(inp,stcb,
										ro, net, 0,
										vrf_id);
				net->src_addr_selected = 1;
			}
			if (net->ro._s_addr == NULL) {
				
				net->src_addr_selected = 0;
				sctp_handle_no_route(stcb, net, so_locked);
				SCTP_LTRACE_ERR_RET_PKT(m, inp, stcb, NULL, SCTP_FROM_SCTP_OUTPUT, EHOSTUNREACH);
				sctp_m_freem(m);
				return (EHOSTUNREACH);
			}
			ip->ip_src = net->ro._s_addr->address.sin.sin_addr;
		} else {
			if (over_addr == NULL) {
				struct sctp_ifa *_lsrc;

				_lsrc = sctp_source_address_selection(inp, stcb, ro,
				                                      net,
				                                      out_of_asoc_ok,
				                                      vrf_id);
				if (_lsrc == NULL) {
					sctp_handle_no_route(stcb, net, so_locked);
					SCTP_LTRACE_ERR_RET_PKT(m, inp, stcb, NULL, SCTP_FROM_SCTP_OUTPUT, EHOSTUNREACH);
					sctp_m_freem(m);
					return (EHOSTUNREACH);
				}
				ip->ip_src = _lsrc->address.sin.sin_addr;
				sctp_free_ifa(_lsrc);
			} else {
				ip->ip_src = over_addr->sin.sin_addr;
				SCTP_RTALLOC(ro, vrf_id);
			}
		}
		if (port) {
			if (htons(SCTP_BASE_SYSCTL(sctp_udp_tunneling_port)) == 0) {
				sctp_handle_no_route(stcb, net, so_locked);
				SCTP_LTRACE_ERR_RET_PKT(m, inp, stcb, NULL, SCTP_FROM_SCTP_OUTPUT, EHOSTUNREACH);
				sctp_m_freem(m);
				return (EHOSTUNREACH);
			}
			udp = (struct udphdr *)((caddr_t)ip + sizeof(struct ip));
			udp->uh_sport = htons(SCTP_BASE_SYSCTL(sctp_udp_tunneling_port));
			udp->uh_dport = port;
			udp->uh_ulen = htons(packet_length - sizeof(struct ip));
#if !defined(__Windows__) && !defined(__Userspace__)
#if defined(__FreeBSD__) && ((__FreeBSD_version > 803000 && __FreeBSD_version < 900000) || __FreeBSD_version > 900000)
			if (V_udp_cksum) {
				udp->uh_sum = in_pseudo(ip->ip_src.s_addr, ip->ip_dst.s_addr, udp->uh_ulen + htons(IPPROTO_UDP));
			} else {
				udp->uh_sum = 0;
			}
#else
			udp->uh_sum = in_pseudo(ip->ip_src.s_addr, ip->ip_dst.s_addr, udp->uh_ulen + htons(IPPROTO_UDP));
#endif
#else
			udp->uh_sum = 0;
#endif
			sctphdr = (struct sctphdr *)((caddr_t)udp + sizeof(struct udphdr));
		} else {
			sctphdr = (struct sctphdr *)((caddr_t)ip + sizeof(struct ip));
		}

		sctphdr->src_port = src_port;
		sctphdr->dest_port = dest_port;
		sctphdr->v_tag = v_tag;
		sctphdr->checksum = 0;

		






		if (ro->ro_rt == NULL) {
			




			sctp_handle_no_route(stcb, net, so_locked);
			SCTP_LTRACE_ERR_RET_PKT(m, inp, stcb, NULL, SCTP_FROM_SCTP_OUTPUT, EHOSTUNREACH);
			sctp_m_freem(m);
			return (EHOSTUNREACH);
		}
		if (ro != &iproute) {
			memcpy(&iproute, ro, sizeof(*ro));
		}
		SCTPDBG(SCTP_DEBUG_OUTPUT3, "Calling ipv4 output routine from low level src addr:%x\n",
			(uint32_t) (ntohl(ip->ip_src.s_addr)));
		SCTPDBG(SCTP_DEBUG_OUTPUT3, "Destination is %x\n",
			(uint32_t)(ntohl(ip->ip_dst.s_addr)));
		SCTPDBG(SCTP_DEBUG_OUTPUT3, "RTP route is %p through\n",
			(void *)ro->ro_rt);

		if (SCTP_GET_HEADER_FOR_OUTPUT(o_pak)) {
			
			SCTP_LTRACE_ERR_RET_PKT(m, inp, stcb, NULL, SCTP_FROM_SCTP_OUTPUT, ENOMEM);
			sctp_m_freem(m);
			return (ENOMEM);
		}
		SCTP_ATTACH_CHAIN(o_pak, m, packet_length);
		if (port) {
#if defined(SCTP_WITH_NO_CSUM)
			SCTP_STAT_INCR(sctps_sendnocrc);
#else
			sctphdr->checksum = sctp_calculate_cksum(m, sizeof(struct ip) + sizeof(struct udphdr));
			SCTP_STAT_INCR(sctps_sendswcrc);
#endif
#if defined(__FreeBSD__) && ((__FreeBSD_version > 803000 && __FreeBSD_version < 900000) || __FreeBSD_version > 900000)
			if (V_udp_cksum) {
				SCTP_ENABLE_UDP_CSUM(o_pak);
			}
#else
			SCTP_ENABLE_UDP_CSUM(o_pak);
#endif
		} else {
#if defined(SCTP_WITH_NO_CSUM)
			SCTP_STAT_INCR(sctps_sendnocrc);
#else
#if defined(__FreeBSD__) && __FreeBSD_version >= 800000
			m->m_pkthdr.csum_flags = CSUM_SCTP;
			m->m_pkthdr.csum_data = 0;
			SCTP_STAT_INCR(sctps_sendhwcrc);
#else
			if (!(SCTP_BASE_SYSCTL(sctp_no_csum_on_loopback) &&
			      (stcb) && (stcb->asoc.scope.loopback_scope))) {
				sctphdr->checksum = sctp_calculate_cksum(m, sizeof(struct ip));
				SCTP_STAT_INCR(sctps_sendswcrc);
			} else {
				SCTP_STAT_INCR(sctps_sendnocrc);
			}
#endif
#endif
		}
#ifdef SCTP_PACKET_LOGGING
		if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_LAST_PACKET_TRACING)
			sctp_packet_log(o_pak);
#endif
		
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
		if ((SCTP_BASE_SYSCTL(sctp_output_unlocked)) && (so_locked)) {
			so = SCTP_INP_SO(inp);
			SCTP_SOCKET_UNLOCK(so, 0);
		}
#endif
		SCTP_IP_OUTPUT(ret, o_pak, ro, stcb, vrf_id);
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
		if ((SCTP_BASE_SYSCTL(sctp_output_unlocked)) && (so_locked)) {
			atomic_add_int(&stcb->asoc.refcnt, 1);
			SCTP_TCB_UNLOCK(stcb);
			SCTP_SOCKET_LOCK(so, 0);
			SCTP_TCB_LOCK(stcb);
			atomic_subtract_int(&stcb->asoc.refcnt, 1);
		}
#endif
		SCTP_STAT_INCR(sctps_sendpackets);
		SCTP_STAT_INCR_COUNTER64(sctps_outpackets);
		if (ret)
			SCTP_STAT_INCR(sctps_senderrors);

		SCTPDBG(SCTP_DEBUG_OUTPUT3, "IP output returns %d\n", ret);
		if (net == NULL) {
			
#if defined(__FreeBSD__) && __FreeBSD_version > 901000
			RO_RTFREE(ro);
#else
			if (ro->ro_rt) {
				RTFREE(ro->ro_rt);
				ro->ro_rt = NULL;
			}
#endif
		} else {
			
			if ((ro->ro_rt != NULL) &&
			    (net->ro._s_addr)) {
				uint32_t mtu;
				mtu = SCTP_GATHER_MTU_FROM_ROUTE(net->ro._s_addr, &net->ro._l_addr.sa, ro->ro_rt);
				if (net->port) {
					mtu -= sizeof(struct udphdr);
				}
				if (mtu && (stcb->asoc.smallest_mtu > mtu)) {
					sctp_mtu_size_reset(inp, &stcb->asoc, mtu);
					net->mtu = mtu;
				}
			} else if (ro->ro_rt == NULL) {
				
				if (net->ro._s_addr &&
				    net->src_addr_selected) {
					sctp_free_ifa(net->ro._s_addr);
					net->ro._s_addr = NULL;
				}
				net->src_addr_selected = 0;
			}
		}
		return (ret);
	}
#endif
#ifdef INET6
	case AF_INET6:
	{
		uint32_t flowlabel, flowinfo;
		struct ip6_hdr *ip6h;
		struct route_in6 ip6route;
#if !(defined(__Panda__) || defined(__Userspace__))
		struct ifnet *ifp;
#endif
		struct sockaddr_in6 *sin6, tmp, *lsa6, lsa6_tmp;
		int prev_scope = 0;
#ifdef SCTP_EMBEDDED_V6_SCOPE
		struct sockaddr_in6 lsa6_storage;
		int error;
#endif
		u_short prev_port = 0;
		int len;

		if (net) {
			flowlabel = net->flowlabel;
		} else if (stcb) {
			flowlabel = stcb->asoc.default_flowlabel;
		} else {
			flowlabel = inp->sctp_ep.default_flowlabel;
		}
		if (flowlabel == 0) {
			



			flowlabel = ntohl(((struct in6pcb *)inp)->in6p_flowinfo);
		}
		flowlabel &= 0x000fffff;
		len = sizeof(struct ip6_hdr) + sizeof(struct sctphdr);
		if (port) {
			len += sizeof(struct udphdr);
		}
		newm = sctp_get_mbuf_for_msg(len, 1, M_NOWAIT, 1, MT_DATA);
		if (newm == NULL) {
			sctp_m_freem(m);
			SCTP_LTRACE_ERR_RET(inp, stcb, NULL, SCTP_FROM_SCTP_OUTPUT, ENOMEM);
			return (ENOMEM);
		}
		SCTP_ALIGN_TO_END(newm, len);
		SCTP_BUF_LEN(newm) = len;
		SCTP_BUF_NEXT(newm) = m;
		m = newm;
#if defined(__FreeBSD__)
		if (net != NULL) {
#ifdef INVARIANTS
			if (net->flowidset == 0) {
				panic("Flow ID not set");
			}
#endif
			m->m_pkthdr.flowid = net->flowid;
			m->m_flags |= M_FLOWID;
		} else {
			if (use_mflowid != 0) {
				m->m_pkthdr.flowid = mflowid;
				m->m_flags |= M_FLOWID;
			}
		}
#endif
		packet_length = sctp_calculate_len(m);

		ip6h = mtod(m, struct ip6_hdr *);
		
		sin6 = (struct sockaddr_in6 *)to;
		tmp = *sin6;
		sin6 = &tmp;

#ifdef SCTP_EMBEDDED_V6_SCOPE
		
#if defined(__APPLE__)
#if defined(APPLE_LEOPARD) || defined(APPLE_SNOWLEOPARD)
		if (in6_embedscope(&sin6->sin6_addr, sin6, NULL, NULL) != 0)
#else
		if (in6_embedscope(&sin6->sin6_addr, sin6, NULL, NULL, NULL) != 0)
#endif
#elif defined(SCTP_KAME)
		if (sa6_embedscope(sin6, MODULE_GLOBAL(ip6_use_defzone)) != 0)
#else
		if (in6_embedscope(&sin6->sin6_addr, sin6) != 0)
#endif
		{
			SCTP_LTRACE_ERR_RET_PKT(m, inp, stcb, net, SCTP_FROM_SCTP_OUTPUT, EINVAL);
			return (EINVAL);
		}
#endif 
		if (net == NULL) {
			memset(&ip6route, 0, sizeof(ip6route));
			ro = (sctp_route_t *)&ip6route;
#ifdef HAVE_SIN6_LEN
			memcpy(&ro->ro_dst, sin6, sin6->sin6_len);
#else
			memcpy(&ro->ro_dst, sin6, sizeof(struct sockaddr_in6));
#endif
		} else {
			ro = (sctp_route_t *)&net->ro;
		}
		



		if (tos_value == 0) {
			



#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__Panda__) || defined(__Windows__) || defined(__Userspace__)
			tos_value = (ntohl(((struct in6pcb *)inp)->in6p_flowinfo) >> 20) & 0xff;
#endif
		}
		tos_value &= 0xfc;
		if (ecn_ok) {
			tos_value |= sctp_get_ect(stcb);
		}
		flowinfo = 0x06;
		flowinfo <<= 8;
		flowinfo |= tos_value;
		flowinfo <<= 20;
		flowinfo |= flowlabel;
		ip6h->ip6_flow = htonl(flowinfo);
		if (port) {
			ip6h->ip6_nxt = IPPROTO_UDP;
		} else {
			ip6h->ip6_nxt = IPPROTO_SCTP;
		}
		ip6h->ip6_plen = (packet_length - sizeof(struct ip6_hdr));
		ip6h->ip6_dst = sin6->sin6_addr;

		




		bzero(&lsa6_tmp, sizeof(lsa6_tmp));
		lsa6_tmp.sin6_family = AF_INET6;
#ifdef HAVE_SIN6_LEN
		lsa6_tmp.sin6_len = sizeof(lsa6_tmp);
#endif
		lsa6 = &lsa6_tmp;
		if (net && out_of_asoc_ok == 0) {
			if (net->ro._s_addr && (net->ro._s_addr->localifa_flags & (SCTP_BEING_DELETED|SCTP_ADDR_IFA_UNUSEABLE))) {
				sctp_free_ifa(net->ro._s_addr);
				net->ro._s_addr = NULL;
				net->src_addr_selected = 0;
				if (ro->ro_rt) {
					RTFREE(ro->ro_rt);
					ro->ro_rt = NULL;
				}
			}
			if (net->src_addr_selected == 0) {
#ifdef SCTP_EMBEDDED_V6_SCOPE
				sin6 = (struct sockaddr_in6 *)&net->ro._l_addr;
				
#if defined(__APPLE__)
#if defined(APPLE_LEOPARD) || defined(APPLE_SNOWLEOPARD)
				if (in6_embedscope(&sin6->sin6_addr, sin6, NULL, NULL) != 0)
#else
				if (in6_embedscope(&sin6->sin6_addr, sin6, NULL, NULL, NULL) != 0)
#endif
#elif defined(SCTP_KAME)
				if (sa6_embedscope(sin6, MODULE_GLOBAL(ip6_use_defzone)) != 0)
#else
				if (in6_embedscope(&sin6->sin6_addr, sin6) != 0)
#endif
				{
					SCTP_LTRACE_ERR_RET_PKT(m, inp, stcb, net, SCTP_FROM_SCTP_OUTPUT, EINVAL);
					return (EINVAL);
				}
#endif 
				
				net->ro._s_addr = sctp_source_address_selection(inp,
										stcb,
										ro,
										net,
										0,
										vrf_id);
#ifdef SCTP_EMBEDDED_V6_SCOPE
#ifdef SCTP_KAME
				(void)sa6_recoverscope(sin6);
#else
				(void)in6_recoverscope(sin6, &sin6->sin6_addr, NULL);
#endif	
#endif	
				net->src_addr_selected = 1;
			}
			if (net->ro._s_addr == NULL) {
				SCTPDBG(SCTP_DEBUG_OUTPUT3, "V6:No route to host\n");
				net->src_addr_selected = 0;
				sctp_handle_no_route(stcb, net, so_locked);
				SCTP_LTRACE_ERR_RET_PKT(m, inp, stcb, NULL, SCTP_FROM_SCTP_OUTPUT, EHOSTUNREACH);
				sctp_m_freem(m);
				return (EHOSTUNREACH);
			}
			lsa6->sin6_addr = net->ro._s_addr->address.sin6.sin6_addr;
		} else {
#ifdef SCTP_EMBEDDED_V6_SCOPE
			sin6 = (struct sockaddr_in6 *)&ro->ro_dst;
			
#if defined(__APPLE__)
#if defined(APPLE_LEOPARD) || defined(APPLE_SNOWLEOPARD)
			if (in6_embedscope(&sin6->sin6_addr, sin6, NULL, NULL) != 0)
#else
			if (in6_embedscope(&sin6->sin6_addr, sin6, NULL, NULL, NULL) != 0)
#endif
#elif defined(SCTP_KAME)
			if (sa6_embedscope(sin6, MODULE_GLOBAL(ip6_use_defzone)) != 0)
#else
			if (in6_embedscope(&sin6->sin6_addr, sin6) != 0)
#endif
			  {
				SCTP_LTRACE_ERR_RET_PKT(m, inp, stcb, net, SCTP_FROM_SCTP_OUTPUT, EINVAL);
				return (EINVAL);
			  }
#endif 
			if (over_addr == NULL) {
				struct sctp_ifa *_lsrc;

				_lsrc = sctp_source_address_selection(inp, stcb, ro,
				                                      net,
				                                      out_of_asoc_ok,
				                                      vrf_id);
				if (_lsrc == NULL) {
					sctp_handle_no_route(stcb, net, so_locked);
					SCTP_LTRACE_ERR_RET_PKT(m, inp, stcb, NULL, SCTP_FROM_SCTP_OUTPUT, EHOSTUNREACH);
					sctp_m_freem(m);
					return (EHOSTUNREACH);
				}
				lsa6->sin6_addr = _lsrc->address.sin6.sin6_addr;
				sctp_free_ifa(_lsrc);
			} else {
				lsa6->sin6_addr = over_addr->sin6.sin6_addr;
				SCTP_RTALLOC(ro, vrf_id);
			}
#ifdef SCTP_EMBEDDED_V6_SCOPE
#ifdef SCTP_KAME
			(void)sa6_recoverscope(sin6);
#else
			(void)in6_recoverscope(sin6, &sin6->sin6_addr, NULL);
#endif	
#endif	
		}
		lsa6->sin6_port = inp->sctp_lport;

		if (ro->ro_rt == NULL) {
			




			sctp_handle_no_route(stcb, net, so_locked);
			SCTP_LTRACE_ERR_RET_PKT(m, inp, stcb, NULL, SCTP_FROM_SCTP_OUTPUT, EHOSTUNREACH);
			sctp_m_freem(m);
			return (EHOSTUNREACH);
		}
#ifndef SCOPEDROUTING
#ifdef SCTP_EMBEDDED_V6_SCOPE
		



		bzero(&lsa6_storage, sizeof(lsa6_storage));
		lsa6_storage.sin6_family = AF_INET6;
#ifdef HAVE_SIN6_LEN
		lsa6_storage.sin6_len = sizeof(lsa6_storage);
#endif
#ifdef SCTP_KAME
		lsa6_storage.sin6_addr = lsa6->sin6_addr;
		if ((error = sa6_recoverscope(&lsa6_storage)) != 0) {
#else
		if ((error = in6_recoverscope(&lsa6_storage, &lsa6->sin6_addr,
		    NULL)) != 0) {
#endif
			SCTPDBG(SCTP_DEBUG_OUTPUT3, "recover scope fails error %d\n", error);
			sctp_m_freem(m);
			return (error);
		}
		
		lsa6_storage.sin6_addr = lsa6->sin6_addr;
		lsa6_storage.sin6_port = inp->sctp_lport;
		lsa6 = &lsa6_storage;
#endif
#endif
		ip6h->ip6_src = lsa6->sin6_addr;

		if (port) {
			if (htons(SCTP_BASE_SYSCTL(sctp_udp_tunneling_port)) == 0) {
				sctp_handle_no_route(stcb, net, so_locked);
				SCTP_LTRACE_ERR_RET_PKT(m, inp, stcb, NULL, SCTP_FROM_SCTP_OUTPUT, EHOSTUNREACH);
				sctp_m_freem(m);
				return (EHOSTUNREACH);
			}
			udp = (struct udphdr *)((caddr_t)ip6h + sizeof(struct ip6_hdr));
			udp->uh_sport = htons(SCTP_BASE_SYSCTL(sctp_udp_tunneling_port));
			udp->uh_dport = port;
			udp->uh_ulen = htons(packet_length - sizeof(struct ip6_hdr));
			udp->uh_sum = 0;
			sctphdr = (struct sctphdr *)((caddr_t)udp + sizeof(struct udphdr));
		} else {
			sctphdr = (struct sctphdr *)((caddr_t)ip6h + sizeof(struct ip6_hdr));
		}

		sctphdr->src_port = src_port;
		sctphdr->dest_port = dest_port;
		sctphdr->v_tag = v_tag;
		sctphdr->checksum = 0;

		



		ip6h->ip6_hlim = SCTP_GET_HLIM(inp, ro);
#if !(defined(__Panda__) || defined(__Userspace__))
		ifp = SCTP_GET_IFN_VOID_FROM_ROUTE(ro);
#endif

#ifdef SCTP_DEBUG
		
		sin6->sin6_addr = ip6h->ip6_dst;
		lsa6->sin6_addr = ip6h->ip6_src;
#endif

		SCTPDBG(SCTP_DEBUG_OUTPUT3, "Calling ipv6 output routine from low level\n");
		SCTPDBG(SCTP_DEBUG_OUTPUT3, "src: ");
		SCTPDBG_ADDR(SCTP_DEBUG_OUTPUT3, (struct sockaddr *)lsa6);
		SCTPDBG(SCTP_DEBUG_OUTPUT3, "dst: ");
		SCTPDBG_ADDR(SCTP_DEBUG_OUTPUT3, (struct sockaddr *)sin6);
		if (net) {
			sin6 = (struct sockaddr_in6 *)&net->ro._l_addr;
			
			prev_scope = sin6->sin6_scope_id;
			prev_port = sin6->sin6_port;
		}

		if (SCTP_GET_HEADER_FOR_OUTPUT(o_pak)) {
			
			sctp_m_freem(m);
			SCTP_LTRACE_ERR_RET(inp, stcb, NULL, SCTP_FROM_SCTP_OUTPUT, ENOMEM);
			return (ENOMEM);
		}
		SCTP_ATTACH_CHAIN(o_pak, m, packet_length);
		if (port) {
#if defined(SCTP_WITH_NO_CSUM)
			SCTP_STAT_INCR(sctps_sendnocrc);
#else
			sctphdr->checksum = sctp_calculate_cksum(m, sizeof(struct ip6_hdr) + sizeof(struct udphdr));
			SCTP_STAT_INCR(sctps_sendswcrc);
#endif
#if defined(__Windows__)
			udp->uh_sum = 0;
#elif !defined(__Userspace__)
			if ((udp->uh_sum = in6_cksum(o_pak, IPPROTO_UDP, sizeof(struct ip6_hdr), packet_length - sizeof(struct ip6_hdr))) == 0) {
				udp->uh_sum = 0xffff;
			}
#endif
		} else {
#if defined(SCTP_WITH_NO_CSUM)
			SCTP_STAT_INCR(sctps_sendnocrc);
#else
#if defined(__FreeBSD__) && __FreeBSD_version >= 800000
#if __FreeBSD_version < 900000
			sctphdr->checksum = sctp_calculate_cksum(m, sizeof(struct ip6_hdr));
			SCTP_STAT_INCR(sctps_sendswcrc);
#else
#if __FreeBSD_version > 901000
			m->m_pkthdr.csum_flags = CSUM_SCTP_IPV6;
#else
			m->m_pkthdr.csum_flags = CSUM_SCTP;
#endif
			m->m_pkthdr.csum_data = 0;
			SCTP_STAT_INCR(sctps_sendhwcrc);
#endif
#else
			if (!(SCTP_BASE_SYSCTL(sctp_no_csum_on_loopback) &&
			      (stcb) && (stcb->asoc.scope.loopback_scope))) {
				sctphdr->checksum = sctp_calculate_cksum(m, sizeof(struct ip6_hdr));
				SCTP_STAT_INCR(sctps_sendswcrc);
			} else {
				SCTP_STAT_INCR(sctps_sendnocrc);
			}
#endif
#endif
		}
		
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
		if ((SCTP_BASE_SYSCTL(sctp_output_unlocked)) && (so_locked)) {
			so = SCTP_INP_SO(inp);
			SCTP_SOCKET_UNLOCK(so, 0);
		}
#endif
#ifdef SCTP_PACKET_LOGGING
		if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_LAST_PACKET_TRACING)
			sctp_packet_log(o_pak);
#endif
#if !(defined(__Panda__) || defined(__Userspace__))
		SCTP_IP6_OUTPUT(ret, o_pak, (struct route_in6 *)ro, &ifp, stcb, vrf_id);
#else
		SCTP_IP6_OUTPUT(ret, o_pak, (struct route_in6 *)ro, NULL, stcb, vrf_id);
#endif
#if defined(__APPLE__) || defined(SCTP_SO_LOCK_TESTING)
		if ((SCTP_BASE_SYSCTL(sctp_output_unlocked)) && (so_locked)) {
			atomic_add_int(&stcb->asoc.refcnt, 1);
			SCTP_TCB_UNLOCK(stcb);
			SCTP_SOCKET_LOCK(so, 0);
			SCTP_TCB_LOCK(stcb);
			atomic_subtract_int(&stcb->asoc.refcnt, 1);
		}
#endif
		if (net) {
			
			sin6->sin6_scope_id = prev_scope;
			sin6->sin6_port = prev_port;
		}
		SCTPDBG(SCTP_DEBUG_OUTPUT3, "return from send is %d\n", ret);
		SCTP_STAT_INCR(sctps_sendpackets);
		SCTP_STAT_INCR_COUNTER64(sctps_outpackets);
		if (ret) {
			SCTP_STAT_INCR(sctps_senderrors);
		}
		if (net == NULL) {
			
#if defined(__FreeBSD__) && __FreeBSD_version > 901000
			RO_RTFREE(ro);
#else
			if (ro->ro_rt) {
				RTFREE(ro->ro_rt);
			}
#endif
		} else {
			
			if (ro->ro_rt == NULL) {
				
				if (net->ro._s_addr &&
				    net->src_addr_selected) {
					sctp_free_ifa(net->ro._s_addr);
					net->ro._s_addr = NULL;
				}
				net->src_addr_selected = 0;
			}
			if ((ro->ro_rt != NULL) &&
			    (net->ro._s_addr)) {
				uint32_t mtu;
				mtu = SCTP_GATHER_MTU_FROM_ROUTE(net->ro._s_addr, &net->ro._l_addr.sa, ro->ro_rt);
				if (mtu &&
				    (stcb->asoc.smallest_mtu > mtu)) {
					sctp_mtu_size_reset(inp, &stcb->asoc, mtu);
					net->mtu = mtu;
					if (net->port) {
						net->mtu -= sizeof(struct udphdr);
					}
				}
			}
#if !defined(__Panda__) && !defined(__Userspace__)
			else if (ifp) {
#if defined(__Windows__)
#define ND_IFINFO(ifp)	(ifp)
#define linkmtu		if_mtu
#endif
				if (ND_IFINFO(ifp)->linkmtu &&
				    (stcb->asoc.smallest_mtu > ND_IFINFO(ifp)->linkmtu)) {
					sctp_mtu_size_reset(inp,
					    &stcb->asoc,
					    ND_IFINFO(ifp)->linkmtu);
				}
			}
#endif
		}
		return (ret);
	}
#endif
#if defined(__Userspace__)
	case AF_CONN:
	{
		char *buffer;
		struct sockaddr_conn *sconn;
		int len;

		sconn = (struct sockaddr_conn *)to;
		len = sizeof(struct sctphdr);
		newm = sctp_get_mbuf_for_msg(len, 1, M_NOWAIT, 1, MT_DATA);
		if (newm == NULL) {
			sctp_m_freem(m);
			SCTP_LTRACE_ERR_RET(inp, stcb, NULL, SCTP_FROM_SCTP_OUTPUT, ENOMEM);
			return (ENOMEM);
		}
		SCTP_ALIGN_TO_END(newm, len);
		SCTP_BUF_LEN(newm) = len;
		SCTP_BUF_NEXT(newm) = m;
		m = newm;
		packet_length = sctp_calculate_len(m);
		sctphdr = mtod(m, struct sctphdr *);
		sctphdr->src_port = src_port;
		sctphdr->dest_port = dest_port;
		sctphdr->v_tag = v_tag;
		sctphdr->checksum = 0;
		sctphdr->checksum = sctp_calculate_cksum(m, 0);
		SCTP_STAT_INCR(sctps_sendswcrc);
		if (tos_value == 0) {
			tos_value = inp->ip_inp.inp.inp_ip_tos;
		}
		tos_value &= 0xfc;
		if (ecn_ok) {
			tos_value |= sctp_get_ect(stcb);
		}
		
		if ((buffer = malloc(packet_length)) != NULL) {
			m_copydata(m, 0, packet_length, buffer);
			ret = SCTP_BASE_VAR(conn_output)(sconn->sconn_addr, buffer, packet_length, tos_value, nofragment_flag);
			free(buffer);
		} else {
			ret = ENOMEM;
		}
		sctp_m_freem(m);
		return (ret);
	}
#endif
	default:
		SCTPDBG(SCTP_DEBUG_OUTPUT1, "Unknown protocol (TSNH) type %d\n",
		        ((struct sockaddr *)to)->sa_family);
		sctp_m_freem(m);
		SCTP_LTRACE_ERR_RET_PKT(m, inp, stcb, net, SCTP_FROM_SCTP_OUTPUT, EFAULT);
		return (EFAULT);
	}
}


void
sctp_send_initiate(struct sctp_inpcb *inp, struct sctp_tcb *stcb, int so_locked
#if !defined(__APPLE__) && !defined(SCTP_SO_LOCK_TESTING)
    SCTP_UNUSED
#endif
    )
{
	struct mbuf *m;
	struct sctp_nets *net;
	struct sctp_init_chunk *init;
#if defined(INET) || defined(INET6)
	struct sctp_supported_addr_param *sup_addr;
#endif
	struct sctp_adaptation_layer_indication *ali;
	struct sctp_supported_chunk_types_param *pr_supported;
	struct sctp_paramhdr *ph;
	int cnt_inits_to = 0;
	int ret;
	uint16_t num_ext, chunk_len, padding_len, parameter_len;

#if defined(__APPLE__)
	if (so_locked) {
		sctp_lock_assert(SCTP_INP_SO(inp));
	} else {
		sctp_unlock_assert(SCTP_INP_SO(inp));
	}
#endif
	
	net = stcb->asoc.primary_destination;
	if (net == NULL) {
		net = TAILQ_FIRST(&stcb->asoc.nets);
		if (net == NULL) {
			
			return;
		}
		
		net->dest_state &= ~SCTP_ADDR_UNCONFIRMED;
		(void)sctp_set_primary_addr(stcb, NULL, net);
	} else {
		
		net->dest_state &= ~SCTP_ADDR_UNCONFIRMED;
	}
	SCTPDBG(SCTP_DEBUG_OUTPUT4, "Sending INIT\n");
#ifdef INET6
	if (net->ro._l_addr.sa.sa_family == AF_INET6) {
		



		if (IN6_IS_ADDR_LINKLOCAL(&net->ro._l_addr.sin6.sin6_addr))
			cnt_inits_to = 1;
	}
#endif
	if (SCTP_OS_TIMER_PENDING(&net->rxt_timer.timer)) {
		
		SCTPDBG(SCTP_DEBUG_OUTPUT4, "Sending INIT - failed timer?\n");
		return;
	}
	
	sctp_timer_start(SCTP_TIMER_TYPE_INIT, inp, stcb, net);

	m = sctp_get_mbuf_for_msg(MCLBYTES, 1, M_NOWAIT, 1, MT_DATA);
	if (m == NULL) {
		
		SCTPDBG(SCTP_DEBUG_OUTPUT4, "Sending INIT - mbuf?\n");
		return;
	}
	chunk_len = (uint16_t)sizeof(struct sctp_init_chunk);
	padding_len = 0;
	




	stcb->asoc.peer_supports_asconf = 1;
	
	init = mtod(m, struct sctp_init_chunk *);
	
	init->ch.chunk_type = SCTP_INITIATION;
	init->ch.chunk_flags = 0;
	
	init->ch.chunk_length = 0;
	
	init->init.initiate_tag = htonl(stcb->asoc.my_vtag);
	
	init->init.a_rwnd = htonl(max(inp->sctp_socket?SCTP_SB_LIMIT_RCV(inp->sctp_socket):0,
	                              SCTP_MINIMAL_RWND));
	init->init.num_outbound_streams = htons(stcb->asoc.pre_open_streams);
	init->init.num_inbound_streams = htons(stcb->asoc.max_inbound_streams);
	init->init.initial_tsn = htonl(stcb->asoc.init_seq_number);

	if (stcb->asoc.scope.ipv4_addr_legal || stcb->asoc.scope.ipv6_addr_legal) {
		uint8_t i;

		parameter_len = (uint16_t)sizeof(struct sctp_paramhdr);
		if (stcb->asoc.scope.ipv4_addr_legal) {
			parameter_len += (uint16_t)sizeof(uint16_t);
		}
		if (stcb->asoc.scope.ipv6_addr_legal) {
			parameter_len += (uint16_t)sizeof(uint16_t);
		}
		sup_addr = (struct sctp_supported_addr_param *)(mtod(m, caddr_t) + chunk_len);
		sup_addr->ph.param_type = htons(SCTP_SUPPORTED_ADDRTYPE);
		sup_addr->ph.param_length = htons(parameter_len);
		i = 0;
		if (stcb->asoc.scope.ipv4_addr_legal) {
			sup_addr->addr_type[i++] = htons(SCTP_IPV4_ADDRESS);
		}
		if (stcb->asoc.scope.ipv6_addr_legal) {
			sup_addr->addr_type[i++] = htons(SCTP_IPV6_ADDRESS);
		}
		padding_len = 4 - 2 * i;
		chunk_len += parameter_len;
	}

	
	if (inp->sctp_ep.adaptation_layer_indicator_provided) {
		if (padding_len > 0) {
			memset(mtod(m, caddr_t) + chunk_len, 0, padding_len);
			chunk_len += padding_len;
			padding_len = 0;
		}
		parameter_len = (uint16_t)sizeof(struct sctp_adaptation_layer_indication);
		ali = (struct sctp_adaptation_layer_indication *)(mtod(m, caddr_t) + chunk_len);
		ali->ph.param_type = htons(SCTP_ULP_ADAPTATION);
		ali->ph.param_length = htons(parameter_len);
		ali->indication = ntohl(inp->sctp_ep.adaptation_layer_indicator);
		chunk_len += parameter_len;
	}

	if (SCTP_BASE_SYSCTL(sctp_inits_include_nat_friendly)) {
		
		if (padding_len > 0) {
			memset(mtod(m, caddr_t) + chunk_len, 0, padding_len);
			chunk_len += padding_len;
			padding_len = 0;
		}
		parameter_len = (uint16_t)sizeof(struct sctp_paramhdr);
		ph = (struct sctp_paramhdr *)(mtod(m, caddr_t) + chunk_len);
		ph->param_type = htons(SCTP_HAS_NAT_SUPPORT);
		ph->param_length = htons(parameter_len);
		chunk_len += parameter_len;
	}

	
	if (stcb->asoc.cookie_preserve_req) {
		struct sctp_cookie_perserve_param *cookie_preserve;

		if (padding_len > 0) {
			memset(mtod(m, caddr_t) + chunk_len, 0, padding_len);
			chunk_len += padding_len;
			padding_len = 0;
		}
		parameter_len = (uint16_t)sizeof(struct sctp_cookie_perserve_param);
		cookie_preserve = (struct sctp_cookie_perserve_param *)(mtod(m, caddr_t) + chunk_len);
		cookie_preserve->ph.param_type = htons(SCTP_COOKIE_PRESERVE);
		cookie_preserve->ph.param_length = htons(parameter_len);
		cookie_preserve->time = htonl(stcb->asoc.cookie_preserve_req);
		stcb->asoc.cookie_preserve_req = 0;
		chunk_len += parameter_len;
	}

	
	if (stcb->asoc.ecn_allowed == 1) {
		if (padding_len > 0) {
			memset(mtod(m, caddr_t) + chunk_len, 0, padding_len);
			chunk_len += padding_len;
			padding_len = 0;
		}
		parameter_len = (uint16_t)sizeof(struct sctp_paramhdr);
		ph = (struct sctp_paramhdr *)(mtod(m, caddr_t) + chunk_len);
		ph->param_type = htons(SCTP_ECN_CAPABLE);
		ph->param_length = htons(parameter_len);
		chunk_len += parameter_len;
	}

	
	if (padding_len > 0) {
		memset(mtod(m, caddr_t) + chunk_len, 0, padding_len);
		chunk_len += padding_len;
		padding_len = 0;
	}
	parameter_len = (uint16_t)sizeof(struct sctp_paramhdr);
	ph = (struct sctp_paramhdr *)(mtod(m, caddr_t) + chunk_len);
	ph->param_type = htons(SCTP_PRSCTP_SUPPORTED);
	ph->param_length = htons(parameter_len);
	chunk_len += parameter_len;

	
	pr_supported = (struct sctp_supported_chunk_types_param *)(mtod(m, caddr_t) + chunk_len);
	pr_supported->ph.param_type = htons(SCTP_SUPPORTED_CHUNK_EXT);
	num_ext = 0;
	pr_supported->chunk_types[num_ext++] = SCTP_ASCONF;
	pr_supported->chunk_types[num_ext++] = SCTP_ASCONF_ACK;
	pr_supported->chunk_types[num_ext++] = SCTP_FORWARD_CUM_TSN;
	pr_supported->chunk_types[num_ext++] = SCTP_PACKET_DROPPED;
	pr_supported->chunk_types[num_ext++] = SCTP_STREAM_RESET;
	if (!SCTP_BASE_SYSCTL(sctp_auth_disable)) {
		pr_supported->chunk_types[num_ext++] = SCTP_AUTHENTICATION;
	}
	if (stcb->asoc.sctp_nr_sack_on_off == 1) {
		pr_supported->chunk_types[num_ext++] = SCTP_NR_SELECTIVE_ACK;
	}
	parameter_len = (uint16_t)sizeof(struct sctp_supported_chunk_types_param) + num_ext;
	pr_supported->ph.param_length = htons(parameter_len);
	padding_len = SCTP_SIZE32(parameter_len) - parameter_len;
	chunk_len += parameter_len;

	
	if (!SCTP_BASE_SYSCTL(sctp_auth_disable)) {
		
		if (stcb->asoc.authinfo.random != NULL) {
			struct sctp_auth_random *randp;

			if (padding_len > 0) {
				memset(mtod(m, caddr_t) + chunk_len, 0, padding_len);
				chunk_len += padding_len;
				padding_len = 0;
			}
			randp = (struct sctp_auth_random *)(mtod(m, caddr_t) + chunk_len);
			parameter_len = (uint16_t)sizeof(struct sctp_auth_random) + stcb->asoc.authinfo.random_len;
			
			memcpy(randp, stcb->asoc.authinfo.random->key, parameter_len);
			padding_len = SCTP_SIZE32(parameter_len) - parameter_len;
			chunk_len += parameter_len;
		}
		
		if ((stcb->asoc.local_hmacs != NULL) &&
		    (stcb->asoc.local_hmacs->num_algo > 0)) {
			struct sctp_auth_hmac_algo *hmacs;

			if (padding_len > 0) {
				memset(mtod(m, caddr_t) + chunk_len, 0, padding_len);
				chunk_len += padding_len;
				padding_len = 0;
			}
			hmacs = (struct sctp_auth_hmac_algo *)(mtod(m, caddr_t) + chunk_len);
			parameter_len = (uint16_t)(sizeof(struct sctp_auth_hmac_algo) +
			                           stcb->asoc.local_hmacs->num_algo * sizeof(uint16_t));
			hmacs->ph.param_type = htons(SCTP_HMAC_LIST);
			hmacs->ph.param_length = htons(parameter_len);
			sctp_serialize_hmaclist(stcb->asoc.local_hmacs, (uint8_t *)hmacs->hmac_ids);
			padding_len = SCTP_SIZE32(parameter_len) - parameter_len;
			chunk_len += parameter_len;
		}
		
		if (sctp_auth_get_chklist_size(stcb->asoc.local_auth_chunks) > 0) {
			struct sctp_auth_chunk_list *chunks;

			if (padding_len > 0) {
				memset(mtod(m, caddr_t) + chunk_len, 0, padding_len);
				chunk_len += padding_len;
				padding_len = 0;
			}
			chunks = (struct sctp_auth_chunk_list *)(mtod(m, caddr_t) + chunk_len);
			parameter_len = (uint16_t)(sizeof(struct sctp_auth_chunk_list) +
			                           sctp_auth_get_chklist_size(stcb->asoc.local_auth_chunks));
			chunks->ph.param_type = htons(SCTP_CHUNK_LIST);
			chunks->ph.param_length = htons(parameter_len);
			sctp_serialize_auth_chunks(stcb->asoc.local_auth_chunks, chunks->chunk_types);
			padding_len = SCTP_SIZE32(parameter_len) - parameter_len;
			chunk_len += parameter_len;
		}
	}
	SCTP_BUF_LEN(m) = chunk_len;

	
	





	sctp_add_addresses_to_i_ia(inp, stcb, &stcb->asoc.scope, m, cnt_inits_to, &padding_len, &chunk_len);

	init->ch.chunk_length = htons(chunk_len);
	if (padding_len > 0) {
		struct mbuf *m_at, *mp_last;

		mp_last = NULL;
		for (m_at = m; m_at; m_at = SCTP_BUF_NEXT(m_at)) {
			if (SCTP_BUF_NEXT(m_at) == NULL)
				mp_last = m_at;
		}
		if ((mp_last == NULL) || sctp_add_pad_tombuf(mp_last, padding_len)) {
			sctp_m_freem(m);
			return;
		}
	}
	SCTPDBG(SCTP_DEBUG_OUTPUT4, "Sending INIT - calls lowlevel_output\n");
	ret = sctp_lowlevel_chunk_output(inp, stcb, net,
	                                 (struct sockaddr *)&net->ro._l_addr,
	                                 m, 0, NULL, 0, 0, 0, 0,
	                                 inp->sctp_lport, stcb->rport, htonl(0),
	                                 net->port, NULL,
#if defined(__FreeBSD__)
	                                 0, 0,
#endif
	                                 so_locked);
	SCTPDBG(SCTP_DEBUG_OUTPUT4, "lowlevel_output - %d\n", ret);
	SCTP_STAT_INCR_COUNTER64(sctps_outcontrolchunks);
	(void)SCTP_GETTIME_TIMEVAL(&net->last_sent_time);
}

struct mbuf *
sctp_arethere_unrecognized_parameters(struct mbuf *in_initpkt,
	int param_offset, int *abort_processing, struct sctp_chunkhdr *cp, int *nat_friendly)
{
	













	struct sctp_paramhdr *phdr, params;

	struct mbuf *mat, *op_err;
	char tempbuf[SCTP_PARAM_BUFFER_SIZE];
	int at, limit, pad_needed;
	uint16_t ptype, plen, padded_size;
	int err_at;

	*abort_processing = 0;
	mat = in_initpkt;
	err_at = 0;
	limit = ntohs(cp->chunk_length) - sizeof(struct sctp_init_chunk);
	at = param_offset;
	op_err = NULL;
	SCTPDBG(SCTP_DEBUG_OUTPUT1, "Check for unrecognized param's\n");
	phdr = sctp_get_next_param(mat, at, &params, sizeof(params));
	while ((phdr != NULL) && ((size_t)limit >= sizeof(struct sctp_paramhdr))) {
		ptype = ntohs(phdr->param_type);
		plen = ntohs(phdr->param_length);
		if ((plen > limit) || (plen < sizeof(struct sctp_paramhdr))) {
			
			SCTPDBG(SCTP_DEBUG_OUTPUT1, "Invalid size - error %d\n", plen);
			goto invalid_size;
		}
		limit -= SCTP_SIZE32(plen);
		






		padded_size = SCTP_SIZE32(plen);
		switch (ptype) {
			
		case SCTP_HEARTBEAT_INFO:
		case SCTP_STATE_COOKIE:
		case SCTP_UNRECOG_PARAM:
		case SCTP_ERROR_CAUSE_IND:
			
			at += padded_size;
			break;
			
		case SCTP_CHUNK_LIST:
		case SCTP_SUPPORTED_CHUNK_EXT:
			if (padded_size > (sizeof(struct sctp_supported_chunk_types_param) + (sizeof(uint8_t) * SCTP_MAX_SUPPORTED_EXT))) {
				SCTPDBG(SCTP_DEBUG_OUTPUT1, "Invalid size - error chklist %d\n", plen);
				goto invalid_size;
			}
			at += padded_size;
			break;
		case SCTP_SUPPORTED_ADDRTYPE:
			if (padded_size > SCTP_MAX_ADDR_PARAMS_SIZE) {
				SCTPDBG(SCTP_DEBUG_OUTPUT1, "Invalid size - error supaddrtype %d\n", plen);
				goto invalid_size;
			}
			at += padded_size;
			break;
		case SCTP_RANDOM:
			if (padded_size > (sizeof(struct sctp_auth_random) + SCTP_RANDOM_MAX_SIZE)) {
				SCTPDBG(SCTP_DEBUG_OUTPUT1, "Invalid size - error random %d\n", plen);
				goto invalid_size;
			}
			at += padded_size;
			break;
		case SCTP_SET_PRIM_ADDR:
		case SCTP_DEL_IP_ADDRESS:
		case SCTP_ADD_IP_ADDRESS:
			if ((padded_size != sizeof(struct sctp_asconf_addrv4_param)) &&
			    (padded_size != sizeof(struct sctp_asconf_addr_param))) {
				SCTPDBG(SCTP_DEBUG_OUTPUT1, "Invalid size - error setprim %d\n", plen);
				goto invalid_size;
			}
			at += padded_size;
			break;
			
		case SCTP_IPV4_ADDRESS:
			if (padded_size != sizeof(struct sctp_ipv4addr_param)) {
				SCTPDBG(SCTP_DEBUG_OUTPUT1, "Invalid size - error ipv4 addr %d\n", plen);
				goto invalid_size;
			}
			at += padded_size;
			break;
		case SCTP_IPV6_ADDRESS:
			if (padded_size != sizeof(struct sctp_ipv6addr_param)) {
				SCTPDBG(SCTP_DEBUG_OUTPUT1, "Invalid size - error ipv6 addr %d\n", plen);
				goto invalid_size;
			}
			at += padded_size;
			break;
		case SCTP_COOKIE_PRESERVE:
			if (padded_size != sizeof(struct sctp_cookie_perserve_param)) {
				SCTPDBG(SCTP_DEBUG_OUTPUT1, "Invalid size - error cookie-preserve %d\n", plen);
				goto invalid_size;
			}
			at += padded_size;
			break;
		case SCTP_HAS_NAT_SUPPORT:
		  *nat_friendly = 1;
		  
		case SCTP_PRSCTP_SUPPORTED:

			if (padded_size != sizeof(struct sctp_paramhdr)) {
				SCTPDBG(SCTP_DEBUG_OUTPUT1, "Invalid size - error prsctp/nat support %d\n", plen);
				goto invalid_size;
			}
			at += padded_size;
			break;
		case SCTP_ECN_CAPABLE:
			if (padded_size != sizeof(struct sctp_ecn_supported_param)) {
				SCTPDBG(SCTP_DEBUG_OUTPUT1, "Invalid size - error ecn %d\n", plen);
				goto invalid_size;
			}
			at += padded_size;
			break;
		case SCTP_ULP_ADAPTATION:
			if (padded_size != sizeof(struct sctp_adaptation_layer_indication)) {
				SCTPDBG(SCTP_DEBUG_OUTPUT1, "Invalid size - error adapatation %d\n", plen);
				goto invalid_size;
			}
			at += padded_size;
			break;
		case SCTP_SUCCESS_REPORT:
			if (padded_size != sizeof(struct sctp_asconf_paramhdr)) {
				SCTPDBG(SCTP_DEBUG_OUTPUT1, "Invalid size - error success %d\n", plen);
				goto invalid_size;
			}
			at += padded_size;
			break;
		case SCTP_HOSTNAME_ADDRESS:
		{
			
			int l_len;

			SCTPDBG(SCTP_DEBUG_OUTPUT1, "Can't handle hostname addresses.. abort processing\n");
			*abort_processing = 1;
			if (op_err == NULL) {
				
#ifdef INET6
				l_len = sizeof(struct ip6_hdr) + sizeof(struct sctphdr) + sizeof(struct sctp_chunkhdr);
#else
				l_len = sizeof(struct ip) + sizeof(struct sctphdr) + sizeof(struct sctp_chunkhdr);
#endif
				l_len += plen;
				l_len += sizeof(struct sctp_paramhdr);
				op_err = sctp_get_mbuf_for_msg(l_len, 0, M_NOWAIT, 1, MT_DATA);
				if (op_err) {
					SCTP_BUF_LEN(op_err) = 0;
					



#ifdef INET6
					SCTP_BUF_RESV_UF(op_err, sizeof(struct ip6_hdr));
#else
					SCTP_BUF_RESV_UF(op_err, sizeof(struct ip));
#endif
					SCTP_BUF_RESV_UF(op_err, sizeof(struct sctphdr));
					SCTP_BUF_RESV_UF(op_err, sizeof(struct sctp_chunkhdr));
				}
			}
			if (op_err) {
				
				struct sctp_paramhdr s;

				if (err_at % 4) {
					uint32_t cpthis = 0;

					pad_needed = 4 - (err_at % 4);
					m_copyback(op_err, err_at, pad_needed, (caddr_t)&cpthis);
					err_at += pad_needed;
				}
				s.param_type = htons(SCTP_CAUSE_UNRESOLVABLE_ADDR);
				s.param_length = htons(sizeof(s) + plen);
				m_copyback(op_err, err_at, sizeof(s), (caddr_t)&s);
				err_at += sizeof(s);
				phdr = sctp_get_next_param(mat, at, (struct sctp_paramhdr *)tempbuf, min(sizeof(tempbuf),plen));
				if (phdr == NULL) {
					sctp_m_freem(op_err);
					





					return (NULL);
				}
				m_copyback(op_err, err_at, plen, (caddr_t)phdr);
			}
			return (op_err);
			break;
		}
		default:
			



			SCTPDBG(SCTP_DEBUG_OUTPUT1, "Hit default param %x\n", ptype);
			if ((ptype & 0x4000) == 0x4000) {
				
				SCTPDBG(SCTP_DEBUG_OUTPUT1, "report op err\n");
				if (op_err == NULL) {
					int l_len;
					
#ifdef INET6
					l_len = sizeof(struct ip6_hdr) + sizeof(struct sctphdr) + sizeof(struct sctp_chunkhdr);
#else
					l_len = sizeof(struct ip) + sizeof(struct sctphdr) + sizeof(struct sctp_chunkhdr);
#endif
					l_len += plen;
					l_len += sizeof(struct sctp_paramhdr);
					op_err = sctp_get_mbuf_for_msg(l_len, 0, M_NOWAIT, 1, MT_DATA);
					if (op_err) {
						SCTP_BUF_LEN(op_err) = 0;
#ifdef INET6
						SCTP_BUF_RESV_UF(op_err, sizeof(struct ip6_hdr));
#else
						SCTP_BUF_RESV_UF(op_err, sizeof(struct ip));
#endif
						SCTP_BUF_RESV_UF(op_err, sizeof(struct sctphdr));
						SCTP_BUF_RESV_UF(op_err, sizeof(struct sctp_chunkhdr));
					}
				}
				if (op_err) {
					
					struct sctp_paramhdr s;

					if (err_at % 4) {
						uint32_t cpthis = 0;

						pad_needed = 4 - (err_at % 4);
						m_copyback(op_err, err_at, pad_needed, (caddr_t)&cpthis);
						err_at += pad_needed;
					}
					s.param_type = htons(SCTP_UNRECOG_PARAM);
					s.param_length = htons(sizeof(s) + plen);
					m_copyback(op_err, err_at, sizeof(s), (caddr_t)&s);
					err_at += sizeof(s);
					if (plen > sizeof(tempbuf)) {
						plen = sizeof(tempbuf);
					}
					phdr = sctp_get_next_param(mat, at, (struct sctp_paramhdr *)tempbuf, min(sizeof(tempbuf),plen));
					if (phdr == NULL) {
						sctp_m_freem(op_err);
						






						op_err = NULL;
						goto more_processing;
					}
					m_copyback(op_err, err_at, plen, (caddr_t)phdr);
					err_at += plen;
				}
			}
		more_processing:
			if ((ptype & 0x8000) == 0x0000) {
				SCTPDBG(SCTP_DEBUG_OUTPUT1, "stop proc\n");
				return (op_err);
			} else {
				
				SCTPDBG(SCTP_DEBUG_OUTPUT1, "move on\n");
				at += SCTP_SIZE32(plen);
			}
			break;

		}
		phdr = sctp_get_next_param(mat, at, &params, sizeof(params));
	}
	return (op_err);
 invalid_size:
	SCTPDBG(SCTP_DEBUG_OUTPUT1, "abort flag set\n");
	*abort_processing = 1;
	if ((op_err == NULL) && phdr) {
		int l_len;
#ifdef INET6
		l_len = sizeof(struct ip6_hdr) + sizeof(struct sctphdr) + sizeof(struct sctp_chunkhdr);
#else
		l_len = sizeof(struct ip) + sizeof(struct sctphdr) + sizeof(struct sctp_chunkhdr);
#endif
		l_len += (2 * sizeof(struct sctp_paramhdr));
		op_err = sctp_get_mbuf_for_msg(l_len, 0, M_NOWAIT, 1, MT_DATA);
        if (op_err) {
    		SCTP_BUF_LEN(op_err) = 0;
#ifdef INET6
	    	SCTP_BUF_RESV_UF(op_err, sizeof(struct ip6_hdr));
#else
	    	SCTP_BUF_RESV_UF(op_err, sizeof(struct ip));
#endif
		SCTP_BUF_RESV_UF(op_err, sizeof(struct sctphdr));
    		SCTP_BUF_RESV_UF(op_err, sizeof(struct sctp_chunkhdr));
        }
	}
	if ((op_err) && phdr) {
		struct sctp_paramhdr s;

		if (err_at % 4) {
			uint32_t cpthis = 0;

			pad_needed = 4 - (err_at % 4);
			m_copyback(op_err, err_at, pad_needed, (caddr_t)&cpthis);
			err_at += pad_needed;
		}
		s.param_type = htons(SCTP_CAUSE_PROTOCOL_VIOLATION);
		s.param_length = htons(sizeof(s) + sizeof(struct sctp_paramhdr));
		m_copyback(op_err, err_at, sizeof(s), (caddr_t)&s);
		err_at += sizeof(s);
		
		m_copyback(op_err, err_at, sizeof(struct sctp_paramhdr), (caddr_t)phdr);
	}
	return (op_err);
}

static int
sctp_are_there_new_addresses(struct sctp_association *asoc,
    struct mbuf *in_initpkt, int offset, struct sockaddr *src)
{
	






	struct sockaddr *sa_touse;
	struct sockaddr *sa;
	struct sctp_paramhdr *phdr, params;
	uint16_t ptype, plen;
	uint8_t fnd;
	struct sctp_nets *net;
#ifdef INET
	struct sockaddr_in sin4, *sa4;
#endif
#ifdef INET6
	struct sockaddr_in6 sin6, *sa6;
#endif

#ifdef INET
	memset(&sin4, 0, sizeof(sin4));
	sin4.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
	sin4.sin_len = sizeof(sin4);
#endif
#endif
#ifdef INET6
	memset(&sin6, 0, sizeof(sin6));
	sin6.sin6_family = AF_INET6;
#ifdef HAVE_SIN6_LEN
	sin6.sin6_len = sizeof(sin6);
#endif
#endif
	
	fnd = 0;
	TAILQ_FOREACH(net, &asoc->nets, sctp_next) {
		sa = (struct sockaddr *)&net->ro._l_addr;
		if (sa->sa_family == src->sa_family) {
#ifdef INET
			if (sa->sa_family == AF_INET) {
				struct sockaddr_in *src4;

				sa4 = (struct sockaddr_in *)sa;
				src4 = (struct sockaddr_in *)src;
				if (sa4->sin_addr.s_addr == src4->sin_addr.s_addr) {
					fnd = 1;
					break;
				}
			}
#endif
#ifdef INET6
			if (sa->sa_family == AF_INET6) {
				struct sockaddr_in6 *src6;

				sa6 = (struct sockaddr_in6 *)sa;
				src6 = (struct sockaddr_in6 *)src;
				if (SCTP6_ARE_ADDR_EQUAL(sa6, src6)) {
					fnd = 1;
					break;
				}
			}
#endif
		}
	}
	if (fnd == 0) {
		
		return (1);
	}
	
	offset += sizeof(struct sctp_init_chunk);
	phdr = sctp_get_next_param(in_initpkt, offset, &params, sizeof(params));
	while (phdr) {
		sa_touse = NULL;
		ptype = ntohs(phdr->param_type);
		plen = ntohs(phdr->param_length);
		switch (ptype) {
#ifdef INET
		case SCTP_IPV4_ADDRESS:
		{
			struct sctp_ipv4addr_param *p4, p4_buf;

			phdr = sctp_get_next_param(in_initpkt, offset,
			    (struct sctp_paramhdr *)&p4_buf, sizeof(p4_buf));
			if (plen != sizeof(struct sctp_ipv4addr_param) ||
			    phdr == NULL) {
				return (1);
			}
			p4 = (struct sctp_ipv4addr_param *)phdr;
			sin4.sin_addr.s_addr = p4->addr;
			sa_touse = (struct sockaddr *)&sin4;
			break;
		}
#endif
#ifdef INET6
		case SCTP_IPV6_ADDRESS:
		{
			struct sctp_ipv6addr_param *p6, p6_buf;

			phdr = sctp_get_next_param(in_initpkt, offset,
			    (struct sctp_paramhdr *)&p6_buf, sizeof(p6_buf));
			if (plen != sizeof(struct sctp_ipv6addr_param) ||
			    phdr == NULL) {
				return (1);
			}
			p6 = (struct sctp_ipv6addr_param *)phdr;
			memcpy((caddr_t)&sin6.sin6_addr, p6->addr,
			    sizeof(p6->addr));
			sa_touse = (struct sockaddr *)&sin6;
			break;
		}
#endif
		default:
			sa_touse = NULL;
			break;
		}
		if (sa_touse) {
			
			fnd = 0;
			TAILQ_FOREACH(net, &asoc->nets, sctp_next) {
				sa = (struct sockaddr *)&net->ro._l_addr;
				if (sa->sa_family != sa_touse->sa_family) {
					continue;
				}
#ifdef INET
				if (sa->sa_family == AF_INET) {
					sa4 = (struct sockaddr_in *)sa;
					if (sa4->sin_addr.s_addr ==
					    sin4.sin_addr.s_addr) {
						fnd = 1;
						break;
					}
				}
#endif
#ifdef INET6
				if (sa->sa_family == AF_INET6) {
					sa6 = (struct sockaddr_in6 *)sa;
					if (SCTP6_ARE_ADDR_EQUAL(
					    sa6, &sin6)) {
						fnd = 1;
						break;
					}
				}
#endif
			}
			if (!fnd) {
				
				return (1);
			}
		}
		offset += SCTP_SIZE32(plen);
		phdr = sctp_get_next_param(in_initpkt, offset, &params, sizeof(params));
	}
	return (0);
}







void
sctp_send_initiate_ack(struct sctp_inpcb *inp, struct sctp_tcb *stcb,
                       struct mbuf *init_pkt, int iphlen, int offset,
                       struct sockaddr *src, struct sockaddr *dst,
                       struct sctphdr *sh, struct sctp_init_chunk *init_chk,
#if defined(__FreeBSD__)
		       uint8_t use_mflowid, uint32_t mflowid,
#endif
                       uint32_t vrf_id, uint16_t port, int hold_inp_lock)
{
	struct sctp_association *asoc;
	struct mbuf *m, *m_at, *m_tmp, *m_cookie, *op_err, *mp_last;
	struct sctp_init_ack_chunk *initack;
	struct sctp_adaptation_layer_indication *ali;
	struct sctp_ecn_supported_param *ecn;
	struct sctp_prsctp_supported_param *prsctp;
	struct sctp_supported_chunk_types_param *pr_supported;
	union sctp_sockstore *over_addr;
#ifdef INET
	struct sockaddr_in *dst4 = (struct sockaddr_in *)dst;
	struct sockaddr_in *src4 = (struct sockaddr_in *)src;
	struct sockaddr_in *sin;
#endif
#ifdef INET6
	struct sockaddr_in6 *dst6 = (struct sockaddr_in6 *)dst;
	struct sockaddr_in6 *src6 = (struct sockaddr_in6 *)src;
	struct sockaddr_in6 *sin6;
#endif
#if defined(__Userspace__)
	struct sockaddr_conn *dstconn = (struct sockaddr_conn *)dst;
	struct sockaddr_conn *srcconn = (struct sockaddr_conn *)src;
	struct sockaddr_conn *sconn;
#endif
	struct sockaddr *to;
	struct sctp_state_cookie stc;
	struct sctp_nets *net = NULL;
	uint8_t *signature = NULL;
	int cnt_inits_to = 0;
	uint16_t his_limit, i_want;
	int abort_flag, padval;
	int num_ext;
	int p_len;
	int nat_friendly = 0;
	struct socket *so;

	if (stcb) {
		asoc = &stcb->asoc;
	} else {
		asoc = NULL;
	}
	mp_last = NULL;
	if ((asoc != NULL) &&
	    (SCTP_GET_STATE(asoc) != SCTP_STATE_COOKIE_WAIT) &&
	    (sctp_are_there_new_addresses(asoc, init_pkt, offset, src))) {
		
		




		sctp_send_abort(init_pkt, iphlen, src, dst, sh, 0, NULL,
#if defined(__FreeBSD__)
		                use_mflowid, mflowid,
#endif
		                vrf_id, port);
		return;
	}
	abort_flag = 0;
	op_err = sctp_arethere_unrecognized_parameters(init_pkt,
						       (offset + sizeof(struct sctp_init_chunk)),
						       &abort_flag, (struct sctp_chunkhdr *)init_chk, &nat_friendly);
	if (abort_flag) {
	do_a_abort:
		sctp_send_abort(init_pkt, iphlen, src, dst, sh,
				init_chk->init.initiate_tag, op_err,
#if defined(__FreeBSD__)
		                use_mflowid, mflowid,
#endif
		                vrf_id, port);
		return;
	}
	m = sctp_get_mbuf_for_msg(MCLBYTES, 0, M_NOWAIT, 1, MT_DATA);
	if (m == NULL) {
		
		if (op_err)
			sctp_m_freem(op_err);
		return;
	}
	SCTP_BUF_LEN(m) = sizeof(struct sctp_init_chunk);

	
	(void)SCTP_GETTIME_TIMEVAL(&stc.time_entered);

	
	if (asoc != NULL) {
		
		stc.tie_tag_my_vtag = asoc->my_vtag_nonce;
		stc.tie_tag_peer_vtag = asoc->peer_vtag_nonce;
		stc.cookie_life = asoc->cookie_life;
		net = asoc->primary_destination;
	} else {
		stc.tie_tag_my_vtag = 0;
		stc.tie_tag_peer_vtag = 0;
		
		stc.cookie_life = inp->sctp_ep.def_cookie_life;
	}

	
	stc.myport = sh->dest_port;
	stc.peerport = sh->src_port;

	



	stc.site_scope = stc.local_scope = stc.loopback_scope = 0;
	if (inp->sctp_flags & SCTP_PCB_FLAGS_BOUND_V6) {
		stc.ipv6_addr_legal = 1;
		if (SCTP_IPV6_V6ONLY(inp)) {
			stc.ipv4_addr_legal = 0;
		} else {
			stc.ipv4_addr_legal = 1;
		}
#if defined(__Userspace__)
		stc.conn_addr_legal = 0;
#endif
	} else {
		stc.ipv6_addr_legal = 0;
#if defined(__Userspace__)
		if (inp->sctp_flags & SCTP_PCB_FLAGS_BOUND_CONN) {
			stc.conn_addr_legal = 1;
			stc.ipv4_addr_legal = 0;
		} else {
			stc.conn_addr_legal = 0;
			stc.ipv4_addr_legal = 1;
		}
#else
		stc.ipv4_addr_legal = 1;
#endif
	}
#ifdef SCTP_DONT_DO_PRIVADDR_SCOPE
	stc.ipv4_scope = 1;
#else
	stc.ipv4_scope = 0;
#endif
	if (net == NULL) {
		to = src;
		switch (dst->sa_family) {
#ifdef INET
		case AF_INET:
		{
			
			stc.address[0] = src4->sin_addr.s_addr;
			stc.address[1] = 0;
			stc.address[2] = 0;
			stc.address[3] = 0;
			stc.addr_type = SCTP_IPV4_ADDRESS;
			
			stc.laddress[0] = dst4->sin_addr.s_addr;
			stc.laddress[1] = 0;
			stc.laddress[2] = 0;
			stc.laddress[3] = 0;
			stc.laddr_type = SCTP_IPV4_ADDRESS;
			
			stc.scope_id = 0;
#ifndef SCTP_DONT_DO_PRIVADDR_SCOPE
			if (IN4_ISPRIVATE_ADDRESS(&src4->sin_addr)) {
				stc.ipv4_scope = 1;
			}
#else
			stc.ipv4_scope = 1;
#endif				
			
			if (sctp_is_address_on_local_host(src, vrf_id)) {
				stc.loopback_scope = 1;
				stc.ipv4_scope = 1;
				stc.site_scope = 1;
				stc.local_scope = 0;
			}
			break;
		}
#endif
#ifdef INET6
		case AF_INET6:
		{
			stc.addr_type = SCTP_IPV6_ADDRESS;
			memcpy(&stc.address, &src6->sin6_addr, sizeof(struct in6_addr));
#if defined(__FreeBSD__) && (((__FreeBSD_version < 900000) && (__FreeBSD_version >= 804000)) || (__FreeBSD_version > 900000))
			stc.scope_id = in6_getscope(&src6->sin6_addr);
#else
			stc.scope_id = 0;
#endif
			if (sctp_is_address_on_local_host(src, vrf_id)) {
				stc.loopback_scope = 1;
				stc.local_scope = 0;
				stc.site_scope = 1;
				stc.ipv4_scope = 1;
			} else if (IN6_IS_ADDR_LINKLOCAL(&src6->sin6_addr)) {
				








#if defined(__APPLE__)
				
				stc.scope_id = src6->sin6_addr.s6_addr16[1];
#endif
				stc.local_scope = 0;
				stc.site_scope = 1;
				stc.ipv4_scope = 1;
				





				cnt_inits_to = 1;
				
			} else if (IN6_IS_ADDR_SITELOCAL(&src6->sin6_addr)) {
				



				stc.site_scope = 1;
			}
			memcpy(&stc.laddress, &dst6->sin6_addr, sizeof(struct in6_addr));
			stc.laddr_type = SCTP_IPV6_ADDRESS;
			break;
		}
#endif
#if defined(__Userspace__)
		case AF_CONN:
		{
			
			stc.address[0] = 0;
			stc.address[1] = 0;
			stc.address[2] = 0;
			stc.address[3] = 0;
			memcpy(&stc.address, &srcconn->sconn_addr, sizeof(void *));
			stc.addr_type = SCTP_CONN_ADDRESS;
			
			stc.laddress[0] = 0;
			stc.laddress[1] = 0;
			stc.laddress[2] = 0;
			stc.laddress[3] = 0;
			memcpy(&stc.laddress, &dstconn->sconn_addr, sizeof(void *));
			stc.laddr_type = SCTP_CONN_ADDRESS;
			
			break;
		}
#endif
		default:
			
			goto do_a_abort;
			break;
		}
	} else {
		

#ifdef INET6
		struct sctp_nets *lnet;
#endif

		stc.loopback_scope = asoc->scope.loopback_scope;
		stc.ipv4_scope = asoc->scope.ipv4_local_scope;
		stc.site_scope = asoc->scope.site_scope;
		stc.local_scope = asoc->scope.local_scope;
#ifdef INET6
		
		TAILQ_FOREACH(lnet, &asoc->nets, sctp_next) {
			if (lnet->ro._l_addr.sin6.sin6_family == AF_INET6) {
				if (IN6_IS_ADDR_LINKLOCAL(&lnet->ro._l_addr.sin6.sin6_addr)) {
					



					cnt_inits_to = 1;
				}
			}
		}
#endif
		
		to = (struct sockaddr *)&net->ro._l_addr;
		switch (to->sa_family) {
#ifdef INET
		case AF_INET:
			sin = (struct sockaddr_in *)to;
			stc.address[0] = sin->sin_addr.s_addr;
			stc.address[1] = 0;
			stc.address[2] = 0;
			stc.address[3] = 0;
			stc.addr_type = SCTP_IPV4_ADDRESS;
			if (net->src_addr_selected == 0) {
				



				net->ro._s_addr = sctp_source_address_selection(inp,
										stcb, (sctp_route_t *)&net->ro,
										net, 0, vrf_id);
				if (net->ro._s_addr == NULL)
					return;

				net->src_addr_selected = 1;

			}
			stc.laddress[0] = net->ro._s_addr->address.sin.sin_addr.s_addr;
			stc.laddress[1] = 0;
			stc.laddress[2] = 0;
			stc.laddress[3] = 0;
			stc.laddr_type = SCTP_IPV4_ADDRESS;
			
			stc.scope_id = 0;
			break;
#endif
#ifdef INET6
		case AF_INET6:
			sin6 = (struct sockaddr_in6 *)to;
			memcpy(&stc.address, &sin6->sin6_addr,
			       sizeof(struct in6_addr));
			stc.addr_type = SCTP_IPV6_ADDRESS;
			stc.scope_id = sin6->sin6_scope_id;
			if (net->src_addr_selected == 0) {
				



				net->ro._s_addr = sctp_source_address_selection(inp,
										stcb, (sctp_route_t *)&net->ro,
										net, 0, vrf_id);
				if (net->ro._s_addr == NULL)
					return;

				net->src_addr_selected = 1;
			}
			memcpy(&stc.laddress, &net->ro._s_addr->address.sin6.sin6_addr,
			       sizeof(struct in6_addr));
			stc.laddr_type = SCTP_IPV6_ADDRESS;
			break;
#endif
#if defined(__Userspace__)
		case AF_CONN:
			sconn = (struct sockaddr_conn *)to;
			memcpy(&stc.address, &sconn->sconn_addr, sizeof(void *));
			stc.addr_type = SCTP_CONN_ADDRESS;
			memcpy(&stc.laddress, &sconn->sconn_addr, sizeof(void *));
			stc.scope_id = 0;
			stc.laddr_type = SCTP_CONN_ADDRESS;
			break;
#endif
		}
	}
	
	initack = mtod(m, struct sctp_init_ack_chunk *);
	
	stc.peers_vtag = init_chk->init.initiate_tag;
	
	memcpy(stc.identification, SCTP_VERSION_STRING,
	       min(strlen(SCTP_VERSION_STRING), sizeof(stc.identification)));
	memset(stc.reserved, 0, SCTP_RESERVE_SPACE);
	
	initack->ch.chunk_type = SCTP_INITIATION_ACK;
	initack->ch.chunk_flags = 0;
	
	initack->ch.chunk_length = 0;
	
	if ((asoc != NULL) &&
	    ((SCTP_GET_STATE(asoc) == SCTP_STATE_COOKIE_WAIT) ||
	     (SCTP_GET_STATE(asoc) == SCTP_STATE_INUSE) ||
	     (SCTP_GET_STATE(asoc) == SCTP_STATE_COOKIE_ECHOED))) {
		
		initack->init.initiate_tag = htonl(asoc->my_vtag);
		initack->init.initial_tsn = htonl(asoc->init_seq_number);
	} else {
		uint32_t vtag, itsn;
		if (hold_inp_lock) {
			SCTP_INP_INCR_REF(inp);
			SCTP_INP_RUNLOCK(inp);
		}
		if (asoc) {
			atomic_add_int(&asoc->refcnt, 1);
			SCTP_TCB_UNLOCK(stcb);
		new_tag:
			vtag = sctp_select_a_tag(inp, inp->sctp_lport, sh->src_port, 1);
			if ((asoc->peer_supports_nat)  && (vtag == asoc->my_vtag)) {
				


				goto new_tag;
			}
			initack->init.initiate_tag = htonl(vtag);
			
			itsn = sctp_select_initial_TSN(&inp->sctp_ep);
			initack->init.initial_tsn = htonl(itsn);
			SCTP_TCB_LOCK(stcb);
			atomic_add_int(&asoc->refcnt, -1);
		} else {
			vtag = sctp_select_a_tag(inp, inp->sctp_lport, sh->src_port, 1);
			initack->init.initiate_tag = htonl(vtag);
			
			initack->init.initial_tsn = htonl(sctp_select_initial_TSN(&inp->sctp_ep));
		}
		if (hold_inp_lock) {
			SCTP_INP_RLOCK(inp);
			SCTP_INP_DECR_REF(inp);
		}
	}
	
	stc.my_vtag = initack->init.initiate_tag;

	
	so = inp->sctp_socket;
	if (so == NULL) {
		
		sctp_m_freem(m);
		return;
	} else {
		initack->init.a_rwnd = htonl(max(SCTP_SB_LIMIT_RCV(so), SCTP_MINIMAL_RWND));
	}
	
	his_limit = ntohs(init_chk->init.num_inbound_streams);
	
	if (asoc != NULL) {
		if (asoc->streamoutcnt > inp->sctp_ep.pre_open_stream_count) {
			i_want = asoc->streamoutcnt;
		} else {
			i_want = inp->sctp_ep.pre_open_stream_count;
		}
	} else {
		i_want = inp->sctp_ep.pre_open_stream_count;
	}
	if (his_limit < i_want) {
		
		initack->init.num_outbound_streams = init_chk->init.num_inbound_streams;
	} else {
		
		initack->init.num_outbound_streams = htons(i_want);
	}
	
	initack->init.num_inbound_streams =
		htons(inp->sctp_ep.max_open_streams_intome);

	
	if (inp->sctp_ep.adaptation_layer_indicator_provided) {
		ali = (struct sctp_adaptation_layer_indication *)((caddr_t)initack + sizeof(*initack));
		ali->ph.param_type = htons(SCTP_ULP_ADAPTATION);
		ali->ph.param_length = htons(sizeof(*ali));
		ali->indication = ntohl(inp->sctp_ep.adaptation_layer_indicator);
		SCTP_BUF_LEN(m) += sizeof(*ali);
		ecn = (struct sctp_ecn_supported_param *)((caddr_t)ali + sizeof(*ali));
	} else {
		ecn = (struct sctp_ecn_supported_param *)((caddr_t)initack + sizeof(*initack));
	}

	
	if (((asoc != NULL) && (asoc->ecn_allowed == 1)) ||
	    (inp->sctp_ecn_enable == 1)) {
		ecn->ph.param_type = htons(SCTP_ECN_CAPABLE);
		ecn->ph.param_length = htons(sizeof(*ecn));
		SCTP_BUF_LEN(m) += sizeof(*ecn);

		prsctp = (struct sctp_prsctp_supported_param *)((caddr_t)ecn +
								sizeof(*ecn));
	} else {
		prsctp = (struct sctp_prsctp_supported_param *)((caddr_t)ecn);
	}
	
	prsctp->ph.param_type = htons(SCTP_PRSCTP_SUPPORTED);
	prsctp->ph.param_length = htons(sizeof(*prsctp));
	SCTP_BUF_LEN(m) += sizeof(*prsctp);
	if (nat_friendly) {
		
		struct sctp_paramhdr *ph;

		ph = (struct sctp_paramhdr *)(mtod(m, caddr_t) + SCTP_BUF_LEN(m));
		ph->param_type = htons(SCTP_HAS_NAT_SUPPORT);
		ph->param_length = htons(sizeof(struct sctp_paramhdr));
		SCTP_BUF_LEN(m) += sizeof(struct sctp_paramhdr);
	}
	
	pr_supported = (struct sctp_supported_chunk_types_param *)(mtod(m, caddr_t) + SCTP_BUF_LEN(m));
	pr_supported->ph.param_type = htons(SCTP_SUPPORTED_CHUNK_EXT);
	num_ext = 0;
	pr_supported->chunk_types[num_ext++] = SCTP_ASCONF;
	pr_supported->chunk_types[num_ext++] = SCTP_ASCONF_ACK;
	pr_supported->chunk_types[num_ext++] = SCTP_FORWARD_CUM_TSN;
	pr_supported->chunk_types[num_ext++] = SCTP_PACKET_DROPPED;
	pr_supported->chunk_types[num_ext++] = SCTP_STREAM_RESET;
	if (!SCTP_BASE_SYSCTL(sctp_auth_disable))
		pr_supported->chunk_types[num_ext++] = SCTP_AUTHENTICATION;
	if (SCTP_BASE_SYSCTL(sctp_nr_sack_on_off))
		pr_supported->chunk_types[num_ext++] = SCTP_NR_SELECTIVE_ACK;
	p_len = sizeof(*pr_supported) + num_ext;
	pr_supported->ph.param_length = htons(p_len);
	bzero((caddr_t)pr_supported + p_len, SCTP_SIZE32(p_len) - p_len);
	SCTP_BUF_LEN(m) += SCTP_SIZE32(p_len);

	
	if (!SCTP_BASE_SYSCTL(sctp_auth_disable)) {
		struct sctp_auth_random *randp;
		struct sctp_auth_hmac_algo *hmacs;
		struct sctp_auth_chunk_list *chunks;
		uint16_t random_len;

		
		random_len = SCTP_AUTH_RANDOM_SIZE_DEFAULT;
		randp = (struct sctp_auth_random *)(mtod(m, caddr_t) + SCTP_BUF_LEN(m));
		randp->ph.param_type = htons(SCTP_RANDOM);
		p_len = sizeof(*randp) + random_len;
		randp->ph.param_length = htons(p_len);
		SCTP_READ_RANDOM(randp->random_data, random_len);
		
		bzero((caddr_t)randp + p_len, SCTP_SIZE32(p_len) - p_len);
		SCTP_BUF_LEN(m) += SCTP_SIZE32(p_len);

		
		hmacs = (struct sctp_auth_hmac_algo *)(mtod(m, caddr_t) + SCTP_BUF_LEN(m));
		p_len = sctp_serialize_hmaclist(inp->sctp_ep.local_hmacs,
						(uint8_t *) hmacs->hmac_ids);
		if (p_len > 0) {
			p_len += sizeof(*hmacs);
			hmacs->ph.param_type = htons(SCTP_HMAC_LIST);
			hmacs->ph.param_length = htons(p_len);
			
			bzero((caddr_t)hmacs + p_len, SCTP_SIZE32(p_len) - p_len);
			SCTP_BUF_LEN(m) += SCTP_SIZE32(p_len);
		}
		
		chunks = (struct sctp_auth_chunk_list *)(mtod(m, caddr_t) + SCTP_BUF_LEN(m));
		p_len = sctp_serialize_auth_chunks(inp->sctp_ep.local_auth_chunks,
						   chunks->chunk_types);
		if (p_len > 0) {
			p_len += sizeof(*chunks);
			chunks->ph.param_type = htons(SCTP_CHUNK_LIST);
			chunks->ph.param_length = htons(p_len);
			
			bzero((caddr_t)chunks + p_len, SCTP_SIZE32(p_len) - p_len);
			SCTP_BUF_LEN(m) += SCTP_SIZE32(p_len);
		}
	}
	m_at = m;
	
	{
		struct sctp_scoping scp;
		





 		scp.ipv4_addr_legal = stc.ipv4_addr_legal;
		scp.ipv6_addr_legal = stc.ipv6_addr_legal;
#if defined(__Userspace__)
		scp.conn_addr_legal = stc.conn_addr_legal;
#endif
		scp.loopback_scope = stc.loopback_scope;
		scp.ipv4_local_scope = stc.ipv4_scope;
		scp.local_scope = stc.local_scope;
		scp.site_scope = stc.site_scope;
		m_at = sctp_add_addresses_to_i_ia(inp, stcb, &scp, m_at, cnt_inits_to, NULL, NULL);
	}

	
	if (op_err) {
		struct mbuf *ol;
		int llen;
		llen = 0;
		ol = op_err;

		while (ol) {
			llen += SCTP_BUF_LEN(ol);
			ol = SCTP_BUF_NEXT(ol);
		}
		if (llen % 4) {
			
			uint32_t cpthis = 0;
			int padlen;

			padlen = 4 - (llen % 4);
			m_copyback(op_err, llen, padlen, (caddr_t)&cpthis);
		}
		while (SCTP_BUF_NEXT(m_at) != NULL) {
			m_at = SCTP_BUF_NEXT(m_at);
		}
		SCTP_BUF_NEXT(m_at) = op_err;
		while (SCTP_BUF_NEXT(m_at) != NULL) {
			m_at = SCTP_BUF_NEXT(m_at);
		}
	}
	
	p_len = 0;
	for (m_tmp = m; m_tmp; m_tmp = SCTP_BUF_NEXT(m_tmp)) {
		p_len += SCTP_BUF_LEN(m_tmp);
		if (SCTP_BUF_NEXT(m_tmp) == NULL) {
			
			break;
		}
	}

	
	m_cookie = sctp_add_cookie(init_pkt, offset, m, 0, &stc, &signature);
	if (m_cookie == NULL) {
		
		sctp_m_freem(m);
		return;
	}
	
	SCTP_BUF_NEXT(m_tmp) = m_cookie;

	for (m_tmp = m_cookie; m_tmp; m_tmp = SCTP_BUF_NEXT(m_tmp)) {
		p_len += SCTP_BUF_LEN(m_tmp);
		if (SCTP_BUF_NEXT(m_tmp) == NULL) {
			
			mp_last = m_tmp;
			break;
		}
	}
	


	initack->ch.chunk_length = htons(p_len);

	


	(void)sctp_hmac_m(SCTP_HMAC,
			  (uint8_t *)inp->sctp_ep.secret_key[(int)(inp->sctp_ep.current_secret_number)],
			  SCTP_SECRET_SIZE, m_cookie, sizeof(struct sctp_paramhdr),
			  (uint8_t *)signature, SCTP_SIGNATURE_SIZE);
	



	padval = p_len % 4;
	if ((padval) && (mp_last)) {
		
		if (sctp_add_pad_tombuf(mp_last, (4 - padval))) {
			
			sctp_m_freem(m);
			return;
		}
	}
	if (stc.loopback_scope) {
		over_addr = (union sctp_sockstore *)dst;
	} else {
		over_addr = NULL;
	}

	(void)sctp_lowlevel_chunk_output(inp, NULL, NULL, to, m, 0, NULL, 0, 0,
	                                 0, 0,
	                                 inp->sctp_lport, sh->src_port, init_chk->init.initiate_tag,
	                                 port, over_addr,
#if defined(__FreeBSD__)
	                                 use_mflowid, mflowid,
#endif
	                                 SCTP_SO_NOT_LOCKED);
	SCTP_STAT_INCR_COUNTER64(sctps_outcontrolchunks);
}


static void
sctp_prune_prsctp(struct sctp_tcb *stcb,
    struct sctp_association *asoc,
    struct sctp_sndrcvinfo *srcv,
    int dataout)
{
	int freed_spc = 0;
	struct sctp_tmit_chunk *chk, *nchk;

	SCTP_TCB_LOCK_ASSERT(stcb);
	if ((asoc->peer_supports_prsctp) &&
	    (asoc->sent_queue_cnt_removeable > 0)) {
		TAILQ_FOREACH(chk, &asoc->sent_queue, sctp_next) {
			





			if (PR_SCTP_BUF_ENABLED(chk->flags)) {
				



				if (chk->rec.data.timetodrop.tv_sec >= (long)srcv->sinfo_timetolive) {
					






					if (chk->data) {
						



						int ret_spc;
						uint8_t sent;

						if (chk->sent > SCTP_DATAGRAM_UNSENT)
							sent = 1;
						else
							sent = 0;
						ret_spc = sctp_release_pr_sctp_chunk(stcb, chk,
						    sent,
						    SCTP_SO_LOCKED);
						freed_spc += ret_spc;
						if (freed_spc >= dataout) {
							return;
						}
					}	
				}	
			}	
		}		

		TAILQ_FOREACH_SAFE(chk, &asoc->send_queue, sctp_next, nchk) {
			
			if (PR_SCTP_BUF_ENABLED(chk->flags)) {
				if (chk->rec.data.timetodrop.tv_sec >= (long)srcv->sinfo_timetolive) {
					if (chk->data) {
						



						int ret_spc;

						ret_spc = sctp_release_pr_sctp_chunk(stcb, chk,
						    0, SCTP_SO_LOCKED);

						freed_spc += ret_spc;
						if (freed_spc >= dataout) {
							return;
						}
					}	
				}	
			}	
		}		
	}			
}

int
sctp_get_frag_point(struct sctp_tcb *stcb,
    struct sctp_association *asoc)
{
	int siz, ovh;

	




	if (stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_BOUND_V6) {
		ovh = SCTP_MED_OVERHEAD;
	} else {
		ovh = SCTP_MED_V4_OVERHEAD;
	}

	if (stcb->asoc.sctp_frag_point > asoc->smallest_mtu)
		siz = asoc->smallest_mtu - ovh;
	else
		siz = (stcb->asoc.sctp_frag_point - ovh);
	


	
	
	

	
	if (sctp_auth_is_required_chunk(SCTP_DATA, stcb->asoc.peer_auth_chunks))
		siz -= sctp_get_auth_chunk_len(stcb->asoc.peer_hmac_id);

	if (siz % 4) {
		
		siz -= (siz % 4);
	}
	return (siz);
}

static void
sctp_set_prsctp_policy(struct sctp_stream_queue_pending *sp)
{
	sp->pr_sctp_on = 0;
	






	if (PR_SCTP_ENABLED(sp->sinfo_flags)) {
		sp->act_flags |= PR_SCTP_POLICY(sp->sinfo_flags);
		sp->pr_sctp_on = 1;
	} else {
		return;
	}
	switch (PR_SCTP_POLICY(sp->sinfo_flags)) {
	case CHUNK_FLAGS_PR_SCTP_BUF:
		



		sp->ts.tv_sec = sp->timetolive;
		sp->ts.tv_usec = 0;
		break;
	case CHUNK_FLAGS_PR_SCTP_TTL:
	{
		struct timeval tv;
		(void)SCTP_GETTIME_TIMEVAL(&sp->ts);
		tv.tv_sec = sp->timetolive / 1000;
		tv.tv_usec = (sp->timetolive * 1000) % 1000000;
		


#ifndef __FreeBSD__
		timeradd(&sp->ts, &tv, &sp->ts);
#else
		timevaladd(&sp->ts, &tv);
#endif
	}
		break;
	case CHUNK_FLAGS_PR_SCTP_RTX:
		



		sp->ts.tv_sec = sp->timetolive;
		sp->ts.tv_usec = 0;
		break;
	default:
		SCTPDBG(SCTP_DEBUG_USRREQ1,
			"Unknown PR_SCTP policy %u.\n",
			PR_SCTP_POLICY(sp->sinfo_flags));
		break;
	}
}

static int
sctp_msg_append(struct sctp_tcb *stcb,
		struct sctp_nets *net,
		struct mbuf *m,
		struct sctp_sndrcvinfo *srcv, int hold_stcb_lock)
{
	int error = 0;
	struct mbuf *at;
	struct sctp_stream_queue_pending *sp = NULL;
	struct sctp_stream_out *strm;

	



	if (srcv->sinfo_stream >= stcb->asoc.streamoutcnt) {
		
		SCTP_LTRACE_ERR_RET_PKT(m, NULL, stcb, net, SCTP_FROM_SCTP_OUTPUT, EINVAL);
		error = EINVAL;
		goto out_now;
	}
	if ((stcb->asoc.stream_locked) &&
	    (stcb->asoc.stream_locked_on != srcv->sinfo_stream)) {
		SCTP_LTRACE_ERR_RET_PKT(m, NULL, stcb, net, SCTP_FROM_SCTP_OUTPUT, EINVAL);
		error = EINVAL;
		goto out_now;
	}
	strm = &stcb->asoc.strmout[srcv->sinfo_stream];
	
	if ((SCTP_GET_STATE(&stcb->asoc) == SCTP_STATE_SHUTDOWN_SENT) ||
	    (SCTP_GET_STATE(&stcb->asoc) == SCTP_STATE_SHUTDOWN_ACK_SENT) ||
	    (SCTP_GET_STATE(&stcb->asoc) == SCTP_STATE_SHUTDOWN_RECEIVED) ||
	    (stcb->asoc.state & SCTP_STATE_SHUTDOWN_PENDING)) {
		
		SCTP_LTRACE_ERR_RET(NULL, stcb, NULL, SCTP_FROM_SCTP_OUTPUT, ECONNRESET);
		error = ECONNRESET;
		goto out_now;
	}
	sctp_alloc_a_strmoq(stcb, sp);
	if (sp == NULL) {
		SCTP_LTRACE_ERR_RET(NULL, stcb, NULL, SCTP_FROM_SCTP_OUTPUT, ENOMEM);
		error = ENOMEM;
		goto out_now;
	}
	sp->sinfo_flags = srcv->sinfo_flags;
	sp->timetolive = srcv->sinfo_timetolive;
	sp->ppid = srcv->sinfo_ppid;
	sp->context = srcv->sinfo_context;
	if (sp->sinfo_flags & SCTP_ADDR_OVER) {
		sp->net = net;
		atomic_add_int(&sp->net->ref_count, 1);
	} else {
		sp->net = NULL;
	}
	(void)SCTP_GETTIME_TIMEVAL(&sp->ts);
	sp->stream = srcv->sinfo_stream;
	sp->msg_is_complete = 1;
	sp->sender_all_done = 1;
	sp->some_taken = 0;
	sp->data = m;
	sp->tail_mbuf = NULL;
	sctp_set_prsctp_policy(sp);
	



	sp->length = 0;
	for (at = m; at; at = SCTP_BUF_NEXT(at)) {
		if (SCTP_BUF_NEXT(at) == NULL)
			sp->tail_mbuf = at;
		sp->length += SCTP_BUF_LEN(at);
	}
	if (srcv->sinfo_keynumber_valid) {
		sp->auth_keyid = srcv->sinfo_keynumber;
	} else {
		sp->auth_keyid = stcb->asoc.authinfo.active_keyid;
	}
	if (sctp_auth_is_required_chunk(SCTP_DATA, stcb->asoc.peer_auth_chunks)) {
		sctp_auth_key_acquire(stcb, sp->auth_keyid);
		sp->holds_key_ref = 1;
	}
	if (hold_stcb_lock == 0) {
		SCTP_TCB_SEND_LOCK(stcb);
	}
	sctp_snd_sb_alloc(stcb, sp->length);
	atomic_add_int(&stcb->asoc.stream_queue_cnt, 1);
	TAILQ_INSERT_TAIL(&strm->outqueue, sp, next);
	stcb->asoc.ss_functions.sctp_ss_add_to_stream(stcb, &stcb->asoc, strm, sp, 1);
	m = NULL;
	if (hold_stcb_lock == 0) {
		SCTP_TCB_SEND_UNLOCK(stcb);
	}
out_now:
	if (m) {
		sctp_m_freem(m);
	}
	return (error);
}


static struct mbuf *
sctp_copy_mbufchain(struct mbuf *clonechain,
		    struct mbuf *outchain,
		    struct mbuf **endofchain,
		    int can_take_mbuf,
		    int sizeofcpy,
		    uint8_t copy_by_ref)
{
	struct mbuf *m;
	struct mbuf *appendchain;
	caddr_t cp;
	int len;

	if (endofchain == NULL) {
		
	error_out:
		if (outchain)
			sctp_m_freem(outchain);
		return (NULL);
	}
	if (can_take_mbuf) {
		appendchain = clonechain;
	} else {
		if (!copy_by_ref &&
#if defined(__Panda__)
		    0
#else
		    (sizeofcpy <= (int)((((SCTP_BASE_SYSCTL(sctp_mbuf_threshold_count) - 1) * MLEN) + MHLEN)))
#endif
		    ) {
			
			if (*endofchain == NULL) {
				
				if (outchain == NULL) {
					
				new_mbuf:
					outchain = sctp_get_mbuf_for_msg(MCLBYTES, 0, M_NOWAIT, 1, MT_HEADER);
					if (outchain == NULL) {
						goto error_out;
					}
					SCTP_BUF_LEN(outchain) = 0;
					*endofchain = outchain;
					
					SCTP_BUF_RESV_UF(outchain, (SCTP_FIRST_MBUF_RESV+4));
				} else {
					
					
					m = outchain;
					while (m) {
						if (SCTP_BUF_NEXT(m) == NULL) {
							*endofchain = m;
							break;
						}
						m = SCTP_BUF_NEXT(m);
					}
					
					if (*endofchain == NULL) {
						
						sctp_m_freem(outchain);
						goto new_mbuf;
					}
				}
				
				len = M_TRAILINGSPACE(*endofchain);
			} else {
				
				len = M_TRAILINGSPACE(*endofchain);
			}
			
			cp = (mtod((*endofchain), caddr_t) + SCTP_BUF_LEN((*endofchain)));

			
			if (len >= sizeofcpy) {
				
				m_copydata(clonechain, 0, sizeofcpy, cp);
				SCTP_BUF_LEN((*endofchain)) += sizeofcpy;
			} else {
				
				if (len > 0) {
					m_copydata(clonechain, 0, len, cp);
					SCTP_BUF_LEN((*endofchain)) += len;
					
					sizeofcpy -= len;
				}
				m = sctp_get_mbuf_for_msg(MCLBYTES, 0, M_NOWAIT, 1, MT_HEADER);
				if (m == NULL) {
					
					goto error_out;
				}
				SCTP_BUF_NEXT((*endofchain)) = m;
				*endofchain = m;
				cp = mtod((*endofchain), caddr_t);
				m_copydata(clonechain, len, sizeofcpy, cp);
				SCTP_BUF_LEN((*endofchain)) += sizeofcpy;
			}
			return (outchain);
		} else {
			
			appendchain = SCTP_M_COPYM(clonechain, 0, M_COPYALL, M_NOWAIT);
#ifdef SCTP_MBUF_LOGGING
			if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_MBUF_LOGGING_ENABLE) {
				struct mbuf *mat;

				for (mat = appendchain; mat; mat = SCTP_BUF_NEXT(mat)) {
					if (SCTP_BUF_IS_EXTENDED(mat)) {
						sctp_log_mb(mat, SCTP_MBUF_ICOPY);
					}
				}
			}
#endif
		}
	}
	if (appendchain == NULL) {
		
		if (outchain)
			sctp_m_freem(outchain);
		return (NULL);
	}
	if (outchain) {
		
		if (*endofchain != NULL) {
			SCTP_BUF_NEXT(((*endofchain))) = appendchain;
		} else {
			m = outchain;
			while (m) {
				if (SCTP_BUF_NEXT(m) == NULL) {
					SCTP_BUF_NEXT(m) = appendchain;
					break;
				}
				m = SCTP_BUF_NEXT(m);
			}
		}
		



		m = appendchain;
		while (m) {
			if (SCTP_BUF_NEXT(m) == NULL) {
				*endofchain = m;
				break;
			}
			m = SCTP_BUF_NEXT(m);
		}
		return (outchain);
	} else {
		
		m = appendchain;
		while (m) {
			if (SCTP_BUF_NEXT(m) == NULL) {
				*endofchain = m;
				break;
			}
			m = SCTP_BUF_NEXT(m);
		}
		return (appendchain);
	}
}

static int
sctp_med_chunk_output(struct sctp_inpcb *inp,
		      struct sctp_tcb *stcb,
		      struct sctp_association *asoc,
		      int *num_out,
		      int *reason_code,
		      int control_only, int from_where,
		      struct timeval *now, int *now_filled, int frag_point, int so_locked
#if !defined(__APPLE__) && !defined(SCTP_SO_LOCK_TESTING)
		      SCTP_UNUSED
#endif
                      );

static void
sctp_sendall_iterator(struct sctp_inpcb *inp, struct sctp_tcb *stcb, void *ptr,
    uint32_t val SCTP_UNUSED)
{
	struct sctp_copy_all *ca;
	struct mbuf *m;
	int ret = 0;
	int added_control = 0;
	int un_sent, do_chunk_output = 1;
	struct sctp_association *asoc;
	struct sctp_nets *net;

	ca = (struct sctp_copy_all *)ptr;
	if (ca->m == NULL) {
		return;
	}
	if (ca->inp != inp) {
		
		return;
	}
	if ((ca->m) && ca->sndlen) {
		m = SCTP_M_COPYM(ca->m, 0, M_COPYALL, M_NOWAIT);
		if (m == NULL) {
			
			ca->cnt_failed++;
			return;
		}
#ifdef SCTP_MBUF_LOGGING
		if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_MBUF_LOGGING_ENABLE) {
			struct mbuf *mat;

			for (mat = m; mat; mat = SCTP_BUF_NEXT(mat)) {
				if (SCTP_BUF_IS_EXTENDED(mat)) {
					sctp_log_mb(mat, SCTP_MBUF_ICOPY);
				}
			}
		}
#endif
	} else {
		m = NULL;
	}
	SCTP_TCB_LOCK_ASSERT(stcb);
	if (stcb->asoc.alternate) {
		net = stcb->asoc.alternate;
	} else {
		net = stcb->asoc.primary_destination;
	}
	if (ca->sndrcv.sinfo_flags & SCTP_ABORT) {
		
		if (m) {
			struct sctp_paramhdr *ph;

			SCTP_BUF_PREPEND(m, sizeof(struct sctp_paramhdr), M_NOWAIT);
			if (m) {
				ph = mtod(m, struct sctp_paramhdr *);
				ph->param_type = htons(SCTP_CAUSE_USER_INITIATED_ABT);
				ph->param_length = htons(sizeof(struct sctp_paramhdr) + ca->sndlen);
			}
			


			atomic_add_int(&stcb->asoc.refcnt, 1);
			sctp_abort_an_association(inp, stcb, m, SCTP_SO_NOT_LOCKED);
			










			SCTP_TCB_LOCK(stcb);
			atomic_add_int(&stcb->asoc.refcnt, -1);
			goto no_chunk_output;
		}
	} else {
		if (m) {
			ret = sctp_msg_append(stcb, net, m,
					      &ca->sndrcv, 1);
		}
		asoc = &stcb->asoc;
		if (ca->sndrcv.sinfo_flags & SCTP_EOF) {
			
			int cnt;
			cnt = sctp_is_there_unsent_data(stcb, SCTP_SO_NOT_LOCKED);

			if (TAILQ_EMPTY(&asoc->send_queue) &&
			    TAILQ_EMPTY(&asoc->sent_queue) &&
			    (cnt == 0)) {
				if (asoc->locked_on_sending) {
					goto abort_anyway;
				}
				
				if ((SCTP_GET_STATE(asoc) != SCTP_STATE_SHUTDOWN_SENT) &&
				    (SCTP_GET_STATE(asoc) != SCTP_STATE_SHUTDOWN_RECEIVED) &&
				    (SCTP_GET_STATE(asoc) != SCTP_STATE_SHUTDOWN_ACK_SENT)) {
					
					if (SCTP_GET_STATE(asoc) == SCTP_STATE_OPEN) {
						SCTP_STAT_DECR_GAUGE32(sctps_currestab);
					}
					SCTP_SET_STATE(asoc, SCTP_STATE_SHUTDOWN_SENT);
					SCTP_CLEAR_SUBSTATE(asoc, SCTP_STATE_SHUTDOWN_PENDING);
					sctp_stop_timers_for_shutdown(stcb);
					sctp_send_shutdown(stcb, net);
					sctp_timer_start(SCTP_TIMER_TYPE_SHUTDOWN, stcb->sctp_ep, stcb,
							 net);
					sctp_timer_start(SCTP_TIMER_TYPE_SHUTDOWNGUARD, stcb->sctp_ep, stcb,
							 asoc->primary_destination);
					added_control = 1;
					do_chunk_output = 0;
				}
			} else {
				



				





				if ((SCTP_GET_STATE(asoc) != SCTP_STATE_SHUTDOWN_SENT) &&
				    (SCTP_GET_STATE(asoc) != SCTP_STATE_SHUTDOWN_RECEIVED) &&
				    (SCTP_GET_STATE(asoc) != SCTP_STATE_SHUTDOWN_ACK_SENT)) {
					if (asoc->locked_on_sending) {
						
						struct sctp_stream_queue_pending *sp;
						sp = TAILQ_LAST(&asoc->locked_on_sending->outqueue, sctp_streamhead);
						if (sp) {
							if ((sp->length == 0) && (sp->msg_is_complete == 0))
								asoc->state |= SCTP_STATE_PARTIAL_MSG_LEFT;
						}
					}
					asoc->state |= SCTP_STATE_SHUTDOWN_PENDING;
					if (TAILQ_EMPTY(&asoc->send_queue) &&
					    TAILQ_EMPTY(&asoc->sent_queue) &&
					    (asoc->state & SCTP_STATE_PARTIAL_MSG_LEFT)) {
					abort_anyway:
						atomic_add_int(&stcb->asoc.refcnt, 1);
						sctp_abort_an_association(stcb->sctp_ep, stcb,
									  NULL, SCTP_SO_NOT_LOCKED);
						atomic_add_int(&stcb->asoc.refcnt, -1);
						goto no_chunk_output;
					}
					sctp_timer_start(SCTP_TIMER_TYPE_SHUTDOWNGUARD, stcb->sctp_ep, stcb,
							 asoc->primary_destination);
				}
			}

		}
	}
	un_sent = ((stcb->asoc.total_output_queue_size - stcb->asoc.total_flight) +
		   (stcb->asoc.stream_queue_cnt * sizeof(struct sctp_data_chunk)));

	if ((sctp_is_feature_off(inp, SCTP_PCB_FLAGS_NODELAY)) &&
	    (stcb->asoc.total_flight > 0) &&
	    (un_sent < (int)(stcb->asoc.smallest_mtu - SCTP_MIN_OVERHEAD))
	    ) {
		do_chunk_output = 0;
	}
	if (do_chunk_output)
		sctp_chunk_output(inp, stcb, SCTP_OUTPUT_FROM_USR_SEND, SCTP_SO_NOT_LOCKED);
	else if (added_control) {
		int num_out = 0, reason = 0, now_filled = 0;
		struct timeval now;
		int frag_point;
		frag_point = sctp_get_frag_point(stcb, &stcb->asoc);
		(void)sctp_med_chunk_output(inp, stcb, &stcb->asoc, &num_out,
				      &reason, 1, 1, &now, &now_filled, frag_point, SCTP_SO_NOT_LOCKED);
	}
 no_chunk_output:
	if (ret) {
		ca->cnt_failed++;
	} else {
		ca->cnt_sent++;
	}
}

static void
sctp_sendall_completes(void *ptr, uint32_t val SCTP_UNUSED)
{
	struct sctp_copy_all *ca;

	ca = (struct sctp_copy_all *)ptr;
	








	
	sctp_m_freem(ca->m);
	SCTP_FREE(ca, SCTP_M_COPYAL);
}


#define	MC_ALIGN(m, len) do {						\
	SCTP_BUF_RESV_UF(m, ((MCLBYTES - (len)) & ~(sizeof(long) - 1));	\
} while (0)



static struct mbuf *
sctp_copy_out_all(struct uio *uio, int len)
{
	struct mbuf *ret, *at;
	int left, willcpy, cancpy, error;

	ret = sctp_get_mbuf_for_msg(MCLBYTES, 0, M_WAITOK, 1, MT_DATA);
	if (ret == NULL) {
		
		return (NULL);
	}
	left = len;
	SCTP_BUF_LEN(ret) = 0;
	
	cancpy = M_TRAILINGSPACE(ret);
	willcpy = min(cancpy, left);
	at = ret;
	while (left > 0) {
		
		error = uiomove(mtod(at, caddr_t), willcpy, uio);
		if (error) {
	err_out_now:
			sctp_m_freem(at);
			return (NULL);
		}
		SCTP_BUF_LEN(at) = willcpy;
		SCTP_BUF_NEXT_PKT(at) = SCTP_BUF_NEXT(at) = 0;
		left -= willcpy;
		if (left > 0) {
			SCTP_BUF_NEXT(at) = sctp_get_mbuf_for_msg(left, 0, M_WAITOK, 1, MT_DATA);
			if (SCTP_BUF_NEXT(at) == NULL) {
				goto err_out_now;
			}
			at = SCTP_BUF_NEXT(at);
			SCTP_BUF_LEN(at) = 0;
			cancpy = M_TRAILINGSPACE(at);
			willcpy = min(cancpy, left);
		}
	}
	return (ret);
}

static int
sctp_sendall(struct sctp_inpcb *inp, struct uio *uio, struct mbuf *m,
    struct sctp_sndrcvinfo *srcv)
{
	int ret;
	struct sctp_copy_all *ca;

	SCTP_MALLOC(ca, struct sctp_copy_all *, sizeof(struct sctp_copy_all),
		    SCTP_M_COPYAL);
	if (ca == NULL) {
		sctp_m_freem(m);
		SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_OUTPUT, ENOMEM);
		return (ENOMEM);
	}
	memset(ca, 0, sizeof(struct sctp_copy_all));

	ca->inp = inp;
	if (srcv) {
		memcpy(&ca->sndrcv, srcv, sizeof(struct sctp_nonpad_sndrcvinfo));
	}
	



	ca->sndrcv.sinfo_flags &= ~SCTP_SENDALL;
	
	if (uio) {
#if defined(__APPLE__)
#if defined(APPLE_LEOPARD)
		ca->sndlen = uio->uio_resid;
#else
		ca->sndlen = uio_resid(uio);
#endif
#else
		ca->sndlen = uio->uio_resid;
#endif
#if defined(__APPLE__)
		SCTP_SOCKET_UNLOCK(SCTP_INP_SO(inp), 0);
#endif
		ca->m = sctp_copy_out_all(uio, ca->sndlen);
#if defined(__APPLE__)
		SCTP_SOCKET_LOCK(SCTP_INP_SO(inp), 0);
#endif
		if (ca->m == NULL) {
			SCTP_FREE(ca, SCTP_M_COPYAL);
			SCTP_LTRACE_ERR_RET(inp, NULL, NULL, SCTP_FROM_SCTP_OUTPUT, ENOMEM);
			return (ENOMEM);
		}
	} else {
		
		struct mbuf *mat;

		mat = m;
		ca->sndlen = 0;
		while (m) {
			ca->sndlen += SCTP_BUF_LEN(m);
			m = SCTP_BUF_NEXT(m);
		}
		ca->m = mat;
	}
	ret = sctp_initiate_iterator(NULL, sctp_sendall_iterator, NULL,
				     SCTP_PCB_ANY_FLAGS, SCTP_PCB_ANY_FEATURES,
				     SCTP_ASOC_ANY_STATE,
				     (void *)ca, 0,
				     sctp_sendall_completes, inp, 1);
	if (ret) {
		SCTP_PRINTF("Failed to initiate iterator for sendall\n");
		SCTP_FREE(ca, SCTP_M_COPYAL);
		SCTP_LTRACE_ERR_RET_PKT(m, inp, NULL, NULL, SCTP_FROM_SCTP_OUTPUT, EFAULT);
		return (EFAULT);
	}
	return (0);
}


void
sctp_toss_old_cookies(struct sctp_tcb *stcb, struct sctp_association *asoc)
{
	struct sctp_tmit_chunk *chk, *nchk;

	TAILQ_FOREACH_SAFE(chk, &asoc->control_send_queue, sctp_next, nchk) {
		if (chk->rec.chunk_id.id == SCTP_COOKIE_ECHO) {
			TAILQ_REMOVE(&asoc->control_send_queue, chk, sctp_next);
			if (chk->data) {
				sctp_m_freem(chk->data);
				chk->data = NULL;
			}
			asoc->ctrl_queue_cnt--;
			sctp_free_a_chunk(stcb, chk, SCTP_SO_NOT_LOCKED);
		}
	}
}

void
sctp_toss_old_asconf(struct sctp_tcb *stcb)
{
	struct sctp_association *asoc;
	struct sctp_tmit_chunk *chk, *nchk;
	struct sctp_asconf_chunk *acp;

	asoc = &stcb->asoc;
	TAILQ_FOREACH_SAFE(chk, &asoc->asconf_send_queue, sctp_next, nchk) {
		
		if (chk->rec.chunk_id.id == SCTP_ASCONF) {
			if (chk->data) {
				acp = mtod(chk->data, struct sctp_asconf_chunk *);
				if (SCTP_TSN_GT(ntohl(acp->serial_number), asoc->asconf_seq_out_acked)) {
					
					break;
				}
			}
			TAILQ_REMOVE(&asoc->asconf_send_queue, chk, sctp_next);
			if (chk->data) {
				sctp_m_freem(chk->data);
				chk->data = NULL;
			}
			asoc->ctrl_queue_cnt--;
			sctp_free_a_chunk(stcb, chk, SCTP_SO_NOT_LOCKED);
		}
	}
}


static void
sctp_clean_up_datalist(struct sctp_tcb *stcb,
    struct sctp_association *asoc,
    struct sctp_tmit_chunk **data_list,
    int bundle_at,
    struct sctp_nets *net)
{
	int i;
	struct sctp_tmit_chunk *tp1;

	for (i = 0; i < bundle_at; i++) {
		
		TAILQ_REMOVE(&asoc->send_queue, data_list[i], sctp_next);
		asoc->send_queue_cnt--;
		if (i > 0) {
			




			data_list[i]->do_rtt = 0;
		}
		
		data_list[i]->sent_rcv_time = net->last_sent_time;
		data_list[i]->rec.data.cwnd_at_send = net->cwnd;
		data_list[i]->rec.data.fast_retran_tsn = data_list[i]->rec.data.TSN_seq;
		if (data_list[i]->whoTo == NULL) {
			data_list[i]->whoTo = net;
			atomic_add_int(&net->ref_count, 1);
		}
		
		tp1 = TAILQ_LAST(&asoc->sent_queue, sctpchunk_listhead);
		if ((tp1) && SCTP_TSN_GT(tp1->rec.data.TSN_seq, data_list[i]->rec.data.TSN_seq)) {
			struct sctp_tmit_chunk *tpp;

			
		back_up_more:
			tpp = TAILQ_PREV(tp1, sctpchunk_listhead, sctp_next);
			if (tpp == NULL) {
				TAILQ_INSERT_BEFORE(tp1, data_list[i], sctp_next);
				goto all_done;
			}
			tp1 = tpp;
			if (SCTP_TSN_GT(tp1->rec.data.TSN_seq, data_list[i]->rec.data.TSN_seq)) {
				goto back_up_more;
			}
			TAILQ_INSERT_AFTER(&asoc->sent_queue, tp1, data_list[i], sctp_next);
		} else {
			TAILQ_INSERT_TAIL(&asoc->sent_queue,
					  data_list[i],
					  sctp_next);
		}
	all_done:
		
		asoc->sent_queue_cnt++;
		if ((asoc->peers_rwnd <= 0) &&
		    (asoc->total_flight == 0) &&
		    (bundle_at == 1)) {
			
			SCTP_STAT_INCR(sctps_windowprobed);
		}
#ifdef SCTP_AUDITING_ENABLED
		sctp_audit_log(0xC2, 3);
#endif
		data_list[i]->sent = SCTP_DATAGRAM_SENT;
		data_list[i]->snd_count = 1;
		data_list[i]->rec.data.chunk_was_revoked = 0;
		if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_FLIGHT_LOGGING_ENABLE) {
			sctp_misc_ints(SCTP_FLIGHT_LOG_UP,
				       data_list[i]->whoTo->flight_size,
				       data_list[i]->book_size,
				       (uintptr_t)data_list[i]->whoTo,
				       data_list[i]->rec.data.TSN_seq);
		}
		sctp_flight_size_increase(data_list[i]);
		sctp_total_flight_increase(stcb, data_list[i]);
		if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_LOG_RWND_ENABLE) {
			sctp_log_rwnd(SCTP_DECREASE_PEER_RWND,
			      asoc->peers_rwnd, data_list[i]->send_size, SCTP_BASE_SYSCTL(sctp_peer_chunk_oh));
		}
		asoc->peers_rwnd = sctp_sbspace_sub(asoc->peers_rwnd,
						    (uint32_t) (data_list[i]->send_size + SCTP_BASE_SYSCTL(sctp_peer_chunk_oh)));
		if (asoc->peers_rwnd < stcb->sctp_ep->sctp_ep.sctp_sws_sender) {
			
			asoc->peers_rwnd = 0;
		}
	}
	if (asoc->cc_functions.sctp_cwnd_update_packet_transmitted) {
		(*asoc->cc_functions.sctp_cwnd_update_packet_transmitted)(stcb, net);
	}
}

static void
sctp_clean_up_ctl(struct sctp_tcb *stcb, struct sctp_association *asoc, int so_locked
#if !defined(__APPLE__) && !defined(SCTP_SO_LOCK_TESTING)
	SCTP_UNUSED
#endif
)
{
	struct sctp_tmit_chunk *chk, *nchk;

	TAILQ_FOREACH_SAFE(chk, &asoc->control_send_queue, sctp_next, nchk) {
		if ((chk->rec.chunk_id.id == SCTP_SELECTIVE_ACK) ||
		    (chk->rec.chunk_id.id == SCTP_NR_SELECTIVE_ACK) ||	
		    (chk->rec.chunk_id.id == SCTP_HEARTBEAT_REQUEST) ||
		    (chk->rec.chunk_id.id == SCTP_HEARTBEAT_ACK) ||
		    (chk->rec.chunk_id.id == SCTP_FORWARD_CUM_TSN) ||
		    (chk->rec.chunk_id.id == SCTP_SHUTDOWN) ||
		    (chk->rec.chunk_id.id == SCTP_SHUTDOWN_ACK) ||
		    (chk->rec.chunk_id.id == SCTP_OPERATION_ERROR) ||
		    (chk->rec.chunk_id.id == SCTP_PACKET_DROPPED) ||
		    (chk->rec.chunk_id.id == SCTP_COOKIE_ACK) ||
		    (chk->rec.chunk_id.id == SCTP_ECN_CWR) ||
		    (chk->rec.chunk_id.id == SCTP_ASCONF_ACK)) {
			
	clean_up_anyway:
			TAILQ_REMOVE(&asoc->control_send_queue, chk, sctp_next);
			if (chk->data) {
				sctp_m_freem(chk->data);
				chk->data = NULL;
			}
			asoc->ctrl_queue_cnt--;
			if (chk->rec.chunk_id.id == SCTP_FORWARD_CUM_TSN)
				asoc->fwd_tsn_cnt--;
			sctp_free_a_chunk(stcb, chk, so_locked);
		} else if (chk->rec.chunk_id.id == SCTP_STREAM_RESET) {
			
			if (chk != asoc->str_reset) {
				goto clean_up_anyway;
			}
		}
	}
}


static int
sctp_can_we_split_this(struct sctp_tcb *stcb,
					   uint32_t length,
		       uint32_t goal_mtu, uint32_t frag_point, int eeor_on)
{
	



	if (eeor_on) {
		



		if (goal_mtu >= length) {
			




			if (stcb->asoc.total_flight == 0) {
				


				return (length);
			}
			return (0);

		} else {
			
			return (goal_mtu);
		}
	}
	




	if (SCTP_SB_LIMIT_SND(stcb->sctp_socket) < frag_point) {
		return (length);
	}

	if ((length <= goal_mtu) ||
	    ((length - goal_mtu) < SCTP_BASE_SYSCTL(sctp_min_residual))) {
		
		return (0);
	}
	



	if (goal_mtu >= min(SCTP_BASE_SYSCTL(sctp_min_split_point), frag_point)) {
		
		return (min(goal_mtu, frag_point));
	}
	
	return (0);

}

static uint32_t
sctp_move_to_outqueue(struct sctp_tcb *stcb,
	struct sctp_stream_out *strq,
	uint32_t goal_mtu,
	uint32_t frag_point,
	int *locked,
        int *giveup,
	int eeor_mode,
        int *bail,
	int so_locked
#if !defined(__APPLE__) && !defined(SCTP_SO_LOCK_TESTING)
	SCTP_UNUSED
#endif
)
{
	
	struct sctp_association *asoc;
	struct sctp_stream_queue_pending *sp;
	struct sctp_tmit_chunk *chk;
	struct sctp_data_chunk *dchkh;
	uint32_t to_move, length;
	uint8_t rcv_flags = 0;
	uint8_t some_taken;
	uint8_t send_lock_up = 0;

	SCTP_TCB_LOCK_ASSERT(stcb);
	asoc = &stcb->asoc;
 one_more_time:
	
	sp = TAILQ_FIRST(&strq->outqueue);
	if (sp == NULL) {
		*locked = 0;
		if (send_lock_up == 0) {
			SCTP_TCB_SEND_LOCK(stcb);
			send_lock_up = 1;
		}
		sp = TAILQ_FIRST(&strq->outqueue);
		if (sp) {
			goto one_more_time;
		}
		if (strq->last_msg_incomplete) {
			SCTP_PRINTF("Huh? Stream:%d lm_in_c=%d but queue is NULL\n",
			            strq->stream_no,
			            strq->last_msg_incomplete);
			strq->last_msg_incomplete = 0;
		}
		to_move = 0;
		if (send_lock_up) {
			SCTP_TCB_SEND_UNLOCK(stcb);
			send_lock_up = 0;
		}
		goto out_of;
	}
	if ((sp->msg_is_complete) && (sp->length == 0)) {
		if (sp->sender_all_done) {
			



			if ((sp->put_last_out == 0) && (sp->discard_rest == 0)) {
				SCTP_PRINTF("Gak, put out entire msg with NO end!-1\n");
				SCTP_PRINTF("sender_done:%d len:%d msg_comp:%d put_last_out:%d send_lock:%d\n",
				             sp->sender_all_done,
				             sp->length,
				             sp->msg_is_complete,
				             sp->put_last_out,
				             send_lock_up);
			}
			if ((TAILQ_NEXT(sp, next) == NULL) && (send_lock_up  == 0)) {
				SCTP_TCB_SEND_LOCK(stcb);
				send_lock_up = 1;
			}
			atomic_subtract_int(&asoc->stream_queue_cnt, 1);
			TAILQ_REMOVE(&strq->outqueue, sp, next);
			stcb->asoc.ss_functions.sctp_ss_remove_from_stream(stcb, asoc, strq, sp, send_lock_up);
			if (sp->net) {
				sctp_free_remote_addr(sp->net);
				sp->net = NULL;
			}
			if (sp->data) {
				sctp_m_freem(sp->data);
				sp->data = NULL;
			}
			sctp_free_a_strmoq(stcb, sp, so_locked);
			
			*locked = 0;
			stcb->asoc.locked_on_sending = NULL;
			if (send_lock_up) {
				SCTP_TCB_SEND_UNLOCK(stcb);
				send_lock_up = 0;
			}
			
			goto one_more_time;
		} else {
			


			*locked = 1;
			*giveup = 1;
			to_move = 0;
			goto out_of;
		}
	} else {
		
		if (sp->length == 0) {
			
			*locked = 1;
			*giveup = 1;
			to_move = 0;
			goto out_of;
		} else if (sp->discard_rest) {
			if (send_lock_up == 0) {
				SCTP_TCB_SEND_LOCK(stcb);
				send_lock_up = 1;
			}
			
			atomic_subtract_int(&stcb->asoc.total_output_queue_size, sp->length);
			if ((stcb->sctp_socket != NULL) &&	     \
			    ((stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_TCPTYPE) ||
			     (stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_IN_TCPPOOL))) {
				atomic_subtract_int(&stcb->sctp_socket->so_snd.sb_cc, sp->length);
			}
			if (sp->data) {
				sctp_m_freem(sp->data);
				sp->data = NULL;
				sp->tail_mbuf = NULL;
			}
			sp->length = 0;
			sp->some_taken = 1;
			*locked = 1;
			*giveup = 1;
			to_move = 0;
			goto out_of;
		}
	}
	some_taken = sp->some_taken;
	if (stcb->asoc.state & SCTP_STATE_CLOSED_SOCKET) {
		sp->msg_is_complete = 1;
	}
 re_look:
	length = sp->length;
	if (sp->msg_is_complete) {
		
		to_move = min(length, frag_point);
		if (to_move == length) {
			
			if (sp->some_taken) {
				rcv_flags |= SCTP_DATA_LAST_FRAG;
				sp->put_last_out = 1;
			} else {
				rcv_flags |= SCTP_DATA_NOT_FRAG;
				sp->put_last_out = 1;
			}
		} else {
			
			if (sp->some_taken == 0) {
				rcv_flags |= SCTP_DATA_FIRST_FRAG;
			}
			sp->some_taken = 1;
		}
	} else {
		to_move = sctp_can_we_split_this(stcb, length, goal_mtu, frag_point, eeor_mode);
		if (to_move) {
			



			uint32_t llen;

			llen = length;
			if (to_move >= llen) {
				to_move = llen;
				if (send_lock_up == 0) {
					



					SCTP_TCB_SEND_LOCK(stcb);
					send_lock_up = 1;
					if (sp->msg_is_complete) {
						
						goto re_look;
					}
				}
			}
			if (sp->some_taken == 0) {
				rcv_flags |= SCTP_DATA_FIRST_FRAG;
				sp->some_taken = 1;
			}
		} else {
			
			if (sp->some_taken) {
				*locked = 1;
			}
			*giveup = 1;
			to_move = 0;
			goto out_of;
		}
	}

	
	sctp_alloc_a_chunk(stcb, chk);
	if (chk == NULL) {
		
		*giveup = 1;
		to_move = 0;
		goto out_of;
	}
	


	if (sp->sinfo_flags & SCTP_UNORDERED) {
		rcv_flags |= SCTP_DATA_UNORDERED;
	}
	if ((SCTP_BASE_SYSCTL(sctp_enable_sack_immediately) && ((sp->sinfo_flags & SCTP_EOF) == SCTP_EOF)) ||
	    ((sp->sinfo_flags & SCTP_SACK_IMMEDIATELY) == SCTP_SACK_IMMEDIATELY)) {
		rcv_flags |= SCTP_DATA_SACK_IMMEDIATELY;
	}
	
	memset(chk, 0, sizeof(*chk));
	chk->rec.data.rcv_flags = rcv_flags;

	if (to_move >= length) {
		
		if ((sp->sender_all_done == 0) && (send_lock_up == 0)) {
			SCTP_TCB_SEND_LOCK(stcb);
			send_lock_up = 1;
		}
		if (to_move < sp->length) {
			
			goto dont_do_it;
		}
		chk->data = sp->data;
		chk->last_mbuf = sp->tail_mbuf;
		
		sp->data = sp->tail_mbuf = NULL;
	} else {
		struct mbuf *m;
  dont_do_it:
		chk->data = SCTP_M_COPYM(sp->data, 0, to_move, M_NOWAIT);
		chk->last_mbuf = NULL;
		if (chk->data == NULL) {
			sp->some_taken = some_taken;
			sctp_free_a_chunk(stcb, chk, so_locked);
			*bail = 1;
			to_move = 0;
			goto out_of;
		}
#ifdef SCTP_MBUF_LOGGING
		if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_MBUF_LOGGING_ENABLE) {
			struct mbuf *mat;

			for (mat = chk->data; mat; mat = SCTP_BUF_NEXT(mat)) {
				if (SCTP_BUF_IS_EXTENDED(mat)) {
					sctp_log_mb(mat, SCTP_MBUF_ICOPY);
				}
			}
		}
#endif
		
		m_adj(sp->data, to_move);
		
		m = sp->data;
		while (m && (SCTP_BUF_LEN(m) == 0)) {
			sp->data  = SCTP_BUF_NEXT(m);
			SCTP_BUF_NEXT(m) = NULL;
			if (sp->tail_mbuf == m) {
				




#ifdef INVARIANTS
				panic("Huh, freing tail? - TSNH");
#else
				SCTP_PRINTF("Huh, freeing tail? - TSNH\n");
				sp->tail_mbuf = sp->data = NULL;
				sp->length = 0;
#endif

			}
			sctp_m_free(m);
			m = sp->data;
		}
	}
	if (SCTP_BUF_IS_EXTENDED(chk->data)) {
		chk->copy_by_ref = 1;
	} else {
		chk->copy_by_ref = 0;
	}
	


	if (chk->last_mbuf == NULL) {
		chk->last_mbuf = chk->data;
		while (SCTP_BUF_NEXT(chk->last_mbuf) != NULL) {
			chk->last_mbuf = SCTP_BUF_NEXT(chk->last_mbuf);
		}
	}

	if (to_move > length) {
		



#ifdef INVARIANTS
		panic("Huh, how can to_move be larger?");
#else
		SCTP_PRINTF("Huh, how can to_move be larger?\n");
		sp->length = 0;
#endif
	} else {
		atomic_subtract_int(&sp->length, to_move);
	}
	if (M_LEADINGSPACE(chk->data) < (int)sizeof(struct sctp_data_chunk)) {
		
		struct mbuf *m;
		m = sctp_get_mbuf_for_msg(1, 0, M_NOWAIT, 0, MT_DATA);
		if (m == NULL) {
			




			if (send_lock_up == 0) {
				SCTP_TCB_SEND_LOCK(stcb);
				send_lock_up = 1;
			}
			if (chk->data == NULL) {
				
				sp->data = chk->data;
				sp->tail_mbuf = chk->last_mbuf;
			} else {
				struct mbuf *m_tmp;
				
				m_tmp = sp->data;
				sp->data = chk->data;
				SCTP_BUF_NEXT(chk->last_mbuf) = m_tmp;
			}
			sp->some_taken = some_taken;
			atomic_add_int(&sp->length, to_move);
			chk->data = NULL;
			*bail = 1;
			sctp_free_a_chunk(stcb, chk, so_locked);
			to_move = 0;
			goto out_of;
		} else {
			SCTP_BUF_LEN(m) = 0;
			SCTP_BUF_NEXT(m) = chk->data;
			chk->data = m;
			M_ALIGN(chk->data, 4);
		}
	}
	SCTP_BUF_PREPEND(chk->data, sizeof(struct sctp_data_chunk), M_NOWAIT);
	if (chk->data == NULL) {
		
#ifdef INVARIANTS
		panic("prepend failes HELP?");
#else
		SCTP_PRINTF("prepend fails HELP?\n");
		sctp_free_a_chunk(stcb, chk, so_locked);
#endif
		*bail = 1;
		to_move = 0;
		goto out_of;
	}
	sctp_snd_sb_alloc(stcb, sizeof(struct sctp_data_chunk));
	chk->book_size = chk->send_size = (to_move + sizeof(struct sctp_data_chunk));
	chk->book_size_scale = 0;
	chk->sent = SCTP_DATAGRAM_UNSENT;

	chk->flags = 0;
	chk->asoc = &stcb->asoc;
	chk->pad_inplace = 0;
	chk->no_fr_allowed = 0;
	chk->rec.data.stream_seq = strq->next_sequence_send;
	if (rcv_flags & SCTP_DATA_LAST_FRAG) {
		strq->next_sequence_send++;
	}
	chk->rec.data.stream_number = sp->stream;
	chk->rec.data.payloadtype = sp->ppid;
	chk->rec.data.context = sp->context;
	chk->rec.data.doing_fast_retransmit = 0;

	chk->rec.data.timetodrop = sp->ts;
	chk->flags = sp->act_flags;

	if (sp->net) {
		chk->whoTo = sp->net;
		atomic_add_int(&chk->whoTo->ref_count, 1);
	} else
		chk->whoTo = NULL;

	if (sp->holds_key_ref) {
		chk->auth_keyid = sp->auth_keyid;
		sctp_auth_key_acquire(stcb, chk->auth_keyid);
		chk->holds_key_ref = 1;
	}

#if defined(__FreeBSD__) || defined(__Panda__)
	chk->rec.data.TSN_seq = atomic_fetchadd_int(&asoc->sending_seq, 1);
#else
	chk->rec.data.TSN_seq = asoc->sending_seq++;
#endif
	if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_LOG_AT_SEND_2_OUTQ) {
		sctp_misc_ints(SCTP_STRMOUT_LOG_SEND,
		               (uintptr_t)stcb, sp->length,
		               (uint32_t)((chk->rec.data.stream_number << 16) | chk->rec.data.stream_seq),
		               chk->rec.data.TSN_seq);
	}
	dchkh = mtod(chk->data, struct sctp_data_chunk *);
	




#ifdef SCTP_ASOCLOG_OF_TSNS
	SCTP_TCB_LOCK_ASSERT(stcb);
	if (asoc->tsn_out_at >= SCTP_TSN_LOG_SIZE) {
		asoc->tsn_out_at = 0;
		asoc->tsn_out_wrapped = 1;
	}
	asoc->out_tsnlog[asoc->tsn_out_at].tsn = chk->rec.data.TSN_seq;
	asoc->out_tsnlog[asoc->tsn_out_at].strm = chk->rec.data.stream_number;
	asoc->out_tsnlog[asoc->tsn_out_at].seq = chk->rec.data.stream_seq;
	asoc->out_tsnlog[asoc->tsn_out_at].sz = chk->send_size;
	asoc->out_tsnlog[asoc->tsn_out_at].flgs = chk->rec.data.rcv_flags;
	asoc->out_tsnlog[asoc->tsn_out_at].stcb = (void *)stcb;
	asoc->out_tsnlog[asoc->tsn_out_at].in_pos = asoc->tsn_out_at;
	asoc->out_tsnlog[asoc->tsn_out_at].in_out = 2;
	asoc->tsn_out_at++;
#endif

	dchkh->ch.chunk_type = SCTP_DATA;
	dchkh->ch.chunk_flags = chk->rec.data.rcv_flags;
	dchkh->dp.tsn = htonl(chk->rec.data.TSN_seq);
	dchkh->dp.stream_id = htons(strq->stream_no);
	dchkh->dp.stream_sequence = htons(chk->rec.data.stream_seq);
	dchkh->dp.protocol_id = chk->rec.data.payloadtype;
	dchkh->ch.chunk_length = htons(chk->send_size);
	
	if (chk->send_size < SCTP_SIZE32(chk->book_size)) {
		
		struct mbuf *lm;
		int pads;

		pads = SCTP_SIZE32(chk->book_size) - chk->send_size;
		if (sctp_pad_lastmbuf(chk->data, pads, chk->last_mbuf) == 0) {
			chk->pad_inplace = 1;
		}
		if ((lm = SCTP_BUF_NEXT(chk->last_mbuf)) != NULL) {
			
			chk->last_mbuf = lm;
		}
		chk->send_size += pads;
	}
	
	if (sp->pr_sctp_on) {
		sctp_set_prsctp_policy(sp);
		asoc->pr_sctp_cnt++;
		chk->pr_sctp_on = 1;
	} else {
		chk->pr_sctp_on = 0;
	}
	if (sp->msg_is_complete && (sp->length == 0) && (sp->sender_all_done)) {
		
		atomic_subtract_int(&asoc->stream_queue_cnt, 1);
		if (sp->put_last_out == 0) {
			SCTP_PRINTF("Gak, put out entire msg with NO end!-2\n");
			SCTP_PRINTF("sender_done:%d len:%d msg_comp:%d put_last_out:%d send_lock:%d\n",
			            sp->sender_all_done,
			            sp->length,
			            sp->msg_is_complete,
			            sp->put_last_out,
			            send_lock_up);
		}
		if ((send_lock_up == 0) && (TAILQ_NEXT(sp, next) == NULL)) {
			SCTP_TCB_SEND_LOCK(stcb);
			send_lock_up = 1;
		}
		TAILQ_REMOVE(&strq->outqueue, sp, next);
		stcb->asoc.ss_functions.sctp_ss_remove_from_stream(stcb, asoc, strq, sp, send_lock_up);
		if (sp->net) {
			sctp_free_remote_addr(sp->net);
			sp->net = NULL;
		}
		if (sp->data) {
			sctp_m_freem(sp->data);
			sp->data = NULL;
		}
		sctp_free_a_strmoq(stcb, sp, so_locked);

		
		*locked = 0;
		stcb->asoc.locked_on_sending = NULL;
	} else {
		
		*locked = 1;
	}
	asoc->chunks_on_out_queue++;
	strq->chunks_on_queues++;
	TAILQ_INSERT_TAIL(&asoc->send_queue, chk, sctp_next);
	asoc->send_queue_cnt++;
 out_of:
	if (send_lock_up) {
		SCTP_TCB_SEND_UNLOCK(stcb);
	}
	return (to_move);
}


static void
sctp_fill_outqueue(struct sctp_tcb *stcb,
    struct sctp_nets *net, int frag_point, int eeor_mode, int *quit_now, int so_locked
#if !defined(__APPLE__) && !defined(SCTP_SO_LOCK_TESTING)
	SCTP_UNUSED
#endif
)
{
	struct sctp_association *asoc;
	struct sctp_stream_out *strq;
	int goal_mtu, moved_how_much, total_moved = 0, bail = 0;
	int locked, giveup;

	SCTP_TCB_LOCK_ASSERT(stcb);
	asoc = &stcb->asoc;
	switch (net->ro._l_addr.sa.sa_family) {
#ifdef INET
		case AF_INET:
			goal_mtu = net->mtu - SCTP_MIN_V4_OVERHEAD;
			break;
#endif
#ifdef INET6
		case AF_INET6:
			goal_mtu = net->mtu - SCTP_MIN_OVERHEAD;
			break;
#endif
#if defined(__Userspace__)
		case AF_CONN:
			goal_mtu = net->mtu - sizeof(struct sctphdr);
			break;
#endif
		default:
			
			goal_mtu = net->mtu;
			break;
	}
	
	goal_mtu -= sizeof(struct sctp_data_chunk);

	
	goal_mtu &= 0xfffffffc;
	if (asoc->locked_on_sending) {
		
		strq = asoc->locked_on_sending;
		locked = 1;
	} else {
		strq = stcb->asoc.ss_functions.sctp_ss_select_stream(stcb, net, asoc);
		locked = 0;
	}
	while ((goal_mtu > 0) && strq) {
		giveup = 0;
		bail = 0;
		moved_how_much = sctp_move_to_outqueue(stcb, strq, goal_mtu, frag_point, &locked,
						       &giveup, eeor_mode, &bail, so_locked);
		if (moved_how_much)
			stcb->asoc.ss_functions.sctp_ss_scheduled(stcb, net, asoc, strq, moved_how_much);

		if (locked) {
			asoc->locked_on_sending = strq;
			if ((moved_how_much == 0) || (giveup) || bail)
				
				break;
		} else {
			asoc->locked_on_sending = NULL;
			if ((giveup) || bail) {
				break;
			}
			strq = stcb->asoc.ss_functions.sctp_ss_select_stream(stcb, net, asoc);
			if (strq == NULL) {
				break;
			}
		}
		total_moved += moved_how_much;
		goal_mtu -= (moved_how_much + sizeof(struct sctp_data_chunk));
		goal_mtu &= 0xfffffffc;
	}
	if (bail)
		*quit_now = 1;

	stcb->asoc.ss_functions.sctp_ss_packet_done(stcb, net, asoc);

	if (total_moved == 0) {
		if ((stcb->asoc.sctp_cmt_on_off == 0) &&
		    (net == stcb->asoc.primary_destination)) {
			
			SCTP_STAT_INCR(sctps_primary_randry);
		} else if (stcb->asoc.sctp_cmt_on_off > 0) {
			
			SCTP_STAT_INCR(sctps_cmt_randry);
		}
	}
}

void
sctp_fix_ecn_echo(struct sctp_association *asoc)
{
	struct sctp_tmit_chunk *chk;

	TAILQ_FOREACH(chk, &asoc->control_send_queue, sctp_next) {
		if (chk->rec.chunk_id.id == SCTP_ECN_ECHO) {
			chk->sent = SCTP_DATAGRAM_UNSENT;
		}
	}
}

void
sctp_move_chunks_from_net(struct sctp_tcb *stcb, struct sctp_nets *net)
{
	struct sctp_association *asoc;
	struct sctp_tmit_chunk *chk;
	struct sctp_stream_queue_pending *sp;
	unsigned int i;

	if (net == NULL) {
		return;
	}
	asoc = &stcb->asoc;
	for (i = 0; i < stcb->asoc.streamoutcnt; i++) {
		TAILQ_FOREACH(sp, &stcb->asoc.strmout[i].outqueue, next) {
			if (sp->net == net) {
				sctp_free_remote_addr(sp->net);
				sp->net = NULL;
			}
		}
	}
	TAILQ_FOREACH(chk, &asoc->send_queue, sctp_next) {
		if (chk->whoTo == net) {
			sctp_free_remote_addr(chk->whoTo);
			chk->whoTo = NULL;
		}
	}
}

int
sctp_med_chunk_output(struct sctp_inpcb *inp,
		      struct sctp_tcb *stcb,
		      struct sctp_association *asoc,
		      int *num_out,
		      int *reason_code,
		      int control_only, int from_where,
		      struct timeval *now, int *now_filled, int frag_point, int so_locked
#if !defined(__APPLE__) && !defined(SCTP_SO_LOCK_TESTING)
		      SCTP_UNUSED
#endif
	)
{
	








	struct sctp_nets *net, *start_at, *sack_goes_to = NULL, *old_start_at = NULL;
	struct mbuf *outchain, *endoutchain;
	struct sctp_tmit_chunk *chk, *nchk;

	
	struct sctp_tmit_chunk *data_list[SCTP_MAX_DATA_BUNDLING];
	int no_fragmentflg, error;
	unsigned int max_rwnd_per_dest, max_send_per_dest;
	int one_chunk, hbflag, skip_data_for_this_net;
	int asconf, cookie, no_out_cnt;
	int bundle_at, ctl_cnt, no_data_chunks, eeor_mode;
	unsigned int mtu, r_mtu, omtu, mx_mtu, to_out;
	int tsns_sent = 0;
	uint32_t auth_offset = 0;
	struct sctp_auth_chunk *auth = NULL;
	uint16_t auth_keyid;
	int override_ok = 1;
	int skip_fill_up = 0;
	int data_auth_reqd = 0;
	

	int quit_now = 0;

#if defined(__APPLE__)
	if (so_locked) {
		sctp_lock_assert(SCTP_INP_SO(inp));
	} else {
		sctp_unlock_assert(SCTP_INP_SO(inp));
	}
#endif
	*num_out = 0;
	auth_keyid = stcb->asoc.authinfo.active_keyid;

	if ((asoc->state & SCTP_STATE_SHUTDOWN_PENDING) ||
	    (asoc->state & SCTP_STATE_SHUTDOWN_RECEIVED) ||
	    (sctp_is_feature_on(inp, SCTP_PCB_FLAGS_EXPLICIT_EOR))) {
		eeor_mode = 1;
	} else {
		eeor_mode = 0;
	}
	ctl_cnt = no_out_cnt = asconf = cookie = 0;
	




#ifdef SCTP_AUDITING_ENABLED
	sctp_audit_log(0xC2, 2);
#endif
	SCTP_TCB_LOCK_ASSERT(stcb);
	hbflag = 0;
	if ((control_only) || (asoc->stream_reset_outstanding))
		no_data_chunks = 1;
	else
		no_data_chunks = 0;

	
	if ((TAILQ_EMPTY(&asoc->control_send_queue) ||
	     (asoc->ctrl_queue_cnt == stcb->asoc.ecn_echo_cnt_onq)) &&
	    TAILQ_EMPTY(&asoc->asconf_send_queue) &&
	    TAILQ_EMPTY(&asoc->send_queue) &&
	    stcb->asoc.ss_functions.sctp_ss_is_empty(stcb, asoc)) {
	nothing_to_send:
		*reason_code = 9;
		return (0);
	}
	if (asoc->peers_rwnd == 0) {
		
		*reason_code = 1;
		if (asoc->total_flight > 0) {
			
			no_data_chunks = 1;
		}
	}
	if (stcb->asoc.ecn_echo_cnt_onq) {
		
		if (no_data_chunks &&
		    (asoc->ctrl_queue_cnt == stcb->asoc.ecn_echo_cnt_onq)) {
			
			goto nothing_to_send;
		}
		TAILQ_FOREACH(chk, &asoc->control_send_queue, sctp_next) {
			if ((chk->rec.chunk_id.id == SCTP_SELECTIVE_ACK) ||
			    (chk->rec.chunk_id.id == SCTP_NR_SELECTIVE_ACK)) {
				sack_goes_to = chk->whoTo;
				break;
			}
		}
	}
	max_rwnd_per_dest = ((asoc->peers_rwnd + asoc->total_flight) / asoc->numnets);
	if (stcb->sctp_socket)
		max_send_per_dest = SCTP_SB_LIMIT_SND(stcb->sctp_socket) / asoc->numnets;
	else
		max_send_per_dest = 0;
	if (no_data_chunks == 0) {
		
		TAILQ_FOREACH(chk, &asoc->send_queue, sctp_next) {
			if (chk->whoTo == NULL) {
				



				skip_fill_up = 1;
				break;
			}
		}

	}
	if ((no_data_chunks == 0) &&
	    (skip_fill_up == 0) &&
	    (!stcb->asoc.ss_functions.sctp_ss_is_empty(stcb, asoc))) {
		TAILQ_FOREACH(net, &asoc->nets, sctp_next) {
			










			net->window_probe = 0;
			if ((net != stcb->asoc.alternate) &&
			    ((net->dest_state & SCTP_ADDR_PF) ||
			     (!(net->dest_state & SCTP_ADDR_REACHABLE)) ||
			     (net->dest_state & SCTP_ADDR_UNCONFIRMED))) {
				if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_CWND_LOGGING_ENABLE) {
					sctp_log_cwnd(stcb, net, 1,
						      SCTP_CWND_LOG_FILL_OUTQ_CALLED);
				}
			        continue;
			}
			if ((stcb->asoc.cc_functions.sctp_cwnd_new_transmission_begins) &&
			    (net->flight_size == 0)) {
				(*stcb->asoc.cc_functions.sctp_cwnd_new_transmission_begins)(stcb, net);
			}
			if (net->flight_size >= net->cwnd) {
				
				if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_CWND_LOGGING_ENABLE) {
					sctp_log_cwnd(stcb, net, 3,
						      SCTP_CWND_LOG_FILL_OUTQ_CALLED);
				}
				continue;
			}
			if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_CWND_LOGGING_ENABLE) {
				sctp_log_cwnd(stcb, net, 4, SCTP_CWND_LOG_FILL_OUTQ_CALLED);
			}
			sctp_fill_outqueue(stcb, net, frag_point, eeor_mode, &quit_now, so_locked);
			if (quit_now) {
				
				no_data_chunks = 1;
				break;
			}
		}
	}
	
	
	if (TAILQ_EMPTY(&asoc->control_send_queue) &&
	    TAILQ_EMPTY(&asoc->asconf_send_queue) &&
	    TAILQ_EMPTY(&asoc->send_queue)) {
		*reason_code = 8;
		return (0);
	}

	if (asoc->sctp_cmt_on_off > 0) {
		
		start_at = asoc->last_net_cmt_send_started;
		if (start_at == NULL) {
			
			start_at = TAILQ_FIRST(&asoc->nets);
		} else {
			start_at = TAILQ_NEXT(asoc->last_net_cmt_send_started, sctp_next);
			if (start_at == NULL) {
				start_at = TAILQ_FIRST(&asoc->nets);
			}
		}
		asoc->last_net_cmt_send_started = start_at;
	} else {
		start_at = TAILQ_FIRST(&asoc->nets);
	}
	TAILQ_FOREACH(chk, &asoc->control_send_queue, sctp_next) {
		if (chk->whoTo == NULL) {
			if (asoc->alternate) {
				chk->whoTo = asoc->alternate;
			} else {
				chk->whoTo = asoc->primary_destination;
			}
			atomic_add_int(&chk->whoTo->ref_count, 1);
		}
	}
	old_start_at = NULL;
again_one_more_time:
	for (net = start_at ; net != NULL; net = TAILQ_NEXT(net, sctp_next)) {
		
		
		if (old_start_at && (old_start_at == net)) {
			
			break;
		}
		tsns_sent = 0xa;
		if (TAILQ_EMPTY(&asoc->control_send_queue) &&
		    TAILQ_EMPTY(&asoc->asconf_send_queue) &&
		    (net->flight_size >= net->cwnd)) {
			


			continue;
		}
		bundle_at = 0;
		endoutchain = outchain = NULL;
		no_fragmentflg = 1;
		one_chunk = 0;
		if (net->dest_state & SCTP_ADDR_UNCONFIRMED) {
			skip_data_for_this_net = 1;
		} else {
			skip_data_for_this_net = 0;
		}
#if !(defined(__Panda__) || defined(__Windows__) || defined(__Userspace__) || defined(__APPLE__))
		if ((net->ro.ro_rt) && (net->ro.ro_rt->rt_ifp)) {
			



			struct ifnet *ifp;

			ifp = net->ro.ro_rt->rt_ifp;
			if ((ifp->if_snd.ifq_len + 2) >= ifp->if_snd.ifq_maxlen) {
				SCTP_STAT_INCR(sctps_ifnomemqueued);
				if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_LOG_MAXBURST_ENABLE) {
					sctp_log_maxburst(stcb, net, ifp->if_snd.ifq_len, ifp->if_snd.ifq_maxlen, SCTP_MAX_IFP_APPLIED);
				}
				continue;
			}
		}
#endif
		switch (((struct sockaddr *)&net->ro._l_addr)->sa_family) {
#ifdef INET
		case AF_INET:
			mtu = net->mtu - (sizeof(struct ip) + sizeof(struct sctphdr));
			break;
#endif
#ifdef INET6
		case AF_INET6:
			mtu = net->mtu - (sizeof(struct ip6_hdr) + sizeof(struct sctphdr));
			break;
#endif
#if defined(__Userspace__)
		case AF_CONN:
			mtu = net->mtu - sizeof(struct sctphdr);
			break;
#endif
		default:
			
			mtu = net->mtu;
			break;
		}
		mx_mtu = mtu;
		to_out = 0;
		if (mtu > asoc->peers_rwnd) {
			if (asoc->total_flight > 0) {
				
				r_mtu = asoc->peers_rwnd;
			} else {
				
				one_chunk = 1;
				r_mtu = mtu;
			}
		} else {
			r_mtu = mtu;
		}
		
		
		
		
		TAILQ_FOREACH_SAFE(chk, &asoc->asconf_send_queue, sctp_next, nchk) {
			if (chk->rec.chunk_id.id != SCTP_ASCONF) {
				continue;
			}
			if (chk->whoTo == NULL) {
				if (asoc->alternate == NULL) {
					if (asoc->primary_destination != net) {
						break;
					}
				} else {
					if (asoc->alternate != net) {
						break;
					}
				}
			} else {
				if (chk->whoTo != net) {
					break;
				}
			}
			if (chk->data == NULL) {
				break;
			}
			if (chk->sent != SCTP_DATAGRAM_UNSENT &&
			    chk->sent != SCTP_DATAGRAM_RESEND) {
				break;
			}
			







			if ((auth == NULL) &&
			    sctp_auth_is_required_chunk(chk->rec.chunk_id.id,
							stcb->asoc.peer_auth_chunks)) {
				omtu = sctp_get_auth_chunk_len(stcb->asoc.peer_hmac_id);
			} else
				omtu = 0;
			
			if ((chk->send_size < (int)(mtu - omtu)) ||
			    (chk->flags & CHUNK_FLAGS_FRAGMENT_OK)) {
				









				



				if ((auth == NULL) &&
				    (sctp_auth_is_required_chunk(chk->rec.chunk_id.id,
								 stcb->asoc.peer_auth_chunks))) {
					outchain = sctp_add_auth_chunk(outchain,
								       &endoutchain,
								       &auth,
								       &auth_offset,
								       stcb,
								       chk->rec.chunk_id.id);
					SCTP_STAT_INCR_COUNTER64(sctps_outcontrolchunks);
				}
				outchain = sctp_copy_mbufchain(chk->data, outchain, &endoutchain,
							       (int)chk->rec.chunk_id.can_take_data,
							       chk->send_size, chk->copy_by_ref);
				if (outchain == NULL) {
					*reason_code = 8;
					SCTP_LTRACE_ERR_RET(inp, stcb, NULL, SCTP_FROM_SCTP_OUTPUT, ENOMEM);
					return (ENOMEM);
				}
				SCTP_STAT_INCR_COUNTER64(sctps_outcontrolchunks);
				
				if (mtu > (chk->send_size + omtu))
					mtu -= (chk->send_size + omtu);
				else
					mtu = 0;
				to_out += (chk->send_size + omtu);
				
				if (chk->flags & CHUNK_FLAGS_FRAGMENT_OK) {
					no_fragmentflg = 0;
				}
				if (chk->rec.chunk_id.can_take_data)
					chk->data = NULL;
				



				hbflag = 1;
				asconf = 1;
				




				no_data_chunks = 1;
				chk->sent = SCTP_DATAGRAM_SENT;
				if (chk->whoTo == NULL) {
					chk->whoTo = net;
					atomic_add_int(&net->ref_count, 1);
				}
				chk->snd_count++;
				if (mtu == 0) {
					





					sctp_timer_start(SCTP_TIMER_TYPE_ASCONF, inp, stcb, net);
					





					if ((error = sctp_lowlevel_chunk_output(inp, stcb, net,
					                                        (struct sockaddr *)&net->ro._l_addr,
					                                        outchain, auth_offset, auth,
					                                        stcb->asoc.authinfo.active_keyid,
					                                        no_fragmentflg, 0, asconf,
					                                        inp->sctp_lport, stcb->rport,
					                                        htonl(stcb->asoc.peer_vtag),
					                                        net->port, NULL,
#if defined(__FreeBSD__)
					                                        0, 0,
#endif
					                                        so_locked))) {
						if (error == ENOBUFS) {
							asoc->ifp_had_enobuf = 1;
							SCTP_STAT_INCR(sctps_lowlevelerr);
						}
						if (from_where == 0) {
							SCTP_STAT_INCR(sctps_lowlevelerrusr);
						}
						if (*now_filled == 0) {
							(void)SCTP_GETTIME_TIMEVAL(&net->last_sent_time);
							*now_filled = 1;
							*now = net->last_sent_time;
						} else {
							net->last_sent_time = *now;
						}
						hbflag = 0;
						
						if (error == EHOSTUNREACH) {
							




							sctp_move_chunks_from_net(stcb, net);
						}
						*reason_code = 7;
						continue;
					} else
						asoc->ifp_had_enobuf = 0;
					if (*now_filled == 0) {
						(void)SCTP_GETTIME_TIMEVAL(&net->last_sent_time);
						*now_filled = 1;
						*now = net->last_sent_time;
					} else {
						net->last_sent_time = *now;
					}
					hbflag = 0;
					




					outchain = endoutchain = NULL;
					auth = NULL;
					auth_offset = 0;
					if (!no_out_cnt)
						*num_out += ctl_cnt;
					
					switch (net->ro._l_addr.sa.sa_family) {
#ifdef INET
						case AF_INET:
							mtu = net->mtu - SCTP_MIN_V4_OVERHEAD;
							break;
#endif
#ifdef INET6
						case AF_INET6:
							mtu = net->mtu - SCTP_MIN_OVERHEAD;
							break;
#endif
#if defined(__Userspace__)
						case AF_CONN:
							mtu = net->mtu - sizeof(struct sctphdr);
							break;
#endif
						default:
							
							mtu = net->mtu;
							break;
					}
					to_out = 0;
					no_fragmentflg = 1;
				}
			}
		}
		
		
		
		
		TAILQ_FOREACH_SAFE(chk, &asoc->control_send_queue, sctp_next, nchk) {
			if ((sack_goes_to) &&
			    (chk->rec.chunk_id.id == SCTP_ECN_ECHO) &&
			    (chk->whoTo != sack_goes_to)) {
				



				if (chk->whoTo == net) {
					
					continue;
				} else if (sack_goes_to == net) {
					
					goto skip_net_check;
				}
			}
			if (chk->whoTo == NULL) {
				if (asoc->alternate == NULL) {
					if (asoc->primary_destination != net) {
						continue;
					}
				} else {
					if (asoc->alternate != net) {
						continue;
					}
				}
			} else {
				if (chk->whoTo != net) {
					continue;
				}
			}
		skip_net_check:
			if (chk->data == NULL) {
				continue;
			}
			if (chk->sent != SCTP_DATAGRAM_UNSENT) {
				




				continue;
			}
			







			if ((auth == NULL) &&
			    sctp_auth_is_required_chunk(chk->rec.chunk_id.id,
							stcb->asoc.peer_auth_chunks)) {
				omtu = sctp_get_auth_chunk_len(stcb->asoc.peer_hmac_id);
			} else
				omtu = 0;
			
			if ((chk->send_size <= (int)(mtu - omtu)) ||
			    (chk->flags & CHUNK_FLAGS_FRAGMENT_OK)) {
				









				



				if ((auth == NULL) &&
				    (sctp_auth_is_required_chunk(chk->rec.chunk_id.id,
								 stcb->asoc.peer_auth_chunks))) {
					outchain = sctp_add_auth_chunk(outchain,
								       &endoutchain,
								       &auth,
								       &auth_offset,
								       stcb,
								       chk->rec.chunk_id.id);
					SCTP_STAT_INCR_COUNTER64(sctps_outcontrolchunks);
				}
				outchain = sctp_copy_mbufchain(chk->data, outchain, &endoutchain,
							       (int)chk->rec.chunk_id.can_take_data,
							       chk->send_size, chk->copy_by_ref);
				if (outchain == NULL) {
					*reason_code = 8;
					SCTP_LTRACE_ERR_RET(inp, stcb, NULL, SCTP_FROM_SCTP_OUTPUT, ENOMEM);
					return (ENOMEM);
				}
				SCTP_STAT_INCR_COUNTER64(sctps_outcontrolchunks);
				
				if (mtu > (chk->send_size + omtu))
					mtu -= (chk->send_size + omtu);
				else
					mtu = 0;
				to_out += (chk->send_size + omtu);
				
				if (chk->flags & CHUNK_FLAGS_FRAGMENT_OK) {
					no_fragmentflg = 0;
				}
				if (chk->rec.chunk_id.can_take_data)
					chk->data = NULL;
				
				if ((chk->rec.chunk_id.id == SCTP_SELECTIVE_ACK) ||
				    (chk->rec.chunk_id.id == SCTP_NR_SELECTIVE_ACK) || 
				    (chk->rec.chunk_id.id == SCTP_HEARTBEAT_REQUEST) ||
				    (chk->rec.chunk_id.id == SCTP_HEARTBEAT_ACK) ||
				    (chk->rec.chunk_id.id == SCTP_SHUTDOWN) ||
				    (chk->rec.chunk_id.id == SCTP_SHUTDOWN_ACK) ||
				    (chk->rec.chunk_id.id == SCTP_OPERATION_ERROR) ||
				    (chk->rec.chunk_id.id == SCTP_COOKIE_ACK) ||
				    (chk->rec.chunk_id.id == SCTP_ECN_CWR) ||
				    (chk->rec.chunk_id.id == SCTP_PACKET_DROPPED) ||
				    (chk->rec.chunk_id.id == SCTP_ASCONF_ACK)) {
					if (chk->rec.chunk_id.id == SCTP_HEARTBEAT_REQUEST) {
						hbflag = 1;
					}
					
					if ((chk->rec.chunk_id.id == SCTP_SELECTIVE_ACK) ||
					    (chk->rec.chunk_id.id == SCTP_NR_SELECTIVE_ACK)) {
						
						if (SCTP_OS_TIMER_PENDING(&stcb->asoc.dack_timer.timer)) {
							sctp_timer_stop(SCTP_TIMER_TYPE_RECV,
									inp, stcb, net, SCTP_FROM_SCTP_OUTPUT+SCTP_LOC_1);
						}
					}
					ctl_cnt++;
				} else {
					





					ctl_cnt++;
					if (chk->rec.chunk_id.id == SCTP_COOKIE_ECHO) {
						cookie = 1;
						no_out_cnt = 1;
					} else if (chk->rec.chunk_id.id == SCTP_ECN_ECHO) {
						







						SCTP_STAT_INCR(sctps_sendecne);
					}
					chk->sent = SCTP_DATAGRAM_SENT;
					if (chk->whoTo == NULL) {
						chk->whoTo = net;
						atomic_add_int(&net->ref_count, 1);
					}
					chk->snd_count++;
				}
				if (mtu == 0) {
					





					if (asconf) {
						sctp_timer_start(SCTP_TIMER_TYPE_ASCONF, inp, stcb, net);
						





					}
					if (cookie) {
						sctp_timer_start(SCTP_TIMER_TYPE_COOKIE, inp, stcb, net);
						cookie = 0;
					}
					if ((error = sctp_lowlevel_chunk_output(inp, stcb, net,
					                                        (struct sockaddr *)&net->ro._l_addr,
					                                        outchain,
					                                        auth_offset, auth,
					                                        stcb->asoc.authinfo.active_keyid,
					                                        no_fragmentflg, 0, asconf,
					                                        inp->sctp_lport, stcb->rport,
					                                        htonl(stcb->asoc.peer_vtag),
					                                        net->port, NULL,
#if defined(__FreeBSD__)
					                                        0, 0,
#endif
					                                        so_locked))) {
						if (error == ENOBUFS) {
							asoc->ifp_had_enobuf = 1;
							SCTP_STAT_INCR(sctps_lowlevelerr);
						}
						if (from_where == 0) {
							SCTP_STAT_INCR(sctps_lowlevelerrusr);
						}
						
						if (hbflag) {
							if (*now_filled == 0) {
								(void)SCTP_GETTIME_TIMEVAL(&net->last_sent_time);
								*now_filled = 1;
								*now = net->last_sent_time;
							} else {
								net->last_sent_time = *now;
							}
							hbflag = 0;
						}
						if (error == EHOSTUNREACH) {
							




							sctp_move_chunks_from_net(stcb, net);
						}
						*reason_code = 7;
						continue;
					} else
						asoc->ifp_had_enobuf = 0;
					
					if (hbflag) {
						if (*now_filled == 0) {
							(void)SCTP_GETTIME_TIMEVAL(&net->last_sent_time);
							*now_filled = 1;
							*now = net->last_sent_time;
						} else {
							net->last_sent_time = *now;
						}
						hbflag = 0;
					}
					




					outchain = endoutchain = NULL;
					auth = NULL;
					auth_offset = 0;
					if (!no_out_cnt)
						*num_out += ctl_cnt;
					
					switch (net->ro._l_addr.sa.sa_family) {
#ifdef INET
						case AF_INET:
							mtu = net->mtu - SCTP_MIN_V4_OVERHEAD;
							break;
#endif
#ifdef INET6
						case AF_INET6:
							mtu = net->mtu - SCTP_MIN_OVERHEAD;
							break;
#endif
#if defined(__Userspace__)
						case AF_CONN:
							mtu = net->mtu - sizeof(struct sctphdr);
							break;
#endif
						default:
							
							mtu = net->mtu;
							break;
					}
					to_out = 0;
					no_fragmentflg = 1;
				}
			}
		}
		
		if ((asoc->sctp_cmt_on_off > 0) &&
		    (net != stcb->asoc.alternate) &&
		    (net->dest_state & SCTP_ADDR_PF)) {
			goto no_data_fill;
		}
		if (net->flight_size >= net->cwnd) {
			goto no_data_fill;
		}
		if ((asoc->sctp_cmt_on_off > 0) &&
		    (SCTP_BASE_SYSCTL(sctp_buffer_splitting) & SCTP_RECV_BUFFER_SPLITTING) &&
		    (net->flight_size > max_rwnd_per_dest)) {
			goto no_data_fill;
		}
		





		if ((asoc->sctp_cmt_on_off > 0) &&
		    (SCTP_BASE_SYSCTL(sctp_buffer_splitting) & SCTP_SEND_BUFFER_SPLITTING) &&
		    (max_send_per_dest > 0) &&
		    (net->flight_size > max_send_per_dest)) {
			goto no_data_fill;
		}
		
		
		
		





		data_auth_reqd = sctp_auth_is_required_chunk(SCTP_DATA,
							     stcb->asoc.peer_auth_chunks);
		if (data_auth_reqd && (auth == NULL)) {
			mtu -= sctp_get_auth_chunk_len(stcb->asoc.peer_hmac_id);
		}
		
		switch (((struct sockaddr *)&net->ro._l_addr)->sa_family) {
#ifdef INET
		case AF_INET:
			if (net->mtu > (sizeof(struct ip) + sizeof(struct sctphdr)))
				omtu = net->mtu - (sizeof(struct ip) + sizeof(struct sctphdr));
			else
				omtu = 0;
			break;
#endif
#ifdef INET6
		case AF_INET6:
			if (net->mtu > (sizeof(struct ip6_hdr) + sizeof(struct sctphdr)))
				omtu = net->mtu - (sizeof(struct ip6_hdr) + sizeof(struct sctphdr));
			else
				omtu = 0;
			break;
#endif
#if defined(__Userspace__)
		case AF_CONN:
			if (net->mtu > sizeof(struct sctphdr)) {
				omtu = net->mtu - sizeof(struct sctphdr);
			} else {
				omtu = 0;
			}
			break;
#endif
		default:
			
			omtu = 0;
			break;
		}
		if ((((asoc->state & SCTP_STATE_OPEN) == SCTP_STATE_OPEN) &&
		     (skip_data_for_this_net == 0)) ||
		    (cookie)) {
			TAILQ_FOREACH_SAFE(chk, &asoc->send_queue, sctp_next, nchk) {
				if (no_data_chunks) {
					
					*reason_code = 1;
					break;
				}
				if (net->flight_size >= net->cwnd) {
					
					*reason_code = 2;
					break;
				}
				if ((chk->whoTo != NULL) &&
				    (chk->whoTo != net)) {
					
					continue;
				}

				if (asoc->sctp_cmt_on_off == 0) {
					if ((asoc->alternate) &&
					    (asoc->alternate != net) &&
					    (chk->whoTo == NULL)) {
						continue;
					} else if ((net != asoc->primary_destination) &&
						   (asoc->alternate == NULL) &&
						   (chk->whoTo == NULL)) {
						continue;
					}
				}
				if ((chk->send_size > omtu) && ((chk->flags & CHUNK_FLAGS_FRAGMENT_OK) == 0)) {
					











					SCTP_PRINTF("Warning chunk of %d bytes > mtu:%d and yet PMTU disc missed\n",
						    chk->send_size, mtu);
					chk->flags |= CHUNK_FLAGS_FRAGMENT_OK;
				}
				if (SCTP_BASE_SYSCTL(sctp_enable_sack_immediately) &&
				    ((asoc->state & SCTP_STATE_SHUTDOWN_PENDING) == SCTP_STATE_SHUTDOWN_PENDING)) {
					struct sctp_data_chunk *dchkh;

					dchkh = mtod(chk->data, struct sctp_data_chunk *);
					dchkh->ch.chunk_flags |= SCTP_DATA_SACK_IMMEDIATELY;
				}
				if (((chk->send_size <= mtu) && (chk->send_size <= r_mtu)) ||
				    ((chk->flags & CHUNK_FLAGS_FRAGMENT_OK) && (chk->send_size <= asoc->peers_rwnd))) {
					

					




					if (data_auth_reqd) {
						if (auth == NULL) {
							outchain = sctp_add_auth_chunk(outchain,
										       &endoutchain,
										       &auth,
										       &auth_offset,
										       stcb,
										       SCTP_DATA);
							auth_keyid = chk->auth_keyid;
							override_ok = 0;
							SCTP_STAT_INCR_COUNTER64(sctps_outcontrolchunks);
						} else if (override_ok) {
							
							auth_keyid = chk->auth_keyid;
							override_ok = 0;
						} else if (auth_keyid != chk->auth_keyid) {
							
							break;
						}
					}
					outchain = sctp_copy_mbufchain(chk->data, outchain, &endoutchain, 0,
								       chk->send_size, chk->copy_by_ref);
					if (outchain == NULL) {
						SCTPDBG(SCTP_DEBUG_OUTPUT3, "No memory?\n");
						if (!SCTP_OS_TIMER_PENDING(&net->rxt_timer.timer)) {
							sctp_timer_start(SCTP_TIMER_TYPE_SEND, inp, stcb, net);
						}
						*reason_code = 3;
						SCTP_LTRACE_ERR_RET(inp, stcb, NULL, SCTP_FROM_SCTP_OUTPUT, ENOMEM);
						return (ENOMEM);
					}
					
					
					if (chk->flags & CHUNK_FLAGS_FRAGMENT_OK) {
						no_fragmentflg = 0;
					}
					
					if (mtu > chk->send_size)
						mtu -= chk->send_size;
					else
						mtu = 0;
					
					if (r_mtu > chk->send_size)
						r_mtu -= chk->send_size;
					else
						r_mtu = 0;

					to_out += chk->send_size;
					if ((to_out > mx_mtu) && no_fragmentflg) {
#ifdef INVARIANTS
						panic("Exceeding mtu of %d out size is %d", mx_mtu, to_out);
#else
						SCTP_PRINTF("Exceeding mtu of %d out size is %d\n",
							    mx_mtu, to_out);
#endif
					}
					chk->window_probe = 0;
					data_list[bundle_at++] = chk;
					if (bundle_at >= SCTP_MAX_DATA_BUNDLING) {
						break;
					}
					if (chk->sent == SCTP_DATAGRAM_UNSENT) {
						if ((chk->rec.data.rcv_flags & SCTP_DATA_UNORDERED) == 0) {
							SCTP_STAT_INCR_COUNTER64(sctps_outorderchunks);
						} else {
							SCTP_STAT_INCR_COUNTER64(sctps_outunorderchunks);
						}
						if (((chk->rec.data.rcv_flags & SCTP_DATA_LAST_FRAG) == SCTP_DATA_LAST_FRAG) &&
						    ((chk->rec.data.rcv_flags & SCTP_DATA_FIRST_FRAG) == 0))
							



							SCTP_STAT_INCR_COUNTER64(sctps_fragusrmsgs);
					}
					if ((mtu == 0) || (r_mtu == 0) || (one_chunk)) {
						if ((one_chunk) && (stcb->asoc.total_flight == 0)) {
							data_list[0]->window_probe = 1;
							net->window_probe = 1;
						}
						break;
					}
				} else {
					



					break;
				}
			}	
		}		
	no_data_fill:
		
		if (outchain) {
			
			if (asconf) {
				sctp_timer_start(SCTP_TIMER_TYPE_ASCONF, inp,
						 stcb, net);
				



			}
			if (cookie) {
				sctp_timer_start(SCTP_TIMER_TYPE_COOKIE, inp, stcb, net);
				cookie = 0;
			}
			
			if (bundle_at && (!SCTP_OS_TIMER_PENDING(&net->rxt_timer.timer))) {
				



				sctp_timer_start(SCTP_TIMER_TYPE_SEND, inp, stcb, net);
			}
			
			if ((error = sctp_lowlevel_chunk_output(inp,
			                                        stcb,
			                                        net,
			                                        (struct sockaddr *)&net->ro._l_addr,
			                                        outchain,
			                                        auth_offset,
			                                        auth,
			                                        auth_keyid,
			                                        no_fragmentflg,
			                                        bundle_at,
			                                        asconf,
			                                        inp->sctp_lport, stcb->rport,
			                                        htonl(stcb->asoc.peer_vtag),
			                                        net->port, NULL,
#if defined(__FreeBSD__)
			                                        0, 0,
#endif
			                                        so_locked))) {
				
				if (error == ENOBUFS) {
					SCTP_STAT_INCR(sctps_lowlevelerr);
					asoc->ifp_had_enobuf = 1;
				}
				if (from_where == 0) {
					SCTP_STAT_INCR(sctps_lowlevelerrusr);
				}
				SCTPDBG(SCTP_DEBUG_OUTPUT3, "Gak send error %d\n", error);
				if (hbflag) {
					if (*now_filled == 0) {
						(void)SCTP_GETTIME_TIMEVAL(&net->last_sent_time);
						*now_filled = 1;
						*now = net->last_sent_time;
					} else {
						net->last_sent_time = *now;
					}
					hbflag = 0;
				}
				if (error == EHOSTUNREACH) {
					



					sctp_move_chunks_from_net(stcb, net);
				}
				*reason_code = 6;
				





				ctl_cnt = bundle_at = 0;
				continue; 
			} else {
				asoc->ifp_had_enobuf = 0;
			}
			endoutchain = NULL;
			auth = NULL;
			auth_offset = 0;
			if (bundle_at || hbflag) {
				
				if (*now_filled == 0) {
					(void)SCTP_GETTIME_TIMEVAL(&net->last_sent_time);
					*now_filled = 1;
					*now = net->last_sent_time;
				} else {
					net->last_sent_time = *now;
				}
			}
			if (!no_out_cnt) {
				*num_out += (ctl_cnt + bundle_at);
			}
			if (bundle_at) {
				
				tsns_sent = data_list[0]->rec.data.TSN_seq;
				
				if (*now_filled == 0) {
					(void)SCTP_GETTIME_TIMEVAL(&asoc->time_last_sent);
					*now_filled = 1;
					*now = asoc->time_last_sent;
				} else {
					asoc->time_last_sent = *now;
				}
				if (net->rto_needed) {
					data_list[0]->do_rtt = 1;
					net->rto_needed = 0;
				}
				SCTP_STAT_INCR_BY(sctps_senddata, bundle_at);
				sctp_clean_up_datalist(stcb, asoc, data_list, bundle_at, net);
			}
			if (one_chunk) {
				break;
			}
		}
		if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_CWND_LOGGING_ENABLE) {
			sctp_log_cwnd(stcb, net, tsns_sent, SCTP_CWND_LOG_FROM_SEND);
		}
	}
	if (old_start_at == NULL) {
		old_start_at = start_at;
		start_at = TAILQ_FIRST(&asoc->nets);
		if (old_start_at)
			goto again_one_more_time;
	}

	



	if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_CWND_LOGGING_ENABLE) {
		sctp_log_cwnd(stcb, net, *num_out, SCTP_CWND_LOG_FROM_SEND);
	}
	if ((*num_out == 0) && (*reason_code == 0)) {
		*reason_code = 4;
	} else {
		*reason_code = 5;
	}
	sctp_clean_up_ctl(stcb, asoc, so_locked);
	return (0);
}

void
sctp_queue_op_err(struct sctp_tcb *stcb, struct mbuf *op_err)
{
	



	struct sctp_chunkhdr *hdr;
	struct sctp_tmit_chunk *chk;
	struct mbuf *mat;

	SCTP_TCB_LOCK_ASSERT(stcb);
	sctp_alloc_a_chunk(stcb, chk);
	if (chk == NULL) {
		
		sctp_m_freem(op_err);
		return;
	}
	chk->copy_by_ref = 0;
	SCTP_BUF_PREPEND(op_err, sizeof(struct sctp_chunkhdr), M_NOWAIT);
	if (op_err == NULL) {
		sctp_free_a_chunk(stcb, chk, SCTP_SO_NOT_LOCKED);
		return;
	}
	chk->send_size = 0;
	mat = op_err;
	while (mat != NULL) {
		chk->send_size += SCTP_BUF_LEN(mat);
		mat = SCTP_BUF_NEXT(mat);
	}
	chk->rec.chunk_id.id = SCTP_OPERATION_ERROR;
	chk->rec.chunk_id.can_take_data = 1;
	chk->sent = SCTP_DATAGRAM_UNSENT;
	chk->snd_count = 0;
	chk->flags = 0;
	chk->asoc = &stcb->asoc;
	chk->data = op_err;
	chk->whoTo = NULL;
	hdr = mtod(op_err, struct sctp_chunkhdr *);
	hdr->chunk_type = SCTP_OPERATION_ERROR;
	hdr->chunk_flags = 0;
	hdr->chunk_length = htons(chk->send_size);
	TAILQ_INSERT_TAIL(&chk->asoc->control_send_queue,
	    chk,
	    sctp_next);
	chk->asoc->ctrl_queue_cnt++;
}

int
sctp_send_cookie_echo(struct mbuf *m,
    int offset,
    struct sctp_tcb *stcb,
    struct sctp_nets *net)
{
	



	int at;
	struct mbuf *cookie;
	struct sctp_paramhdr parm, *phdr;
	struct sctp_chunkhdr *hdr;
	struct sctp_tmit_chunk *chk;
	uint16_t ptype, plen;

	
	cookie = NULL;
	at = offset + sizeof(struct sctp_init_chunk);

	SCTP_TCB_LOCK_ASSERT(stcb);
	do {
		phdr = sctp_get_next_param(m, at, &parm, sizeof(parm));
		if (phdr == NULL) {
			return (-3);
		}
		ptype = ntohs(phdr->param_type);
		plen = ntohs(phdr->param_length);
		if (ptype == SCTP_STATE_COOKIE) {
			int pad;

			
			if ((pad = (plen % 4))) {
				plen += 4 - pad;
			}
			cookie = SCTP_M_COPYM(m, at, plen, M_NOWAIT);
			if (cookie == NULL) {
				
				return (-2);
			}
#ifdef SCTP_MBUF_LOGGING
			if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_MBUF_LOGGING_ENABLE) {
				struct mbuf *mat;

				for (mat = cookie; mat; mat = SCTP_BUF_NEXT(mat)) {
					if (SCTP_BUF_IS_EXTENDED(mat)) {
						sctp_log_mb(mat, SCTP_MBUF_ICOPY);
					}
				}
			}
#endif
			break;
		}
		at += SCTP_SIZE32(plen);
	} while (phdr);
	if (cookie == NULL) {
		
		return (-3);
	}
	

	
	hdr = mtod(cookie, struct sctp_chunkhdr *);
	hdr->chunk_type = SCTP_COOKIE_ECHO;
	hdr->chunk_flags = 0;
	
	sctp_alloc_a_chunk(stcb, chk);
	if (chk == NULL) {
		
		sctp_m_freem(cookie);
		return (-5);
	}
	chk->copy_by_ref = 0;
	chk->send_size = plen;
	chk->rec.chunk_id.id = SCTP_COOKIE_ECHO;
	chk->rec.chunk_id.can_take_data = 0;
	chk->sent = SCTP_DATAGRAM_UNSENT;
	chk->snd_count = 0;
	chk->flags = CHUNK_FLAGS_FRAGMENT_OK;
	chk->asoc = &stcb->asoc;
	chk->data = cookie;
	chk->whoTo = net;
	atomic_add_int(&chk->whoTo->ref_count, 1);
	TAILQ_INSERT_HEAD(&chk->asoc->control_send_queue, chk, sctp_next);
	chk->asoc->ctrl_queue_cnt++;
	return (0);
}

void
sctp_send_heartbeat_ack(struct sctp_tcb *stcb,
    struct mbuf *m,
    int offset,
    int chk_length,
    struct sctp_nets *net)
{
	


	struct mbuf *outchain;
	struct sctp_chunkhdr *chdr;
	struct sctp_tmit_chunk *chk;


	if (net == NULL)
		
		return;

	outchain = SCTP_M_COPYM(m, offset, chk_length, M_NOWAIT);
	if (outchain == NULL) {
		
		return;
	}
#ifdef SCTP_MBUF_LOGGING
	if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_MBUF_LOGGING_ENABLE) {
		struct mbuf *mat;

		for (mat = outchain; mat; mat = SCTP_BUF_NEXT(mat)) {
			if (SCTP_BUF_IS_EXTENDED(mat)) {
				sctp_log_mb(mat, SCTP_MBUF_ICOPY);
			}
		}
	}
#endif
	chdr = mtod(outchain, struct sctp_chunkhdr *);
	chdr->chunk_type = SCTP_HEARTBEAT_ACK;
	chdr->chunk_flags = 0;
	if (chk_length % 4) {
		
		uint32_t cpthis = 0;
		int padlen;

		padlen = 4 - (chk_length % 4);
		m_copyback(outchain, chk_length, padlen, (caddr_t)&cpthis);
	}
	sctp_alloc_a_chunk(stcb, chk);
	if (chk == NULL) {
		
		sctp_m_freem(outchain);
		return;
	}
	chk->copy_by_ref = 0;
	chk->send_size = chk_length;
	chk->rec.chunk_id.id = SCTP_HEARTBEAT_ACK;
	chk->rec.chunk_id.can_take_data = 1;
	chk->sent = SCTP_DATAGRAM_UNSENT;
	chk->snd_count = 0;
	chk->flags = 0;
	chk->asoc = &stcb->asoc;
	chk->data = outchain;
	chk->whoTo = net;
	atomic_add_int(&chk->whoTo->ref_count, 1);
	TAILQ_INSERT_TAIL(&chk->asoc->control_send_queue, chk, sctp_next);
	chk->asoc->ctrl_queue_cnt++;
}

void
sctp_send_cookie_ack(struct sctp_tcb *stcb)
{
	
	struct mbuf *cookie_ack;
	struct sctp_chunkhdr *hdr;
	struct sctp_tmit_chunk *chk;

	cookie_ack = NULL;
	SCTP_TCB_LOCK_ASSERT(stcb);

	cookie_ack = sctp_get_mbuf_for_msg(sizeof(struct sctp_chunkhdr), 0, M_NOWAIT, 1, MT_HEADER);
	if (cookie_ack == NULL) {
		
		return;
	}
	SCTP_BUF_RESV_UF(cookie_ack, SCTP_MIN_OVERHEAD);
	sctp_alloc_a_chunk(stcb, chk);
	if (chk == NULL) {
		
		sctp_m_freem(cookie_ack);
		return;
	}
	chk->copy_by_ref = 0;
	chk->send_size = sizeof(struct sctp_chunkhdr);
	chk->rec.chunk_id.id = SCTP_COOKIE_ACK;
	chk->rec.chunk_id.can_take_data = 1;
	chk->sent = SCTP_DATAGRAM_UNSENT;
	chk->snd_count = 0;
	chk->flags = 0;
	chk->asoc = &stcb->asoc;
	chk->data = cookie_ack;
	if (chk->asoc->last_control_chunk_from != NULL) {
		chk->whoTo = chk->asoc->last_control_chunk_from;
		atomic_add_int(&chk->whoTo->ref_count, 1);
	} else {
		chk->whoTo = NULL;
	}
	hdr = mtod(cookie_ack, struct sctp_chunkhdr *);
	hdr->chunk_type = SCTP_COOKIE_ACK;
	hdr->chunk_flags = 0;
	hdr->chunk_length = htons(chk->send_size);
	SCTP_BUF_LEN(cookie_ack) = chk->send_size;
	TAILQ_INSERT_TAIL(&chk->asoc->control_send_queue, chk, sctp_next);
	chk->asoc->ctrl_queue_cnt++;
	return;
}


void
sctp_send_shutdown_ack(struct sctp_tcb *stcb, struct sctp_nets *net)
{
	
	struct mbuf *m_shutdown_ack;
	struct sctp_shutdown_ack_chunk *ack_cp;
	struct sctp_tmit_chunk *chk;

	m_shutdown_ack = sctp_get_mbuf_for_msg(sizeof(struct sctp_shutdown_ack_chunk), 0, M_NOWAIT, 1, MT_HEADER);
	if (m_shutdown_ack == NULL) {
		
		return;
	}
	SCTP_BUF_RESV_UF(m_shutdown_ack, SCTP_MIN_OVERHEAD);
	sctp_alloc_a_chunk(stcb, chk);
	if (chk == NULL) {
		
		sctp_m_freem(m_shutdown_ack);
		return;
	}
	chk->copy_by_ref = 0;
	chk->send_size = sizeof(struct sctp_chunkhdr);
	chk->rec.chunk_id.id = SCTP_SHUTDOWN_ACK;
	chk->rec.chunk_id.can_take_data = 1;
	chk->sent = SCTP_DATAGRAM_UNSENT;
	chk->snd_count = 0;
	chk->flags = 0;
	chk->asoc = &stcb->asoc;
	chk->data = m_shutdown_ack;
	chk->whoTo = net;
	if (chk->whoTo) {
		atomic_add_int(&chk->whoTo->ref_count, 1);
	}
	ack_cp = mtod(m_shutdown_ack, struct sctp_shutdown_ack_chunk *);
	ack_cp->ch.chunk_type = SCTP_SHUTDOWN_ACK;
	ack_cp->ch.chunk_flags = 0;
	ack_cp->ch.chunk_length = htons(chk->send_size);
	SCTP_BUF_LEN(m_shutdown_ack) = chk->send_size;
	TAILQ_INSERT_TAIL(&chk->asoc->control_send_queue, chk, sctp_next);
	chk->asoc->ctrl_queue_cnt++;
	return;
}

void
sctp_send_shutdown(struct sctp_tcb *stcb, struct sctp_nets *net)
{
	
	struct mbuf *m_shutdown;
	struct sctp_shutdown_chunk *shutdown_cp;
	struct sctp_tmit_chunk *chk;

	m_shutdown = sctp_get_mbuf_for_msg(sizeof(struct sctp_shutdown_chunk), 0, M_NOWAIT, 1, MT_HEADER);
	if (m_shutdown == NULL) {
		
		return;
	}
	SCTP_BUF_RESV_UF(m_shutdown, SCTP_MIN_OVERHEAD);
	sctp_alloc_a_chunk(stcb, chk);
	if (chk == NULL) {
		
		sctp_m_freem(m_shutdown);
		return;
	}
	chk->copy_by_ref = 0;
	chk->send_size = sizeof(struct sctp_shutdown_chunk);
	chk->rec.chunk_id.id = SCTP_SHUTDOWN;
	chk->rec.chunk_id.can_take_data = 1;
	chk->sent = SCTP_DATAGRAM_UNSENT;
	chk->snd_count = 0;
	chk->flags = 0;
	chk->asoc = &stcb->asoc;
	chk->data = m_shutdown;
	chk->whoTo = net;
	if (chk->whoTo) {
		atomic_add_int(&chk->whoTo->ref_count, 1);
	}
	shutdown_cp = mtod(m_shutdown, struct sctp_shutdown_chunk *);
	shutdown_cp->ch.chunk_type = SCTP_SHUTDOWN;
	shutdown_cp->ch.chunk_flags = 0;
	shutdown_cp->ch.chunk_length = htons(chk->send_size);
	shutdown_cp->cumulative_tsn_ack = htonl(stcb->asoc.cumulative_tsn);
	SCTP_BUF_LEN(m_shutdown) = chk->send_size;
	TAILQ_INSERT_TAIL(&chk->asoc->control_send_queue, chk, sctp_next);
	chk->asoc->ctrl_queue_cnt++;
	return;
}

void
sctp_send_asconf(struct sctp_tcb *stcb, struct sctp_nets *net, int addr_locked)
{
	



	struct sctp_tmit_chunk *chk;
	struct mbuf *m_asconf;
	int len;

	SCTP_TCB_LOCK_ASSERT(stcb);

	if ((!TAILQ_EMPTY(&stcb->asoc.asconf_send_queue)) &&
	    (!sctp_is_feature_on(stcb->sctp_ep, SCTP_PCB_FLAGS_MULTIPLE_ASCONFS))) {
		
		return;
	}

	
	m_asconf = sctp_compose_asconf(stcb, &len, addr_locked);
	if (m_asconf == NULL) {
		return;
	}

	sctp_alloc_a_chunk(stcb, chk);
	if (chk == NULL) {
		
		sctp_m_freem(m_asconf);
		return;
	}

	chk->copy_by_ref = 0;
	chk->data = m_asconf;
	chk->send_size = len;
	chk->rec.chunk_id.id = SCTP_ASCONF;
	chk->rec.chunk_id.can_take_data = 0;
	chk->sent = SCTP_DATAGRAM_UNSENT;
	chk->snd_count = 0;
	chk->flags = CHUNK_FLAGS_FRAGMENT_OK;
	chk->asoc = &stcb->asoc;
	chk->whoTo = net;
	if (chk->whoTo) {
		atomic_add_int(&chk->whoTo->ref_count, 1);
	}
	TAILQ_INSERT_TAIL(&chk->asoc->asconf_send_queue, chk, sctp_next);
	chk->asoc->ctrl_queue_cnt++;
	return;
}

void
sctp_send_asconf_ack(struct sctp_tcb *stcb)
{
	



	struct sctp_tmit_chunk *chk;
	struct sctp_asconf_ack *ack, *latest_ack;
	struct mbuf *m_ack;
	struct sctp_nets *net = NULL;

	SCTP_TCB_LOCK_ASSERT(stcb);
	
	latest_ack = TAILQ_LAST(&stcb->asoc.asconf_ack_sent, sctp_asconf_ackhead);
	if (latest_ack == NULL) {
		return;
	}
	if (latest_ack->last_sent_to != NULL &&
	    latest_ack->last_sent_to == stcb->asoc.last_control_chunk_from) {
		
		net = sctp_find_alternate_net(stcb, stcb->asoc.last_control_chunk_from, 0);
		if (net == NULL) {
			
			if (stcb->asoc.last_control_chunk_from == NULL) {
				if (stcb->asoc.alternate) {
					net = stcb->asoc.alternate;
				} else {
					net = stcb->asoc.primary_destination;
				}
			} else {
				net = stcb->asoc.last_control_chunk_from;
			}
		}
	} else {
		
		if (stcb->asoc.last_control_chunk_from == NULL) {
			if (stcb->asoc.alternate) {
				net = stcb->asoc.alternate;
			} else {
				net = stcb->asoc.primary_destination;
			}
		} else {
			net = stcb->asoc.last_control_chunk_from;
		}
	}
	latest_ack->last_sent_to = net;

	TAILQ_FOREACH(ack, &stcb->asoc.asconf_ack_sent, next) {
		if (ack->data == NULL) {
			continue;
		}

		
		m_ack = SCTP_M_COPYM(ack->data, 0, M_COPYALL, M_NOWAIT);
		if (m_ack == NULL) {
			
			return;
		}
#ifdef SCTP_MBUF_LOGGING
		if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_MBUF_LOGGING_ENABLE) {
			struct mbuf *mat;

			for (mat = m_ack; mat; mat = SCTP_BUF_NEXT(mat)) {
				if (SCTP_BUF_IS_EXTENDED(mat)) {
					sctp_log_mb(mat, SCTP_MBUF_ICOPY);
				}
			}
		}
#endif

		sctp_alloc_a_chunk(stcb, chk);
		if (chk == NULL) {
			
			if (m_ack)
				sctp_m_freem(m_ack);
			return;
		}
		chk->copy_by_ref = 0;

		chk->whoTo = net;
		if (chk->whoTo) {
			atomic_add_int(&chk->whoTo->ref_count, 1);
		}
		chk->data = m_ack;
		chk->send_size = 0;
		
		chk->send_size = ack->len;
		chk->rec.chunk_id.id = SCTP_ASCONF_ACK;
		chk->rec.chunk_id.can_take_data = 1;
		chk->sent = SCTP_DATAGRAM_UNSENT;
		chk->snd_count = 0;
		chk->flags |= CHUNK_FLAGS_FRAGMENT_OK; 
		chk->asoc = &stcb->asoc;

		TAILQ_INSERT_TAIL(&chk->asoc->control_send_queue, chk, sctp_next);
		chk->asoc->ctrl_queue_cnt++;
	}
	return;
}


static int
sctp_chunk_retransmission(struct sctp_inpcb *inp,
    struct sctp_tcb *stcb,
    struct sctp_association *asoc,
    int *cnt_out, struct timeval *now, int *now_filled, int *fr_done, int so_locked
#if !defined(__APPLE__) && !defined(SCTP_SO_LOCK_TESTING)
    SCTP_UNUSED
#endif
    )
{
	











	struct sctp_tmit_chunk *data_list[SCTP_MAX_DATA_BUNDLING];
	struct sctp_tmit_chunk *chk, *fwd;
	struct mbuf *m, *endofchain;
	struct sctp_nets *net = NULL;
	uint32_t tsns_sent = 0;
	int no_fragmentflg, bundle_at, cnt_thru;
	unsigned int mtu;
	int error, i, one_chunk, fwd_tsn, ctl_cnt, tmr_started;
	struct sctp_auth_chunk *auth = NULL;
	uint32_t auth_offset = 0;
	uint16_t auth_keyid;
	int override_ok = 1;
	int data_auth_reqd = 0;
	uint32_t dmtu = 0;

#if defined(__APPLE__)
	if (so_locked) {
		sctp_lock_assert(SCTP_INP_SO(inp));
	} else {
		sctp_unlock_assert(SCTP_INP_SO(inp));
	}
#endif
	SCTP_TCB_LOCK_ASSERT(stcb);
	tmr_started = ctl_cnt = bundle_at = error = 0;
	no_fragmentflg = 1;
	fwd_tsn = 0;
	*cnt_out = 0;
	fwd = NULL;
	endofchain = m = NULL;
	auth_keyid = stcb->asoc.authinfo.active_keyid;
#ifdef SCTP_AUDITING_ENABLED
	sctp_audit_log(0xC3, 1);
#endif
	if ((TAILQ_EMPTY(&asoc->sent_queue)) &&
	    (TAILQ_EMPTY(&asoc->control_send_queue))) {
		SCTPDBG(SCTP_DEBUG_OUTPUT1,"SCTP hits empty queue with cnt set to %d?\n",
			asoc->sent_queue_retran_cnt);
		asoc->sent_queue_cnt = 0;
		asoc->sent_queue_cnt_removeable = 0;
		
		*cnt_out = 0;
		return (0);
	}
	TAILQ_FOREACH(chk, &asoc->control_send_queue, sctp_next) {
		if ((chk->rec.chunk_id.id == SCTP_COOKIE_ECHO) ||
		    (chk->rec.chunk_id.id == SCTP_STREAM_RESET) ||
		    (chk->rec.chunk_id.id == SCTP_FORWARD_CUM_TSN)) {
			if (chk->sent != SCTP_DATAGRAM_RESEND) {
				continue;
			}
			if (chk->rec.chunk_id.id == SCTP_STREAM_RESET) {
				if (chk != asoc->str_reset) {
					



					continue;
				}
			}
			ctl_cnt++;
			if (chk->rec.chunk_id.id == SCTP_FORWARD_CUM_TSN) {
				fwd_tsn = 1;
			}
			



			if ((auth == NULL) &&
			    (sctp_auth_is_required_chunk(chk->rec.chunk_id.id,
							 stcb->asoc.peer_auth_chunks))) {
				m = sctp_add_auth_chunk(m, &endofchain,
							&auth, &auth_offset,
							stcb,
							chk->rec.chunk_id.id);
				SCTP_STAT_INCR_COUNTER64(sctps_outcontrolchunks);
			}
			m = sctp_copy_mbufchain(chk->data, m, &endofchain, 0, chk->send_size, chk->copy_by_ref);
			break;
		}
	}
	one_chunk = 0;
	cnt_thru = 0;
	
	if (m != NULL) {
		
		if (chk->rec.chunk_id.id == SCTP_COOKIE_ECHO) {
			sctp_timer_start(SCTP_TIMER_TYPE_COOKIE, inp, stcb, chk->whoTo);
		} else if (chk->rec.chunk_id.id == SCTP_ASCONF)
			sctp_timer_start(SCTP_TIMER_TYPE_ASCONF, inp, stcb, chk->whoTo);
		chk->snd_count++;	
		if ((error = sctp_lowlevel_chunk_output(inp, stcb, chk->whoTo,
		                                        (struct sockaddr *)&chk->whoTo->ro._l_addr, m,
		                                        auth_offset, auth, stcb->asoc.authinfo.active_keyid,
		                                        no_fragmentflg, 0, 0,
		                                        inp->sctp_lport, stcb->rport, htonl(stcb->asoc.peer_vtag),
		                                        chk->whoTo->port, NULL,
#if defined(__FreeBSD__)
		                                        0, 0,
#endif
		                                        so_locked))) {
			SCTP_STAT_INCR(sctps_lowlevelerr);
			return (error);
		}
		endofchain = NULL;
		auth = NULL;
		auth_offset = 0;
		



		
		*cnt_out += 1;
		chk->sent = SCTP_DATAGRAM_SENT;
		sctp_ucount_decr(stcb->asoc.sent_queue_retran_cnt);
		if (fwd_tsn == 0) {
			return (0);
		} else {
			
			sctp_clean_up_ctl(stcb, asoc, so_locked);
			return (0);
		}
	}
	



	if (TAILQ_EMPTY(&asoc->sent_queue)) {
		return (SCTP_RETRAN_DONE);
	}
	if ((SCTP_GET_STATE(asoc) == SCTP_STATE_COOKIE_ECHOED) ||
	    (SCTP_GET_STATE(asoc) == SCTP_STATE_COOKIE_WAIT)) {
		
		return (1);
	}
#ifdef SCTP_AUDITING_ENABLED
	sctp_auditing(20, inp, stcb, NULL);
#endif
	data_auth_reqd = sctp_auth_is_required_chunk(SCTP_DATA, stcb->asoc.peer_auth_chunks);
	TAILQ_FOREACH(chk, &asoc->sent_queue, sctp_next) {
		if (chk->sent != SCTP_DATAGRAM_RESEND) {
			
			continue;
		}
		if (chk->data == NULL) {
			SCTP_PRINTF("TSN:%x chk->snd_count:%d chk->sent:%d can't retran - no data\n",
			            chk->rec.data.TSN_seq, chk->snd_count, chk->sent);
			continue;
		}
		if ((SCTP_BASE_SYSCTL(sctp_max_retran_chunk)) &&
		    (chk->snd_count >= SCTP_BASE_SYSCTL(sctp_max_retran_chunk))) {
			
			SCTP_PRINTF("Gak, chk->snd_count:%d >= max:%d - send abort\n",
				    chk->snd_count,
				    SCTP_BASE_SYSCTL(sctp_max_retran_chunk));
			atomic_add_int(&stcb->asoc.refcnt, 1);
			sctp_abort_an_association(stcb->sctp_ep, stcb, NULL, so_locked);
			SCTP_TCB_LOCK(stcb);
			atomic_subtract_int(&stcb->asoc.refcnt, 1);
			return (SCTP_RETRAN_EXIT);
		}
		
		net = chk->whoTo;
		switch (net->ro._l_addr.sa.sa_family) {
#ifdef INET
			case AF_INET:
				mtu = net->mtu - SCTP_MIN_V4_OVERHEAD;
				break;
#endif
#ifdef INET6
			case AF_INET6:
				mtu = net->mtu - SCTP_MIN_OVERHEAD;
				break;
#endif
#if defined(__Userspace__)
			case AF_CONN:
				mtu = net->mtu - sizeof(struct sctphdr);
				break;
#endif
			default:
				
				mtu = net->mtu;
				break;
		}

		if ((asoc->peers_rwnd < mtu) && (asoc->total_flight > 0)) {
			
			uint32_t tsn;

			tsn = asoc->last_acked_seq + 1;
			if (tsn == chk->rec.data.TSN_seq) {
				





				goto one_chunk_around;
			}
			return (1);
		}
	one_chunk_around:
		if (asoc->peers_rwnd < mtu) {
			one_chunk = 1;
			if ((asoc->peers_rwnd == 0) &&
			    (asoc->total_flight == 0)) {
				chk->window_probe = 1;
				chk->whoTo->window_probe = 1;
			}
		}
#ifdef SCTP_AUDITING_ENABLED
		sctp_audit_log(0xC3, 2);
#endif
		bundle_at = 0;
		m = NULL;
		net->fast_retran_ip = 0;
		if (chk->rec.data.doing_fast_retransmit == 0) {
			



			if (net->flight_size >= net->cwnd) {
				continue;
			}
		} else {
			



			*fr_done = 1;
			net->fast_retran_ip = 1;
		}

		





		if (data_auth_reqd && (auth == NULL)) {
			dmtu = sctp_get_auth_chunk_len(stcb->asoc.peer_hmac_id);
		} else
			dmtu = 0;

		if ((chk->send_size <= (mtu - dmtu)) ||
		    (chk->flags & CHUNK_FLAGS_FRAGMENT_OK)) {
			
			if (data_auth_reqd) {
				if (auth == NULL) {
					m = sctp_add_auth_chunk(m,
								&endofchain,
								&auth,
								&auth_offset,
								stcb,
								SCTP_DATA);
					auth_keyid = chk->auth_keyid;
					override_ok = 0;
					SCTP_STAT_INCR_COUNTER64(sctps_outcontrolchunks);
				} else if (override_ok) {
					auth_keyid = chk->auth_keyid;
					override_ok = 0;
				} else if (chk->auth_keyid != auth_keyid) {
					
					break;
				}
			}
			m = sctp_copy_mbufchain(chk->data, m, &endofchain, 0, chk->send_size, chk->copy_by_ref);
			if (m == NULL) {
				SCTP_LTRACE_ERR_RET(inp, stcb, NULL, SCTP_FROM_SCTP_OUTPUT, ENOMEM);
				return (ENOMEM);
			}
			
			if (chk->flags & CHUNK_FLAGS_FRAGMENT_OK) {
				no_fragmentflg = 0;
			}
			
			if (mtu > (chk->send_size + dmtu))
				mtu -= (chk->send_size + dmtu);
			else
				mtu = 0;
			data_list[bundle_at++] = chk;
			if (one_chunk && (asoc->total_flight <= 0)) {
				SCTP_STAT_INCR(sctps_windowprobed);
			}
		}
		if (one_chunk == 0) {
			



			for (fwd = TAILQ_NEXT(chk, sctp_next); fwd != NULL; fwd = TAILQ_NEXT(fwd, sctp_next)) {
				if (fwd->sent != SCTP_DATAGRAM_RESEND) {
					
					continue;
				}
				if (fwd->whoTo != net) {
					
					continue;
				}
				if (data_auth_reqd && (auth == NULL)) {
					dmtu = sctp_get_auth_chunk_len(stcb->asoc.peer_hmac_id);
				} else
					dmtu = 0;
				if (fwd->send_size <= (mtu - dmtu)) {
					if (data_auth_reqd) {
						if (auth == NULL) {
							m = sctp_add_auth_chunk(m,
										&endofchain,
										&auth,
										&auth_offset,
										stcb,
										SCTP_DATA);
							auth_keyid = fwd->auth_keyid;
							override_ok = 0;
							SCTP_STAT_INCR_COUNTER64(sctps_outcontrolchunks);
						} else if (override_ok) {
							auth_keyid = fwd->auth_keyid;
							override_ok = 0;
						} else if (fwd->auth_keyid != auth_keyid) {
							
							break;
						}
					}
					m = sctp_copy_mbufchain(fwd->data, m, &endofchain, 0, fwd->send_size, fwd->copy_by_ref);
					if (m == NULL) {
						SCTP_LTRACE_ERR_RET(inp, stcb, NULL, SCTP_FROM_SCTP_OUTPUT, ENOMEM);
						return (ENOMEM);
					}
					
					if (fwd->flags & CHUNK_FLAGS_FRAGMENT_OK) {
						no_fragmentflg = 0;
					}
					
					if (mtu > (fwd->send_size + dmtu))
						mtu -= (fwd->send_size + dmtu);
					else
						mtu = 0;
					data_list[bundle_at++] = fwd;
					if (bundle_at >= SCTP_MAX_DATA_BUNDLING) {
						break;
					}
				} else {
					
					break;
				}
			}
		}
		
		if (m) {
			



			if (!SCTP_OS_TIMER_PENDING(&net->rxt_timer.timer)) {
				



				sctp_timer_start(SCTP_TIMER_TYPE_SEND, inp, stcb, net);
				tmr_started = 1;
			}
			
			if ((error = sctp_lowlevel_chunk_output(inp, stcb, net,
			                                        (struct sockaddr *)&net->ro._l_addr, m,
			                                        auth_offset, auth, auth_keyid,
			                                        no_fragmentflg, 0, 0,
			                                        inp->sctp_lport, stcb->rport, htonl(stcb->asoc.peer_vtag),
			                                        net->port, NULL,
#if defined(__FreeBSD__)
			                                        0, 0,
#endif
			                                        so_locked))) {
				
				SCTP_STAT_INCR(sctps_lowlevelerr);
				return (error);
			}
			endofchain = NULL;
			auth = NULL;
			auth_offset = 0;
			
			




			

			
			cnt_thru++;
			if (*now_filled == 0) {
				(void)SCTP_GETTIME_TIMEVAL(&asoc->time_last_sent);
				*now = asoc->time_last_sent;
				*now_filled = 1;
			} else {
				asoc->time_last_sent = *now;
			}
			*cnt_out += bundle_at;
#ifdef SCTP_AUDITING_ENABLED
			sctp_audit_log(0xC4, bundle_at);
#endif
			if (bundle_at) {
				tsns_sent = data_list[0]->rec.data.TSN_seq;
			}
			for (i = 0; i < bundle_at; i++) {
				SCTP_STAT_INCR(sctps_sendretransdata);
				data_list[i]->sent = SCTP_DATAGRAM_SENT;
				





				if (data_list[i]->rec.data.chunk_was_revoked) {
					
					data_list[i]->whoTo->cwnd -= data_list[i]->book_size;
					data_list[i]->rec.data.chunk_was_revoked = 0;
				}
				data_list[i]->snd_count++;
				sctp_ucount_decr(asoc->sent_queue_retran_cnt);
				
				data_list[i]->sent_rcv_time = asoc->time_last_sent;
				if (data_list[i]->book_size_scale) {
					



					data_list[i]->book_size_scale = 0;
					



					atomic_add_int(&((asoc)->total_output_queue_size),data_list[i]->book_size);
					data_list[i]->book_size *= 2;


				} else {
					if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_LOG_RWND_ENABLE) {
						sctp_log_rwnd(SCTP_DECREASE_PEER_RWND,
						      asoc->peers_rwnd, data_list[i]->send_size, SCTP_BASE_SYSCTL(sctp_peer_chunk_oh));
					}
					asoc->peers_rwnd = sctp_sbspace_sub(asoc->peers_rwnd,
									    (uint32_t) (data_list[i]->send_size +
											SCTP_BASE_SYSCTL(sctp_peer_chunk_oh)));
				}
				if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_FLIGHT_LOGGING_ENABLE) {
					sctp_misc_ints(SCTP_FLIGHT_LOG_UP_RSND,
						       data_list[i]->whoTo->flight_size,
						       data_list[i]->book_size,
						       (uintptr_t)data_list[i]->whoTo,
						       data_list[i]->rec.data.TSN_seq);
				}
				sctp_flight_size_increase(data_list[i]);
				sctp_total_flight_increase(stcb, data_list[i]);
				if (asoc->peers_rwnd < stcb->sctp_ep->sctp_ep.sctp_sws_sender) {
					
					asoc->peers_rwnd = 0;
				}
				if ((i == 0) &&
				    (data_list[i]->rec.data.doing_fast_retransmit)) {
					SCTP_STAT_INCR(sctps_sendfastretrans);
					if ((data_list[i] == TAILQ_FIRST(&asoc->sent_queue)) &&
					    (tmr_started == 0)) {
						








						sctp_timer_stop(SCTP_TIMER_TYPE_SEND, inp, stcb, net,
								SCTP_FROM_SCTP_OUTPUT+SCTP_LOC_4);
						sctp_timer_start(SCTP_TIMER_TYPE_SEND, inp, stcb, net);
					}
				}
			}
			if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_CWND_LOGGING_ENABLE) {
				sctp_log_cwnd(stcb, net, tsns_sent, SCTP_CWND_LOG_FROM_RESEND);
			}
#ifdef SCTP_AUDITING_ENABLED
			sctp_auditing(21, inp, stcb, NULL);
#endif
		} else {
			
			return (1);
		}
		if (asoc->sent_queue_retran_cnt <= 0) {
			
			asoc->sent_queue_retran_cnt = 0;
			break;
		}
		if (one_chunk) {
			
			return (1);
		}
		
		break;
	}
	return (0);
}

static void
sctp_timer_validation(struct sctp_inpcb *inp,
    struct sctp_tcb *stcb,
    struct sctp_association *asoc)
{
	struct sctp_nets *net;

	
	TAILQ_FOREACH(net, &asoc->nets, sctp_next) {
		if (SCTP_OS_TIMER_PENDING(&net->rxt_timer.timer)) {
			
			return;
		}
	}
	SCTP_TCB_LOCK_ASSERT(stcb);
	
	SCTPDBG(SCTP_DEBUG_OUTPUT3, "Deadlock avoided starting timer on a dest at retran\n");
	if (asoc->alternate) {
		sctp_timer_start(SCTP_TIMER_TYPE_SEND, inp, stcb, asoc->alternate);
	} else {
		sctp_timer_start(SCTP_TIMER_TYPE_SEND, inp, stcb, asoc->primary_destination);
	}
	return;
}

void
sctp_chunk_output (struct sctp_inpcb *inp,
    struct sctp_tcb *stcb,
    int from_where,
    int so_locked
#if !defined(__APPLE__) && !defined(SCTP_SO_LOCK_TESTING)
    SCTP_UNUSED
#endif
    )
{
	












	struct sctp_association *asoc;
	struct sctp_nets *net;
	int error = 0, num_out = 0, tot_out = 0, ret = 0, reason_code = 0;
	unsigned int burst_cnt = 0;
	struct timeval now;
	int now_filled = 0;
	int nagle_on;
	int frag_point = sctp_get_frag_point(stcb, &stcb->asoc);
	int un_sent = 0;
	int fr_done;
	unsigned int tot_frs = 0;

#if defined(__APPLE__)
	if (so_locked) {
		sctp_lock_assert(SCTP_INP_SO(inp));
	} else {
		sctp_unlock_assert(SCTP_INP_SO(inp));
	}
#endif
	asoc = &stcb->asoc;
	
	if (from_where == SCTP_OUTPUT_FROM_USR_SEND) {
		if (sctp_is_feature_on(inp, SCTP_PCB_FLAGS_NODELAY)) {
			nagle_on = 0;
		} else {
			nagle_on = 1;
		}
	} else {
		nagle_on = 0;
	}
	SCTP_TCB_LOCK_ASSERT(stcb);

	un_sent = (stcb->asoc.total_output_queue_size - stcb->asoc.total_flight);

	if ((un_sent <= 0) &&
	    (TAILQ_EMPTY(&asoc->control_send_queue)) &&
	    (TAILQ_EMPTY(&asoc->asconf_send_queue)) &&
	    (asoc->sent_queue_retran_cnt == 0)) {
		
		return;
	}
	


 	if (SCTP_OS_TIMER_PENDING(&stcb->asoc.dack_timer.timer)) {
		sctp_send_sack(stcb, so_locked);
		(void)SCTP_OS_TIMER_STOP(&stcb->asoc.dack_timer.timer);
	}
	while (asoc->sent_queue_retran_cnt) {
		



		if (from_where == SCTP_OUTPUT_FROM_COOKIE_ACK) {
			




 			(void)sctp_med_chunk_output(inp, stcb, asoc, &num_out, &reason_code, 1,
						    from_where,
						    &now, &now_filled, frag_point, so_locked);
			return;
		} else if (from_where != SCTP_OUTPUT_FROM_HB_TMR) {
			
			fr_done = 0;
			ret = sctp_chunk_retransmission(inp, stcb, asoc, &num_out, &now, &now_filled, &fr_done, so_locked);
			if (fr_done) {
				tot_frs++;
			}
		} else {
			



			ret = 1;
		}
		if (ret > 0) {
			
			




			(void)sctp_med_chunk_output(inp, stcb, asoc, &num_out, &reason_code, 1,
						    from_where,
						    &now, &now_filled, frag_point, so_locked);
#ifdef SCTP_AUDITING_ENABLED
			sctp_auditing(8, inp, stcb, NULL);
#endif
			sctp_timer_validation(inp, stcb, asoc);
			return;
		}
		if (ret < 0) {
			



#ifdef SCTP_AUDITING_ENABLED
			sctp_auditing(9, inp, stcb, NULL);
#endif
			if (ret == SCTP_RETRAN_EXIT) {
				return;
			}
			break;
		}
		if (from_where == SCTP_OUTPUT_FROM_T3) {
			
#ifdef SCTP_AUDITING_ENABLED
			sctp_auditing(10, inp, stcb, NULL);
#endif
			
			(void)sctp_med_chunk_output(inp, stcb, asoc, &num_out, &reason_code, 1, from_where,
						    &now, &now_filled, frag_point, so_locked);
			return;
		}
		if ((asoc->fr_max_burst > 0) && (tot_frs >= asoc->fr_max_burst)) {
			
			return;
		}
		if ((num_out == 0) && (ret == 0)) {
			
			break;
		}
	}
#ifdef SCTP_AUDITING_ENABLED
	sctp_auditing(12, inp, stcb, NULL);
#endif
	
	TAILQ_FOREACH(net, &asoc->nets, sctp_next) {
		if (!(net->dest_state & SCTP_ADDR_REACHABLE)) {
			






			if (net->ref_count > 1)
				sctp_move_chunks_from_net(stcb, net);
		} else {
			




			if (asoc->max_burst > 0) {
				if (SCTP_BASE_SYSCTL(sctp_use_cwnd_based_maxburst)) {
					if ((net->flight_size + (asoc->max_burst * net->mtu)) < net->cwnd) {
						
						asoc->cc_functions.sctp_cwnd_update_after_output(stcb, net, asoc->max_burst);
						if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_LOG_MAXBURST_ENABLE) {
							sctp_log_maxburst(stcb, net, 0, asoc->max_burst, SCTP_MAX_BURST_APPLIED);
						}
						SCTP_STAT_INCR(sctps_maxburstqueued);
					}
					net->fast_retran_ip = 0;
				} else {
					if (net->flight_size == 0) {
						
						;
					}
				}
			}
		}

	}
	burst_cnt = 0;
	do {
		error = sctp_med_chunk_output(inp, stcb, asoc, &num_out,
					      &reason_code, 0, from_where,
					      &now, &now_filled, frag_point, so_locked);
		if (error) {
			SCTPDBG(SCTP_DEBUG_OUTPUT1, "Error %d was returned from med-c-op\n", error);
			if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_LOG_MAXBURST_ENABLE) {
				sctp_log_maxburst(stcb, asoc->primary_destination, error, burst_cnt, SCTP_MAX_BURST_ERROR_STOP);
			}
			if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_CWND_LOGGING_ENABLE) {
				sctp_log_cwnd(stcb, NULL, error, SCTP_SEND_NOW_COMPLETES);
				sctp_log_cwnd(stcb, NULL, 0xdeadbeef, SCTP_SEND_NOW_COMPLETES);
			}
			break;
		}
		SCTPDBG(SCTP_DEBUG_OUTPUT3, "m-c-o put out %d\n", num_out);

		tot_out += num_out;
		burst_cnt++;
		if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_CWND_LOGGING_ENABLE) {
			sctp_log_cwnd(stcb, NULL, num_out, SCTP_SEND_NOW_COMPLETES);
			if (num_out == 0) {
				sctp_log_cwnd(stcb, NULL, reason_code, SCTP_SEND_NOW_COMPLETES);
			}
		}
		if (nagle_on) {
			





			un_sent = ((stcb->asoc.total_output_queue_size - stcb->asoc.total_flight) +
			           (stcb->asoc.stream_queue_cnt * sizeof(struct sctp_data_chunk)));
			if ((un_sent < (int)(stcb->asoc.smallest_mtu - SCTP_MIN_OVERHEAD)) &&
			    (stcb->asoc.total_flight > 0) &&
			    ((stcb->asoc.locked_on_sending == NULL) ||
			     sctp_is_feature_on(inp, SCTP_PCB_FLAGS_EXPLICIT_EOR))) {
				break;
			}
		}
		if (TAILQ_EMPTY(&asoc->control_send_queue) &&
		    TAILQ_EMPTY(&asoc->send_queue) &&
		    stcb->asoc.ss_functions.sctp_ss_is_empty(stcb, asoc)) {
			
			break;
		}
		if ((stcb->asoc.total_output_queue_size - stcb->asoc.total_flight) <= 0) {
			
			break;
		}
	} while (num_out &&
	         ((asoc->max_burst == 0) ||
		  SCTP_BASE_SYSCTL(sctp_use_cwnd_based_maxburst) ||
		  (burst_cnt < asoc->max_burst)));

	if (SCTP_BASE_SYSCTL(sctp_use_cwnd_based_maxburst) == 0) {
		if ((asoc->max_burst > 0) && (burst_cnt >= asoc->max_burst)) {
			SCTP_STAT_INCR(sctps_maxburstqueued);
			asoc->burst_limit_applied = 1;
			if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_LOG_MAXBURST_ENABLE) {
				sctp_log_maxburst(stcb, asoc->primary_destination, 0, burst_cnt, SCTP_MAX_BURST_APPLIED);
			}
		} else {
			asoc->burst_limit_applied = 0;
		}
	}
	if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_CWND_LOGGING_ENABLE) {
		sctp_log_cwnd(stcb, NULL, tot_out, SCTP_SEND_NOW_COMPLETES);
	}
	SCTPDBG(SCTP_DEBUG_OUTPUT1, "Ok, we have put out %d chunks\n",
		tot_out);

	




	if (stcb->asoc.ecn_echo_cnt_onq)
		sctp_fix_ecn_echo(asoc);
	return;
}


int
sctp_output(
	struct sctp_inpcb *inp,
#if defined(__Panda__)
	pakhandle_type m,
#else
	struct mbuf *m,
#endif
	struct sockaddr *addr,
#if defined(__Panda__)
	pakhandle_type control,
#else
	struct mbuf *control,
#endif
#if defined(__FreeBSD__) && __FreeBSD_version >= 500000
	struct thread *p,
#elif defined(__Windows__)
	PKTHREAD p,
#else
#if defined(__APPLE__)
	struct proc *p SCTP_UNUSED,
#else
	struct proc *p,
#endif
#endif
	int flags)
{
	if (inp == NULL) {
		SCTP_LTRACE_ERR_RET_PKT(m, inp, NULL, NULL, SCTP_FROM_SCTP_OUTPUT, EINVAL);
		return (EINVAL);
	}

	if (inp->sctp_socket == NULL) {
		SCTP_LTRACE_ERR_RET_PKT(m, inp, NULL, NULL, SCTP_FROM_SCTP_OUTPUT, EINVAL);
		return (EINVAL);
	}
	return (sctp_sosend(inp->sctp_socket,
			    addr,
			    (struct uio *)NULL,
			    m,
			    control,
#if defined(__APPLE__) || defined(__Panda__)
			    flags
#else
			    flags, p
#endif
			));
}

void
send_forward_tsn(struct sctp_tcb *stcb,
		 struct sctp_association *asoc)
{
        struct sctp_tmit_chunk *chk;
	struct sctp_forward_tsn_chunk *fwdtsn;
	uint32_t advance_peer_ack_point;

        SCTP_TCB_LOCK_ASSERT(stcb);
	TAILQ_FOREACH(chk, &asoc->control_send_queue, sctp_next) {
		if (chk->rec.chunk_id.id == SCTP_FORWARD_CUM_TSN) {
			
			chk->sent = SCTP_DATAGRAM_UNSENT;
			chk->snd_count = 0;
			
			if (chk->whoTo) {
				sctp_free_remote_addr(chk->whoTo);
				chk->whoTo = NULL;
			}
			goto sctp_fill_in_rest;
		}
	}
	
	sctp_alloc_a_chunk(stcb, chk);
	if (chk == NULL) {
		return;
	}
	asoc->fwd_tsn_cnt++;
	chk->copy_by_ref = 0;
	chk->rec.chunk_id.id = SCTP_FORWARD_CUM_TSN;
	chk->rec.chunk_id.can_take_data = 0;
	chk->asoc = asoc;
	chk->whoTo = NULL;
	chk->data = sctp_get_mbuf_for_msg(MCLBYTES, 0, M_NOWAIT, 1, MT_DATA);
	if (chk->data == NULL) {
		sctp_free_a_chunk(stcb, chk, SCTP_SO_NOT_LOCKED);
		return;
	}
	SCTP_BUF_RESV_UF(chk->data, SCTP_MIN_OVERHEAD);
	chk->sent = SCTP_DATAGRAM_UNSENT;
	chk->snd_count = 0;
	TAILQ_INSERT_TAIL(&asoc->control_send_queue, chk, sctp_next);
	asoc->ctrl_queue_cnt++;
sctp_fill_in_rest:
	



	SCTP_BUF_LEN(chk->data) = 0;
	{
		struct sctp_tmit_chunk *at, *tp1, *last;
		struct sctp_strseq *strseq;
		unsigned int cnt_of_space, i, ovh;
		unsigned int space_needed;
		unsigned int cnt_of_skipped = 0;

		TAILQ_FOREACH(at, &asoc->sent_queue, sctp_next) {
			if ((at->sent != SCTP_FORWARD_TSN_SKIP) &&
			    (at->sent != SCTP_DATAGRAM_NR_ACKED)) {
				
				break;
			}
			if (at->rec.data.rcv_flags & SCTP_DATA_UNORDERED) {
				
				continue;
			}
			cnt_of_skipped++;
		}
		space_needed = (sizeof(struct sctp_forward_tsn_chunk) +
		    (cnt_of_skipped * sizeof(struct sctp_strseq)));

		cnt_of_space = M_TRAILINGSPACE(chk->data);

		if (stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_BOUND_V6) {
			ovh = SCTP_MIN_OVERHEAD;
		} else {
			ovh = SCTP_MIN_V4_OVERHEAD;
		}
		if (cnt_of_space > (asoc->smallest_mtu - ovh)) {
			
			cnt_of_space = asoc->smallest_mtu - ovh;
		}
		if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_LOG_TRY_ADVANCE) {
			sctp_misc_ints(SCTP_FWD_TSN_CHECK,
				       0xff, 0, cnt_of_skipped,
				       asoc->advanced_peer_ack_point);

		}
		advance_peer_ack_point = asoc->advanced_peer_ack_point;
		if (cnt_of_space < space_needed) {
			



			if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_LOG_TRY_ADVANCE) {
				sctp_misc_ints(SCTP_FWD_TSN_CHECK,
					       0xff, 0xff, cnt_of_space,
					       space_needed);
			}
			cnt_of_skipped = cnt_of_space - sizeof(struct sctp_forward_tsn_chunk);
			cnt_of_skipped /= sizeof(struct sctp_strseq);
			



			at = TAILQ_FIRST(&asoc->sent_queue);
			if (at != NULL) {
				for (i = 0; i < cnt_of_skipped; i++) {
					tp1 = TAILQ_NEXT(at, sctp_next);
					if (tp1 == NULL) {
						break;
					}
					at = tp1;
				}
			}
			if (at && SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_LOG_TRY_ADVANCE) {
				sctp_misc_ints(SCTP_FWD_TSN_CHECK,
					       0xff, cnt_of_skipped, at->rec.data.TSN_seq,
					       asoc->advanced_peer_ack_point);
			}
			last = at;
			



			if (last)
				advance_peer_ack_point = last->rec.data.TSN_seq;
			space_needed = sizeof(struct sctp_forward_tsn_chunk) +
			               cnt_of_skipped * sizeof(struct sctp_strseq);
		}
		chk->send_size = space_needed;
		
		fwdtsn = mtod(chk->data, struct sctp_forward_tsn_chunk *);
		fwdtsn->ch.chunk_length = htons(chk->send_size);
		fwdtsn->ch.chunk_flags = 0;
		fwdtsn->ch.chunk_type = SCTP_FORWARD_CUM_TSN;
		fwdtsn->new_cumulative_tsn = htonl(advance_peer_ack_point);
		SCTP_BUF_LEN(chk->data) = chk->send_size;
		fwdtsn++;
		



		strseq = (struct sctp_strseq *)fwdtsn;
		











		at = TAILQ_FIRST(&asoc->sent_queue);
		for (i = 0; i < cnt_of_skipped; i++) {
			tp1 = TAILQ_NEXT(at, sctp_next);
			if (tp1 == NULL)
				break;
			if (at->rec.data.rcv_flags & SCTP_DATA_UNORDERED) {
				
				i--;
				at = tp1;
				continue;
			}
			if (at->rec.data.TSN_seq == advance_peer_ack_point) {
				at->rec.data.fwd_tsn_cnt = 0;
			}
			strseq->stream = ntohs(at->rec.data.stream_number);
			strseq->sequence = ntohs(at->rec.data.stream_seq);
			strseq++;
			at = tp1;
		}
	}
	return;
}

void
sctp_send_sack(struct sctp_tcb *stcb, int so_locked
#if !defined(__APPLE__) && !defined(SCTP_SO_LOCK_TESTING)
	SCTP_UNUSED
#endif
)
{
	





	struct sctp_association *asoc;
	struct sctp_tmit_chunk *chk, *a_chk;
	struct sctp_sack_chunk *sack;
	struct sctp_nr_sack_chunk *nr_sack;
	struct sctp_gap_ack_block *gap_descriptor;
	struct sack_track *selector;
	int mergeable = 0;
	int offset;
	caddr_t limit;
	uint32_t *dup;
	int limit_reached = 0;
	unsigned int i, siz, j;
	unsigned int num_gap_blocks = 0, num_nr_gap_blocks = 0, space;
	int num_dups = 0;
	int space_req;
	uint32_t highest_tsn;
	uint8_t flags;
	uint8_t type;
	uint8_t tsn_map;

	if ((stcb->asoc.sctp_nr_sack_on_off == 1) &&
	    (stcb->asoc.peer_supports_nr_sack == 1)) {
		type = SCTP_NR_SELECTIVE_ACK;
	} else {
		type = SCTP_SELECTIVE_ACK;
	}
	a_chk = NULL;
	asoc = &stcb->asoc;
	SCTP_TCB_LOCK_ASSERT(stcb);
	if (asoc->last_data_chunk_from == NULL) {
		
		return;
	}
	sctp_slide_mapping_arrays(stcb);
	sctp_set_rwnd(stcb, asoc);
	TAILQ_FOREACH(chk, &asoc->control_send_queue, sctp_next) {
		if (chk->rec.chunk_id.id == type) {
			
			TAILQ_REMOVE(&asoc->control_send_queue, chk, sctp_next);
			asoc->ctrl_queue_cnt--;
			a_chk = chk;
			if (a_chk->data) {
				sctp_m_freem(a_chk->data);
				a_chk->data = NULL;
			}
			if (a_chk->whoTo) {
				sctp_free_remote_addr(a_chk->whoTo);
				a_chk->whoTo = NULL;
			}
			break;
		}
	}
	if (a_chk == NULL) {
		sctp_alloc_a_chunk(stcb, a_chk);
		if (a_chk == NULL) {
			
			if (stcb->asoc.delayed_ack) {
				sctp_timer_stop(SCTP_TIMER_TYPE_RECV,
				    stcb->sctp_ep, stcb, NULL, SCTP_FROM_SCTP_OUTPUT + SCTP_LOC_5);
				sctp_timer_start(SCTP_TIMER_TYPE_RECV,
				    stcb->sctp_ep, stcb, NULL);
			} else {
				stcb->asoc.send_sack = 1;
			}
			return;
		}
		a_chk->copy_by_ref = 0;
		a_chk->rec.chunk_id.id = type;
		a_chk->rec.chunk_id.can_take_data = 1;
	}
	
	asoc->data_pkts_seen = 0;

	a_chk->asoc = asoc;
	a_chk->snd_count = 0;
	a_chk->send_size = 0;	
	a_chk->sent = SCTP_DATAGRAM_UNSENT;
	a_chk->whoTo = NULL;

	if ((asoc->numduptsns) ||
	    (!(asoc->last_data_chunk_from->dest_state & SCTP_ADDR_REACHABLE))) {
		




		if ((asoc->last_data_chunk_from->dest_state & SCTP_ADDR_REACHABLE) &&
		    (asoc->used_alt_onsack > asoc->numnets)) {
			
			a_chk->whoTo = NULL;
		} else {
			asoc->used_alt_onsack++;
			a_chk->whoTo = sctp_find_alternate_net(stcb, asoc->last_data_chunk_from, 0);
		}
		if (a_chk->whoTo == NULL) {
			
			a_chk->whoTo = asoc->last_data_chunk_from;
			asoc->used_alt_onsack = 0;
		}
	} else {
		



		asoc->used_alt_onsack = 0;
		a_chk->whoTo = asoc->last_data_chunk_from;
	}
	if (a_chk->whoTo) {
		atomic_add_int(&a_chk->whoTo->ref_count, 1);
	}
	if (SCTP_TSN_GT(asoc->highest_tsn_inside_map, asoc->highest_tsn_inside_nr_map)) {
		highest_tsn = asoc->highest_tsn_inside_map;
	} else {
		highest_tsn = asoc->highest_tsn_inside_nr_map;
	}
	if (highest_tsn == asoc->cumulative_tsn) {
		
		if (type == SCTP_SELECTIVE_ACK) {
			space_req = sizeof(struct sctp_sack_chunk);
		} else {
			space_req = sizeof(struct sctp_nr_sack_chunk);
		}
	} else {
		
		space_req = MCLBYTES;
	}
	
	a_chk->data = sctp_get_mbuf_for_msg(space_req, 0, M_NOWAIT, 1, MT_DATA);
	if ((a_chk->data == NULL) ||
	    (a_chk->whoTo == NULL)) {
		
		if (a_chk->data) {
			
			sctp_m_freem(a_chk->data);
			a_chk->data = NULL;
		}
		sctp_free_a_chunk(stcb, a_chk, so_locked);
		
		if (stcb->asoc.delayed_ack) {
			sctp_timer_stop(SCTP_TIMER_TYPE_RECV,
			    stcb->sctp_ep, stcb, NULL, SCTP_FROM_SCTP_OUTPUT + SCTP_LOC_6);
			sctp_timer_start(SCTP_TIMER_TYPE_RECV,
			    stcb->sctp_ep, stcb, NULL);
		} else {
			stcb->asoc.send_sack = 1;
		}
		return;
	}
	
	SCTP_BUF_RESV_UF(a_chk->data, SCTP_MIN_OVERHEAD);
	space = M_TRAILINGSPACE(a_chk->data);
	if (space > (a_chk->whoTo->mtu - SCTP_MIN_OVERHEAD)) {
		space = (a_chk->whoTo->mtu - SCTP_MIN_OVERHEAD);
	}
	limit = mtod(a_chk->data, caddr_t);
	limit += space;

	flags = 0;

	if ((asoc->sctp_cmt_on_off > 0) &&
	    SCTP_BASE_SYSCTL(sctp_cmt_use_dac)) {
		




		flags |= (asoc->cmt_dac_pkts_rcvd << 6);
		asoc->cmt_dac_pkts_rcvd = 0;
	}
#ifdef SCTP_ASOCLOG_OF_TSNS
	stcb->asoc.cumack_logsnt[stcb->asoc.cumack_log_atsnt] = asoc->cumulative_tsn;
	stcb->asoc.cumack_log_atsnt++;
	if (stcb->asoc.cumack_log_atsnt >= SCTP_TSN_LOG_SIZE) {
		stcb->asoc.cumack_log_atsnt = 0;
	}
#endif
	
	stcb->freed_by_sorcv_sincelast = 0;

	if (type == SCTP_SELECTIVE_ACK) {
		sack = mtod(a_chk->data, struct sctp_sack_chunk *);
		nr_sack = NULL;
		gap_descriptor = (struct sctp_gap_ack_block *)((caddr_t)sack + sizeof(struct sctp_sack_chunk));
		if (highest_tsn > asoc->mapping_array_base_tsn) {
			siz = (((highest_tsn - asoc->mapping_array_base_tsn) + 1) + 7) / 8;
		} else {
			siz = (((MAX_TSN - highest_tsn) + 1) + highest_tsn + 7) / 8;
		}
	} else {
		sack = NULL;
		nr_sack = mtod(a_chk->data, struct sctp_nr_sack_chunk *);
		gap_descriptor = (struct sctp_gap_ack_block *)((caddr_t)nr_sack + sizeof(struct sctp_nr_sack_chunk));
		if (asoc->highest_tsn_inside_map > asoc->mapping_array_base_tsn) {
			siz = (((asoc->highest_tsn_inside_map - asoc->mapping_array_base_tsn) + 1) + 7) / 8;
		} else {
			siz = (((MAX_TSN - asoc->mapping_array_base_tsn) + 1) + asoc->highest_tsn_inside_map + 7) / 8;
		}
	}

	if (SCTP_TSN_GT(asoc->mapping_array_base_tsn, asoc->cumulative_tsn)) {
		offset = 1;
	} else {
		offset = asoc->mapping_array_base_tsn - asoc->cumulative_tsn;
	}
	if (((type == SCTP_SELECTIVE_ACK) &&
	     SCTP_TSN_GT(highest_tsn, asoc->cumulative_tsn)) ||
	    ((type == SCTP_NR_SELECTIVE_ACK) &&
	     SCTP_TSN_GT(asoc->highest_tsn_inside_map, asoc->cumulative_tsn))) {
		
		for (i = 0; i < siz; i++) {
			tsn_map = asoc->mapping_array[i];
			if (type == SCTP_SELECTIVE_ACK) {
				tsn_map |= asoc->nr_mapping_array[i];
			}
			if (i == 0) {
				



				tsn_map &= (~0 << (1 - offset));
			}
			selector = &sack_array[tsn_map];
			if (mergeable && selector->right_edge) {
				



				num_gap_blocks--;
				gap_descriptor--;
			}
			if (selector->num_entries == 0)
				mergeable = 0;
			else {
				for (j = 0; j < selector->num_entries; j++) {
					if (mergeable && selector->right_edge) {
						



						mergeable = 0;
					} else {
						



						mergeable = 0;
						gap_descriptor->start = htons((selector->gaps[j].start + offset));
					}
					gap_descriptor->end = htons((selector->gaps[j].end + offset));
					num_gap_blocks++;
					gap_descriptor++;
					if (((caddr_t)gap_descriptor + sizeof(struct sctp_gap_ack_block)) > limit) {
						
						limit_reached = 1;
						break;
					}
				}
				if (selector->left_edge) {
					mergeable = 1;
				}
			}
			if (limit_reached) {
				
				break;
			}
			offset += 8;
		}
	}
	if ((type == SCTP_NR_SELECTIVE_ACK) &&
	    (limit_reached == 0)) {

		mergeable = 0;

		if (asoc->highest_tsn_inside_nr_map > asoc->mapping_array_base_tsn) {
			siz = (((asoc->highest_tsn_inside_nr_map - asoc->mapping_array_base_tsn) + 1) + 7) / 8;
		} else {
			siz = (((MAX_TSN - asoc->mapping_array_base_tsn) + 1) + asoc->highest_tsn_inside_nr_map + 7) / 8;
		}

		if (SCTP_TSN_GT(asoc->mapping_array_base_tsn, asoc->cumulative_tsn)) {
			offset = 1;
		} else {
			offset = asoc->mapping_array_base_tsn - asoc->cumulative_tsn;
		}
		if (SCTP_TSN_GT(asoc->highest_tsn_inside_nr_map, asoc->cumulative_tsn)) {
			
			for (i = 0; i < siz; i++) {
				tsn_map = asoc->nr_mapping_array[i];
				if (i == 0) {
					



					tsn_map &= (~0 << (1 - offset));
				}
				selector = &sack_array[tsn_map];
				if (mergeable && selector->right_edge) {
					



					num_nr_gap_blocks--;
					gap_descriptor--;
				}
				if (selector->num_entries == 0)
					mergeable = 0;
				else {
					for (j = 0; j < selector->num_entries; j++) {
						if (mergeable && selector->right_edge) {
							



							mergeable = 0;
						} else {
							



							mergeable = 0;
							gap_descriptor->start = htons((selector->gaps[j].start + offset));
						}
						gap_descriptor->end = htons((selector->gaps[j].end + offset));
						num_nr_gap_blocks++;
						gap_descriptor++;
						if (((caddr_t)gap_descriptor + sizeof(struct sctp_gap_ack_block)) > limit) {
							
							limit_reached = 1;
							break;
						}
					}
					if (selector->left_edge) {
						mergeable = 1;
					}
				}
				if (limit_reached) {
					
					break;
				}
				offset += 8;
			}
		}
	}
	
	if ((limit_reached == 0) && (asoc->numduptsns)) {
		dup = (uint32_t *) gap_descriptor;
		for (i = 0; i < asoc->numduptsns; i++) {
			*dup = htonl(asoc->dup_tsns[i]);
			dup++;
			num_dups++;
			if (((caddr_t)dup + sizeof(uint32_t)) > limit) {
				
				break;
			}
		}
		asoc->numduptsns = 0;
	}
	



	if (type == SCTP_SELECTIVE_ACK) {
		a_chk->send_size = sizeof(struct sctp_sack_chunk) +
		                   (num_gap_blocks + num_nr_gap_blocks) * sizeof(struct sctp_gap_ack_block) +
		                   num_dups * sizeof(int32_t);
		SCTP_BUF_LEN(a_chk->data) = a_chk->send_size;
		sack->sack.cum_tsn_ack = htonl(asoc->cumulative_tsn);
		sack->sack.a_rwnd = htonl(asoc->my_rwnd);
		sack->sack.num_gap_ack_blks = htons(num_gap_blocks);
		sack->sack.num_dup_tsns = htons(num_dups);
		sack->ch.chunk_type = type;
		sack->ch.chunk_flags = flags;
		sack->ch.chunk_length = htons(a_chk->send_size);
	} else {
		a_chk->send_size = sizeof(struct sctp_nr_sack_chunk) +
		                   (num_gap_blocks + num_nr_gap_blocks) * sizeof(struct sctp_gap_ack_block) +
		                   num_dups * sizeof(int32_t);
		SCTP_BUF_LEN(a_chk->data) = a_chk->send_size;
		nr_sack->nr_sack.cum_tsn_ack = htonl(asoc->cumulative_tsn);
		nr_sack->nr_sack.a_rwnd = htonl(asoc->my_rwnd);
		nr_sack->nr_sack.num_gap_ack_blks = htons(num_gap_blocks);
		nr_sack->nr_sack.num_nr_gap_ack_blks = htons(num_nr_gap_blocks);
		nr_sack->nr_sack.num_dup_tsns = htons(num_dups);
		nr_sack->nr_sack.reserved = 0;
		nr_sack->ch.chunk_type = type;
		nr_sack->ch.chunk_flags = flags;
		nr_sack->ch.chunk_length = htons(a_chk->send_size);
	}
	TAILQ_INSERT_TAIL(&asoc->control_send_queue, a_chk, sctp_next);
	asoc->my_last_reported_rwnd = asoc->my_rwnd;
	asoc->ctrl_queue_cnt++;
	asoc->send_sack = 0;
	SCTP_STAT_INCR(sctps_sendsacks);
	return;
}

void
sctp_send_abort_tcb(struct sctp_tcb *stcb, struct mbuf *operr, int so_locked
#if !defined(__APPLE__) && !defined(SCTP_SO_LOCK_TESTING)
    SCTP_UNUSED
#endif
    )
{
	struct mbuf *m_abort, *m, *m_last;
	struct mbuf *m_out, *m_end = NULL;
	struct sctp_abort_chunk *abort;
	struct sctp_auth_chunk *auth = NULL;
	struct sctp_nets *net;
	uint32_t auth_offset = 0;
	uint16_t cause_len, chunk_len, padding_len;

#if defined(__APPLE__)
	if (so_locked) {
		sctp_lock_assert(SCTP_INP_SO(stcb->sctp_ep));
	} else {
		sctp_unlock_assert(SCTP_INP_SO(stcb->sctp_ep));
	}
#endif
	SCTP_TCB_LOCK_ASSERT(stcb);
	



	if (sctp_auth_is_required_chunk(SCTP_ABORT_ASSOCIATION,
	                                stcb->asoc.peer_auth_chunks)) {
		m_out = sctp_add_auth_chunk(NULL, &m_end, &auth, &auth_offset,
					    stcb, SCTP_ABORT_ASSOCIATION);
		SCTP_STAT_INCR_COUNTER64(sctps_outcontrolchunks);
	} else {
		m_out = NULL;
	}
	m_abort = sctp_get_mbuf_for_msg(sizeof(struct sctp_abort_chunk), 0, M_NOWAIT, 1, MT_HEADER);
	if (m_abort == NULL) {
		if (m_out) {
			sctp_m_freem(m_out);
		}
		if (operr) {
			sctp_m_freem(operr);
		}
		return;
	}
	
	SCTP_BUF_NEXT(m_abort) = operr;
	cause_len = 0;
	m_last = NULL;
	for (m = operr; m; m = SCTP_BUF_NEXT(m)) {
		cause_len += (uint16_t)SCTP_BUF_LEN(m);
		if (SCTP_BUF_NEXT(m) == NULL) {
			m_last = m;
		}
	}
	SCTP_BUF_LEN(m_abort) = sizeof(struct sctp_abort_chunk);
	chunk_len = (uint16_t)sizeof(struct sctp_abort_chunk) + cause_len;
	padding_len = SCTP_SIZE32(chunk_len) - chunk_len;
	if (m_out == NULL) {
		
		SCTP_BUF_RESV_UF(m_abort, SCTP_MIN_OVERHEAD);
		m_out = m_abort;
	} else {
		
		SCTP_BUF_NEXT(m_end) = m_abort;
	}
	if (stcb->asoc.alternate) {
		net = stcb->asoc.alternate;
	} else {
		net = stcb->asoc.primary_destination;
	}
	
	abort = mtod(m_abort, struct sctp_abort_chunk *);
	abort->ch.chunk_type = SCTP_ABORT_ASSOCIATION;
	abort->ch.chunk_flags = 0;
	abort->ch.chunk_length = htons(chunk_len);
	
	if (padding_len > 0) {
		if ((m_last == NULL) || sctp_add_pad_tombuf(m_last, padding_len)) {
			sctp_m_freem(m_out);
			return;
		}
	}
	(void)sctp_lowlevel_chunk_output(stcb->sctp_ep, stcb, net,
	                                 (struct sockaddr *)&net->ro._l_addr,
	                                 m_out, auth_offset, auth, stcb->asoc.authinfo.active_keyid, 1, 0, 0,
	                                 stcb->sctp_ep->sctp_lport, stcb->rport, htonl(stcb->asoc.peer_vtag),
	                                 stcb->asoc.primary_destination->port, NULL,
#if defined(__FreeBSD__)
	                                 0, 0,
#endif
	                                 so_locked);
	SCTP_STAT_INCR_COUNTER64(sctps_outcontrolchunks);
}

void
sctp_send_shutdown_complete(struct sctp_tcb *stcb,
                            struct sctp_nets *net,
                            int reflect_vtag)
{
	
	struct mbuf *m_shutdown_comp;
	struct sctp_shutdown_complete_chunk *shutdown_complete;
	uint32_t vtag;
	uint8_t flags;

	m_shutdown_comp = sctp_get_mbuf_for_msg(sizeof(struct sctp_chunkhdr), 0, M_NOWAIT, 1, MT_HEADER);
	if (m_shutdown_comp == NULL) {
		
		return;
	}
	if (reflect_vtag) {
		flags = SCTP_HAD_NO_TCB;
		vtag = stcb->asoc.my_vtag;
	} else {
		flags = 0;
		vtag = stcb->asoc.peer_vtag;
	}
	shutdown_complete = mtod(m_shutdown_comp, struct sctp_shutdown_complete_chunk *);
	shutdown_complete->ch.chunk_type = SCTP_SHUTDOWN_COMPLETE;
	shutdown_complete->ch.chunk_flags = flags;
	shutdown_complete->ch.chunk_length = htons(sizeof(struct sctp_shutdown_complete_chunk));
	SCTP_BUF_LEN(m_shutdown_comp) = sizeof(struct sctp_shutdown_complete_chunk);
	(void)sctp_lowlevel_chunk_output(stcb->sctp_ep, stcb, net,
	                                 (struct sockaddr *)&net->ro._l_addr,
	                                 m_shutdown_comp, 0, NULL, 0, 1, 0, 0,
	                                 stcb->sctp_ep->sctp_lport, stcb->rport,
	                                 htonl(vtag),
	                                 net->port, NULL,
#if defined(__FreeBSD__)
	                                 0, 0,
#endif
	                                 SCTP_SO_NOT_LOCKED);
	SCTP_STAT_INCR_COUNTER64(sctps_outcontrolchunks);
	return;
}

#if defined(__FreeBSD__)
static void
sctp_send_resp_msg(struct sockaddr *src, struct sockaddr *dst,
                   struct sctphdr *sh, uint32_t vtag,
                   uint8_t type, struct mbuf *cause,
                   uint8_t use_mflowid, uint32_t mflowid,
                   uint32_t vrf_id, uint16_t port)
#else
static void
sctp_send_resp_msg(struct sockaddr *src, struct sockaddr *dst,
                   struct sctphdr *sh, uint32_t vtag,
                   uint8_t type, struct mbuf *cause,
                   uint32_t vrf_id SCTP_UNUSED, uint16_t port)
#endif
{
#ifdef __Panda__
	pakhandle_type o_pak;
#else
	struct mbuf *o_pak;
#endif
	struct mbuf *mout;
	struct sctphdr *shout;
	struct sctp_chunkhdr *ch;
	struct udphdr *udp;
	int len, cause_len, padding_len, ret;
#ifdef INET
#if defined(__APPLE__) || defined(__Panda__)
	sctp_route_t ro;
#endif
	struct sockaddr_in *src_sin, *dst_sin;
	struct ip *ip;
#endif
#ifdef INET6
	struct sockaddr_in6 *src_sin6, *dst_sin6;
	struct ip6_hdr *ip6;
#endif

	
	cause_len = 0;
	if (cause != NULL) {
		struct mbuf *m_at, *m_last = NULL;

		for (m_at = cause; m_at; m_at = SCTP_BUF_NEXT(m_at)) {
			if (SCTP_BUF_NEXT(m_at) == NULL)
				m_last = m_at;
			cause_len += SCTP_BUF_LEN(m_at);
		}
		padding_len = cause_len % 4;
		if (padding_len != 0) {
			padding_len = 4 - padding_len;
		}
		if (padding_len != 0) {
			if (sctp_add_pad_tombuf(m_last, padding_len)) {
				sctp_m_freem(cause);
				return;
			}
		}
	} else {
		padding_len = 0;
	}
	
	len = sizeof(struct sctphdr) + sizeof(struct sctp_chunkhdr);
	switch (dst->sa_family) {
#ifdef INET
	case AF_INET:
		len += sizeof(struct ip);
		break;
#endif
#ifdef INET6
	case AF_INET6:
		len += sizeof(struct ip6_hdr);
		break;
#endif
	default:
		break;
	}
	if (port) {
		len += sizeof(struct udphdr);
	}
#if defined(__APPLE__)
#if defined(APPLE_LEOPARD) || defined(APPLE_SNOWLEOPARD)
	mout = sctp_get_mbuf_for_msg(len + max_linkhdr, 1, M_NOWAIT, 1, MT_DATA);
#else
	mout = sctp_get_mbuf_for_msg(len + SCTP_MAX_LINKHDR, 1, M_NOWAIT, 1, MT_DATA);
#endif
#else
	mout = sctp_get_mbuf_for_msg(len + max_linkhdr, 1, M_NOWAIT, 1, MT_DATA);
#endif
	if (mout == NULL) {
		if (cause) {
			sctp_m_freem(cause);
		}
		return;
	}
#if defined(__APPLE__)
#if defined(APPLE_LEOPARD) || defined(APPLE_SNOWLEOPARD)
	SCTP_BUF_RESV_UF(mout, max_linkhdr);
#else
	SCTP_BUF_RESV_UF(mout, SCTP_MAX_LINKHDR);
#endif
#else
	SCTP_BUF_RESV_UF(mout, max_linkhdr);
#endif
	SCTP_BUF_LEN(mout) = len;
	SCTP_BUF_NEXT(mout) = cause;
#if defined(__FreeBSD__)
	if (use_mflowid != 0) {
		mout->m_pkthdr.flowid = mflowid;
		mout->m_flags |= M_FLOWID;
	}
#endif
#ifdef INET
	ip = NULL;
#endif
#ifdef INET6
	ip6 = NULL;
#endif
	switch (dst->sa_family) {
#ifdef INET
	case AF_INET:
		src_sin = (struct sockaddr_in *)src;
		dst_sin = (struct sockaddr_in *)dst;
		ip = mtod(mout, struct ip *);
		ip->ip_v = IPVERSION;
		ip->ip_hl = (sizeof(struct ip) >> 2);
		ip->ip_tos = 0;
#if defined(__FreeBSD__)
		ip->ip_id = ip_newid();
#elif defined(__APPLE__)
#if RANDOM_IP_ID
		ip->ip_id = ip_randomid();
#else
		ip->ip_id = htons(ip_id++);
#endif
#else
                ip->ip_id = htons(ip_id++);
#endif
		ip->ip_off = 0;
		ip->ip_ttl = MODULE_GLOBAL(ip_defttl);
		if (port) {
			ip->ip_p = IPPROTO_UDP;
		} else {
			ip->ip_p = IPPROTO_SCTP;
		}
		ip->ip_src.s_addr = dst_sin->sin_addr.s_addr;
		ip->ip_dst.s_addr = src_sin->sin_addr.s_addr;
		ip->ip_sum = 0;
		len = sizeof(struct ip);
		shout = (struct sctphdr *)((caddr_t)ip + len);
		break;
#endif
#ifdef INET6
	case AF_INET6:
		src_sin6 = (struct sockaddr_in6 *)src;
		dst_sin6 = (struct sockaddr_in6 *)dst;
		ip6 = mtod(mout, struct ip6_hdr *);
		ip6->ip6_flow = htonl(0x60000000);
#if defined(__FreeBSD__)
		if (V_ip6_auto_flowlabel) {
			ip6->ip6_flow |= (htonl(ip6_randomflowlabel()) & IPV6_FLOWLABEL_MASK);
		}
#endif
#if defined(__Userspace__)
		ip6->ip6_hlim = IPv6_HOP_LIMIT;
#else
		ip6->ip6_hlim = MODULE_GLOBAL(ip6_defhlim);
#endif
		if (port) {
			ip6->ip6_nxt = IPPROTO_UDP;
		} else {
			ip6->ip6_nxt = IPPROTO_SCTP;
		}
		ip6->ip6_src = dst_sin6->sin6_addr;
		ip6->ip6_dst = src_sin6->sin6_addr;
		len = sizeof(struct ip6_hdr);
		shout = (struct sctphdr *)((caddr_t)ip6 + len);
		break;
#endif
	default:
		len = 0;
		shout = mtod(mout, struct sctphdr *);
		break;
	}
	if (port) {
		if (htons(SCTP_BASE_SYSCTL(sctp_udp_tunneling_port)) == 0) {
			sctp_m_freem(mout);
			return;
		}
		udp = (struct udphdr *)shout;
		udp->uh_sport = htons(SCTP_BASE_SYSCTL(sctp_udp_tunneling_port));
		udp->uh_dport = port;
		udp->uh_sum = 0;
		udp->uh_ulen = htons(sizeof(struct udphdr) +
		                     sizeof(struct sctphdr) +
		                     sizeof(struct sctp_chunkhdr) +
		                     cause_len + padding_len);
		len += sizeof(struct udphdr);
		shout = (struct sctphdr *)((caddr_t)shout + sizeof(struct udphdr));
	} else {
		udp = NULL;
	}
	shout->src_port = sh->dest_port;
	shout->dest_port = sh->src_port;
	shout->checksum = 0;
	if (vtag) {
		shout->v_tag = htonl(vtag);
	} else {
		shout->v_tag = sh->v_tag;
	}
	len += sizeof(struct sctphdr);
	ch = (struct sctp_chunkhdr *)((caddr_t)shout + sizeof(struct sctphdr));
	ch->chunk_type = type;
	if (vtag) {
		ch->chunk_flags = 0;
	} else {
		ch->chunk_flags = SCTP_HAD_NO_TCB;
	}
	ch->chunk_length = htons(sizeof(struct sctp_chunkhdr) + cause_len);
	len += sizeof(struct sctp_chunkhdr);
	len += cause_len + padding_len;

	if (SCTP_GET_HEADER_FOR_OUTPUT(o_pak)) {
		sctp_m_freem(mout);
		return;
	}
	SCTP_ATTACH_CHAIN(o_pak, mout, len);
	switch (dst->sa_family) {
#ifdef INET
	case AF_INET:
#if defined(__APPLE__) || defined(__Panda__)
		
		bzero(&ro, sizeof(sctp_route_t));
#if defined(__Panda__)
		ro._l_addr.sa.sa_family = AF_INET;
#endif
#endif
		if (port) {
#if !defined(__Windows__) && !defined(__Userspace__)
#if defined(__FreeBSD__) && ((__FreeBSD_version > 803000 && __FreeBSD_version < 900000) || __FreeBSD_version > 900000)
			if (V_udp_cksum) {
				udp->uh_sum = in_pseudo(ip->ip_src.s_addr, ip->ip_dst.s_addr, udp->uh_ulen + htons(IPPROTO_UDP));
			} else {
				udp->uh_sum = 0;
			}
#else
			udp->uh_sum = in_pseudo(ip->ip_src.s_addr, ip->ip_dst.s_addr, udp->uh_ulen + htons(IPPROTO_UDP));
#endif
#else
			udp->uh_sum = 0;
#endif
		}
#if defined(__FreeBSD__)
#if __FreeBSD_version >= 1000000
		ip->ip_len = htons(len);
#else
		ip->ip_len = len;
#endif
#elif defined(__APPLE__) || defined(__Userspace__)
		ip->ip_len = len;
#else
		ip->ip_len = htons(len);
#endif
		if (port) {
#if defined(SCTP_WITH_NO_CSUM)
			SCTP_STAT_INCR(sctps_sendnocrc);
#else
			shout->checksum = sctp_calculate_cksum(mout, sizeof(struct ip) + sizeof(struct udphdr));
			SCTP_STAT_INCR(sctps_sendswcrc);
#endif
#if defined(__FreeBSD__) && ((__FreeBSD_version > 803000 && __FreeBSD_version < 900000) || __FreeBSD_version > 900000)
			if (V_udp_cksum) {
				SCTP_ENABLE_UDP_CSUM(o_pak);
			}
#else
			SCTP_ENABLE_UDP_CSUM(o_pak);
#endif
		} else {
#if defined(SCTP_WITH_NO_CSUM)
			SCTP_STAT_INCR(sctps_sendnocrc);
#else
#if defined(__FreeBSD__) && __FreeBSD_version >= 800000
			mout->m_pkthdr.csum_flags = CSUM_SCTP;
			mout->m_pkthdr.csum_data = 0;
			SCTP_STAT_INCR(sctps_sendhwcrc);
#else
			shout->checksum = sctp_calculate_cksum(mout, sizeof(struct ip));
			SCTP_STAT_INCR(sctps_sendswcrc);
#endif
#endif
		}
#ifdef SCTP_PACKET_LOGGING
		if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_LAST_PACKET_TRACING) {
			sctp_packet_log(o_pak);
		}
#endif
#if defined(__APPLE__) || defined(__Panda__)
		SCTP_IP_OUTPUT(ret, o_pak, &ro, NULL, vrf_id);
		
		if (ro.ro_rt) {
			RTFREE(ro.ro_rt);
		}
#else
		SCTP_IP_OUTPUT(ret, o_pak, NULL, NULL, vrf_id);
#endif
		break;
#endif
#ifdef INET6
	case AF_INET6:
		ip6->ip6_plen = len - sizeof(struct ip6_hdr);
		if (port) {
#if defined(SCTP_WITH_NO_CSUM)
			SCTP_STAT_INCR(sctps_sendnocrc);
#else
			shout->checksum = sctp_calculate_cksum(mout, sizeof(struct ip6_hdr) + sizeof(struct udphdr));
			SCTP_STAT_INCR(sctps_sendswcrc);
#endif
#if defined(__Windows__)
			udp->uh_sum = 0;
#elif !defined(__Userspace__)
			if ((udp->uh_sum = in6_cksum(o_pak, IPPROTO_UDP, sizeof(struct ip6_hdr), len - sizeof(struct ip6_hdr))) == 0) {
				udp->uh_sum = 0xffff;
			}
#endif
		} else {
#if defined(SCTP_WITH_NO_CSUM)
			SCTP_STAT_INCR(sctps_sendnocrc);
#else
#if defined(__FreeBSD__) && __FreeBSD_version >= 900000
#if __FreeBSD_version > 901000
			mout->m_pkthdr.csum_flags = CSUM_SCTP_IPV6;
#else
			mout->m_pkthdr.csum_flags = CSUM_SCTP;
#endif
			mout->m_pkthdr.csum_data = 0;
			SCTP_STAT_INCR(sctps_sendhwcrc);
#else
			shout->checksum = sctp_calculate_cksum(mout, sizeof(struct ip6_hdr));
			SCTP_STAT_INCR(sctps_sendswcrc);
#endif
#endif
		}
#ifdef SCTP_PACKET_LOGGING
		if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_LAST_PACKET_TRACING) {
			sctp_packet_log(o_pak);
		}
#endif
		SCTP_IP6_OUTPUT(ret, o_pak, NULL, NULL, NULL, vrf_id);
		break;
#endif
#if defined(__Userspace__)
	case AF_CONN:
	{
		char *buffer;
		struct sockaddr_conn *sconn;

		sconn = (struct sockaddr_conn *)src;
		shout->checksum = sctp_calculate_cksum(mout, 0);
		SCTP_STAT_INCR(sctps_sendswcrc);
#ifdef SCTP_PACKET_LOGGING
		if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_LAST_PACKET_TRACING) {
			sctp_packet_log(mout);
		}
#endif
		
		if ((buffer = malloc(len)) != NULL) {
			m_copydata(mout, 0, len, buffer);
			ret = SCTP_BASE_VAR(conn_output)(sconn->sconn_addr, buffer, len, 0, 0);
			free(buffer);
		}
		sctp_m_freem(mout);
		break;
	}
#endif
	default:
		SCTPDBG(SCTP_DEBUG_OUTPUT1, "Unknown protocol (TSNH) type %d\n",
		        dst->sa_family);
		sctp_m_freem(mout);
		SCTP_LTRACE_ERR_RET_PKT(mout, NULL, NULL, NULL, SCTP_FROM_SCTP_OUTPUT, EFAULT);
		return;
	}
	SCTP_STAT_INCR(sctps_sendpackets);
	SCTP_STAT_INCR_COUNTER64(sctps_outpackets);
	SCTP_STAT_INCR_COUNTER64(sctps_outcontrolchunks);
	return;
}

void
sctp_send_shutdown_complete2(struct sockaddr *src, struct sockaddr *dst,
                             struct sctphdr *sh,
#if defined(__FreeBSD__)
                             uint8_t use_mflowid, uint32_t mflowid,
#endif
                             uint32_t vrf_id, uint16_t port)
{
	sctp_send_resp_msg(src, dst, sh, 0, SCTP_SHUTDOWN_COMPLETE, NULL,
#if defined(__FreeBSD__)
	                   use_mflowid, mflowid,
#endif
	                   vrf_id, port);
}

void
sctp_send_hb(struct sctp_tcb *stcb, struct sctp_nets *net,int so_locked
#if !defined(__APPLE__) && !defined(SCTP_SO_LOCK_TESTING)
	SCTP_UNUSED
#endif
)
{
	struct sctp_tmit_chunk *chk;
	struct sctp_heartbeat_chunk *hb;
	struct timeval now;

	SCTP_TCB_LOCK_ASSERT(stcb);
	if (net == NULL) {
		return;
	}
	(void)SCTP_GETTIME_TIMEVAL(&now);
	switch (net->ro._l_addr.sa.sa_family) {
#ifdef INET
	case AF_INET:
		break;
#endif
#ifdef INET6
	case AF_INET6:
		break;
#endif
#if defined(__Userspace__)
	case AF_CONN:
		break;
#endif
	default:
		return;
	}
	sctp_alloc_a_chunk(stcb, chk);
	if (chk == NULL) {
		SCTPDBG(SCTP_DEBUG_OUTPUT4, "Gak, can't get a chunk for hb\n");
		return;
	}

	chk->copy_by_ref = 0;
	chk->rec.chunk_id.id = SCTP_HEARTBEAT_REQUEST;
	chk->rec.chunk_id.can_take_data = 1;
	chk->asoc = &stcb->asoc;
	chk->send_size = sizeof(struct sctp_heartbeat_chunk);

	chk->data = sctp_get_mbuf_for_msg(chk->send_size, 0, M_NOWAIT, 1, MT_HEADER);
	if (chk->data == NULL) {
		sctp_free_a_chunk(stcb, chk, so_locked);
		return;
	}
	SCTP_BUF_RESV_UF(chk->data, SCTP_MIN_OVERHEAD);
	SCTP_BUF_LEN(chk->data) = chk->send_size;
	chk->sent = SCTP_DATAGRAM_UNSENT;
	chk->snd_count = 0;
	chk->whoTo = net;
	atomic_add_int(&chk->whoTo->ref_count, 1);
	
	hb = mtod(chk->data, struct sctp_heartbeat_chunk *);
	memset(hb, 0, sizeof(struct sctp_heartbeat_chunk));
	
	hb->ch.chunk_type = SCTP_HEARTBEAT_REQUEST;
	hb->ch.chunk_flags = 0;
	hb->ch.chunk_length = htons(chk->send_size);
	
	hb->heartbeat.hb_info.ph.param_type = htons(SCTP_HEARTBEAT_INFO);
	hb->heartbeat.hb_info.ph.param_length = htons(sizeof(struct sctp_heartbeat_info_param));
	hb->heartbeat.hb_info.time_value_1 = now.tv_sec;
	hb->heartbeat.hb_info.time_value_2 = now.tv_usec;
	
	hb->heartbeat.hb_info.addr_family = net->ro._l_addr.sa.sa_family;
#ifdef HAVE_SA_LEN
	hb->heartbeat.hb_info.addr_len = net->ro._l_addr.sa.sa_len;
#else
	switch (net->ro._l_addr.sa.sa_family) {
#ifdef INET
	case AF_INET:
		hb->heartbeat.hb_info.addr_len = sizeof(struct sockaddr_in);
		break;
#endif
#ifdef INET6
	case AF_INET6:
		hb->heartbeat.hb_info.addr_len = sizeof(struct sockaddr_in6);
		break;
#endif
#if defined(__Userspace__)
	case AF_CONN:
		hb->heartbeat.hb_info.addr_len = sizeof(struct sockaddr_conn);
		break;
#endif
	default:
		hb->heartbeat.hb_info.addr_len = 0;
		break;
	}
#endif
	if (net->dest_state & SCTP_ADDR_UNCONFIRMED) {
		



		net->heartbeat_random1 = hb->heartbeat.hb_info.random_value1 = sctp_select_initial_TSN(&stcb->sctp_ep->sctp_ep);
		net->heartbeat_random2 = hb->heartbeat.hb_info.random_value2 = sctp_select_initial_TSN(&stcb->sctp_ep->sctp_ep);
	} else {
		net->heartbeat_random1 = hb->heartbeat.hb_info.random_value1 = 0;
		net->heartbeat_random2 = hb->heartbeat.hb_info.random_value2 = 0;
	}
	switch (net->ro._l_addr.sa.sa_family) {
#ifdef INET
	case AF_INET:
		memcpy(hb->heartbeat.hb_info.address,
		       &net->ro._l_addr.sin.sin_addr,
		       sizeof(net->ro._l_addr.sin.sin_addr));
		break;
#endif
#ifdef INET6
	case AF_INET6:
		memcpy(hb->heartbeat.hb_info.address,
		       &net->ro._l_addr.sin6.sin6_addr,
		       sizeof(net->ro._l_addr.sin6.sin6_addr));
		break;
#endif
#if defined(__Userspace__)
	case AF_CONN:
		memcpy(hb->heartbeat.hb_info.address,
		       &net->ro._l_addr.sconn.sconn_addr,
		       sizeof(net->ro._l_addr.sconn.sconn_addr));
		break;
#endif
	default:
		return;
		break;
	}
	net->hb_responded = 0;
	TAILQ_INSERT_TAIL(&stcb->asoc.control_send_queue, chk, sctp_next);
	stcb->asoc.ctrl_queue_cnt++;
	SCTP_STAT_INCR(sctps_sendheartbeat);
	return;
}

void
sctp_send_ecn_echo(struct sctp_tcb *stcb, struct sctp_nets *net,
		   uint32_t high_tsn)
{
	struct sctp_association *asoc;
	struct sctp_ecne_chunk *ecne;
	struct sctp_tmit_chunk *chk;

	if (net == NULL) {
		return;
	}
	asoc = &stcb->asoc;
	SCTP_TCB_LOCK_ASSERT(stcb);
	TAILQ_FOREACH(chk, &asoc->control_send_queue, sctp_next) {
		if ((chk->rec.chunk_id.id == SCTP_ECN_ECHO) && (net == chk->whoTo)) {
			
			uint32_t cnt, ctsn;
			ecne = mtod(chk->data, struct sctp_ecne_chunk *);
			ctsn = ntohl(ecne->tsn);
			if (SCTP_TSN_GT(high_tsn, ctsn)) {
				ecne->tsn = htonl(high_tsn);
				SCTP_STAT_INCR(sctps_queue_upd_ecne);
			}
			cnt = ntohl(ecne->num_pkts_since_cwr);
			cnt++;
			ecne->num_pkts_since_cwr = htonl(cnt);
			return;
		}
	}
	
	sctp_alloc_a_chunk(stcb, chk);
	if (chk == NULL) {
		return;
	}
	chk->copy_by_ref = 0;
	SCTP_STAT_INCR(sctps_queue_upd_ecne);
	chk->rec.chunk_id.id = SCTP_ECN_ECHO;
	chk->rec.chunk_id.can_take_data = 0;
	chk->asoc = &stcb->asoc;
	chk->send_size = sizeof(struct sctp_ecne_chunk);
	chk->data = sctp_get_mbuf_for_msg(chk->send_size, 0, M_NOWAIT, 1, MT_HEADER);
	if (chk->data == NULL) {
		sctp_free_a_chunk(stcb, chk, SCTP_SO_NOT_LOCKED);
		return;
	}
	SCTP_BUF_RESV_UF(chk->data, SCTP_MIN_OVERHEAD);
	SCTP_BUF_LEN(chk->data) = chk->send_size;
	chk->sent = SCTP_DATAGRAM_UNSENT;
	chk->snd_count = 0;
	chk->whoTo = net;
	atomic_add_int(&chk->whoTo->ref_count, 1);

	stcb->asoc.ecn_echo_cnt_onq++;
	ecne = mtod(chk->data, struct sctp_ecne_chunk *);
	ecne->ch.chunk_type = SCTP_ECN_ECHO;
	ecne->ch.chunk_flags = 0;
	ecne->ch.chunk_length = htons(sizeof(struct sctp_ecne_chunk));
	ecne->tsn = htonl(high_tsn);
	ecne->num_pkts_since_cwr = htonl(1);
	TAILQ_INSERT_HEAD(&stcb->asoc.control_send_queue, chk, sctp_next);
	asoc->ctrl_queue_cnt++;
}

void
sctp_send_packet_dropped(struct sctp_tcb *stcb, struct sctp_nets *net,
    struct mbuf *m, int len, int iphlen, int bad_crc)
{
	struct sctp_association *asoc;
	struct sctp_pktdrop_chunk *drp;
	struct sctp_tmit_chunk *chk;
	uint8_t *datap;
	int was_trunc = 0;
	int fullsz = 0;
	long spc;
	int offset;
	struct sctp_chunkhdr *ch, chunk_buf;
	unsigned int chk_length;

        if (!stcb) {
            return;
        }
	asoc = &stcb->asoc;
	SCTP_TCB_LOCK_ASSERT(stcb);
	if (asoc->peer_supports_pktdrop == 0) {
		


		return;
	}
	if (stcb->sctp_socket == NULL) {
		return;
	}
	sctp_alloc_a_chunk(stcb, chk);
	if (chk == NULL) {
		return;
	}
	chk->copy_by_ref = 0;
	len -= iphlen;
	chk->send_size = len;
        
	offset = iphlen + sizeof(struct sctphdr);
	ch = (struct sctp_chunkhdr *)sctp_m_getptr(m, offset,
						   sizeof(*ch), (uint8_t *) & chunk_buf);
	while (ch != NULL) {
		chk_length = ntohs(ch->chunk_length);
		if (chk_length < sizeof(*ch)) {
			
			break;
		}
		switch (ch->chunk_type) {
		case SCTP_PACKET_DROPPED:
		case SCTP_ABORT_ASSOCIATION:
		case SCTP_INITIATION_ACK:
			





			sctp_free_a_chunk(stcb, chk, SCTP_SO_NOT_LOCKED);
			return;
		default:
			break;
		}
		offset += SCTP_SIZE32(chk_length);
		ch = (struct sctp_chunkhdr *)sctp_m_getptr(m, offset,
		    sizeof(*ch), (uint8_t *) & chunk_buf);
	}

	if ((len + SCTP_MAX_OVERHEAD + sizeof(struct sctp_pktdrop_chunk)) >
	    min(stcb->asoc.smallest_mtu, MCLBYTES)) {
		


		fullsz = len;
		len = min(stcb->asoc.smallest_mtu, MCLBYTES) - SCTP_MAX_OVERHEAD;
		was_trunc = 1;
	}
	chk->asoc = &stcb->asoc;
	chk->data = sctp_get_mbuf_for_msg(MCLBYTES, 0, M_NOWAIT, 1, MT_DATA);
	if (chk->data == NULL) {
jump_out:
		sctp_free_a_chunk(stcb, chk, SCTP_SO_NOT_LOCKED);
		return;
	}
	SCTP_BUF_RESV_UF(chk->data, SCTP_MIN_OVERHEAD);
	drp = mtod(chk->data, struct sctp_pktdrop_chunk *);
	if (drp == NULL) {
		sctp_m_freem(chk->data);
		chk->data = NULL;
		goto jump_out;
	}
	chk->book_size = SCTP_SIZE32((chk->send_size + sizeof(struct sctp_pktdrop_chunk) +
	    sizeof(struct sctphdr) + SCTP_MED_OVERHEAD));
	chk->book_size_scale = 0;
	if (was_trunc) {
		drp->ch.chunk_flags = SCTP_PACKET_TRUNCATED;
		drp->trunc_len = htons(fullsz);
		


		chk->send_size = len - sizeof(struct sctp_pktdrop_chunk);
		len = chk->send_size;
	} else {
		
		drp->ch.chunk_flags = 0;
		drp->trunc_len = htons(0);
	}
	if (bad_crc) {
		drp->ch.chunk_flags |= SCTP_BADCRC;
	}
	chk->send_size += sizeof(struct sctp_pktdrop_chunk);
	SCTP_BUF_LEN(chk->data) = chk->send_size;
	chk->sent = SCTP_DATAGRAM_UNSENT;
	chk->snd_count = 0;
	if (net) {
		
		chk->whoTo = net;
		atomic_add_int(&chk->whoTo->ref_count, 1);
	} else {
		chk->whoTo = NULL;
	}
	chk->rec.chunk_id.id = SCTP_PACKET_DROPPED;
	chk->rec.chunk_id.can_take_data = 1;
	drp->ch.chunk_type = SCTP_PACKET_DROPPED;
	drp->ch.chunk_length = htons(chk->send_size);
	spc = SCTP_SB_LIMIT_RCV(stcb->sctp_socket);
	if (spc < 0) {
		spc = 0;
	}
	drp->bottle_bw = htonl(spc);
	if (asoc->my_rwnd) {
		drp->current_onq = htonl(asoc->size_on_reasm_queue +
		    asoc->size_on_all_streams +
		    asoc->my_rwnd_control_len +
		    stcb->sctp_socket->so_rcv.sb_cc);
	} else {
		



		drp->current_onq = htonl(spc);
	}
	drp->reserved = 0;
	datap = drp->data;
	m_copydata(m, iphlen, len, (caddr_t)datap);
	TAILQ_INSERT_TAIL(&stcb->asoc.control_send_queue, chk, sctp_next);
	asoc->ctrl_queue_cnt++;
}

void
sctp_send_cwr(struct sctp_tcb *stcb, struct sctp_nets *net, uint32_t high_tsn, uint8_t override)
{
	struct sctp_association *asoc;
	struct sctp_cwr_chunk *cwr;
	struct sctp_tmit_chunk *chk;

	SCTP_TCB_LOCK_ASSERT(stcb);
	if (net == NULL) {
		return;
	}
	asoc = &stcb->asoc;
	TAILQ_FOREACH(chk, &asoc->control_send_queue, sctp_next) {
		if ((chk->rec.chunk_id.id == SCTP_ECN_CWR) && (net == chk->whoTo)) {
			
			uint32_t ctsn;
			cwr = mtod(chk->data, struct sctp_cwr_chunk *);
			ctsn = ntohl(cwr->tsn);
			if (SCTP_TSN_GT(high_tsn, ctsn)) {
				cwr->tsn = htonl(high_tsn);
			}
			if (override & SCTP_CWR_REDUCE_OVERRIDE) {
				
				cwr->ch.chunk_flags |= SCTP_CWR_REDUCE_OVERRIDE;
			}
			return;
		}
	}
	sctp_alloc_a_chunk(stcb, chk);
	if (chk == NULL) {
		return;
	}
	chk->copy_by_ref = 0;
	chk->rec.chunk_id.id = SCTP_ECN_CWR;
	chk->rec.chunk_id.can_take_data = 1;
	chk->asoc = &stcb->asoc;
	chk->send_size = sizeof(struct sctp_cwr_chunk);
	chk->data = sctp_get_mbuf_for_msg(chk->send_size, 0, M_NOWAIT, 1, MT_HEADER);
	if (chk->data == NULL) {
		sctp_free_a_chunk(stcb, chk, SCTP_SO_NOT_LOCKED);
		return;
	}
	SCTP_BUF_RESV_UF(chk->data, SCTP_MIN_OVERHEAD);
	SCTP_BUF_LEN(chk->data) = chk->send_size;
	chk->sent = SCTP_DATAGRAM_UNSENT;
	chk->snd_count = 0;
	chk->whoTo = net;
	atomic_add_int(&chk->whoTo->ref_count, 1);
	cwr = mtod(chk->data, struct sctp_cwr_chunk *);
	cwr->ch.chunk_type = SCTP_ECN_CWR;
	cwr->ch.chunk_flags = override;
	cwr->ch.chunk_length = htons(sizeof(struct sctp_cwr_chunk));
	cwr->tsn = htonl(high_tsn);
	TAILQ_INSERT_TAIL(&stcb->asoc.control_send_queue, chk, sctp_next);
	asoc->ctrl_queue_cnt++;
}

void
sctp_add_stream_reset_out(struct sctp_tmit_chunk *chk,
                          int number_entries, uint16_t * list,
                          uint32_t seq, uint32_t resp_seq, uint32_t last_sent)
{
	uint16_t len, old_len, i;
	struct sctp_stream_reset_out_request *req_out;
	struct sctp_chunkhdr *ch;

	ch = mtod(chk->data, struct sctp_chunkhdr *);
	old_len = len = SCTP_SIZE32(ntohs(ch->chunk_length));

	
	req_out = (struct sctp_stream_reset_out_request *)((caddr_t)ch + len);
	
	len = (sizeof(struct sctp_stream_reset_out_request) + (sizeof(uint16_t) * number_entries));
	req_out->ph.param_type = htons(SCTP_STR_RESET_OUT_REQUEST);
	req_out->ph.param_length = htons(len);
	req_out->request_seq = htonl(seq);
	req_out->response_seq = htonl(resp_seq);
	req_out->send_reset_at_tsn = htonl(last_sent);
	if (number_entries) {
		for (i = 0; i < number_entries; i++) {
			req_out->list_of_streams[i] = htons(list[i]);
		}
	}
	if (SCTP_SIZE32(len) > len) {
		




		req_out->list_of_streams[number_entries] = 0;
	}
	
	ch->chunk_length = htons(len + old_len);
	chk->book_size = len + old_len;
	chk->book_size_scale = 0;
	chk->send_size = SCTP_SIZE32(chk->book_size);
	SCTP_BUF_LEN(chk->data) = chk->send_size;
	return;
}

static void
sctp_add_stream_reset_in(struct sctp_tmit_chunk *chk,
                         int number_entries, uint16_t *list,
                         uint32_t seq)
{
	uint16_t len, old_len, i;
	struct sctp_stream_reset_in_request *req_in;
	struct sctp_chunkhdr *ch;

	ch = mtod(chk->data, struct sctp_chunkhdr *);
	old_len = len = SCTP_SIZE32(ntohs(ch->chunk_length));

	
	req_in = (struct sctp_stream_reset_in_request *)((caddr_t)ch + len);
	
	len = (sizeof(struct sctp_stream_reset_in_request) + (sizeof(uint16_t) * number_entries));
	req_in->ph.param_type = htons(SCTP_STR_RESET_IN_REQUEST);
	req_in->ph.param_length = htons(len);
	req_in->request_seq = htonl(seq);
	if (number_entries) {
		for (i = 0; i < number_entries; i++) {
			req_in->list_of_streams[i] = htons(list[i]);
		}
	}
	if (SCTP_SIZE32(len) > len) {
		




		req_in->list_of_streams[number_entries] = 0;
	}
	
	ch->chunk_length = htons(len + old_len);
	chk->book_size = len + old_len;
	chk->book_size_scale = 0;
	chk->send_size = SCTP_SIZE32(chk->book_size);
	SCTP_BUF_LEN(chk->data) = chk->send_size;
	return;
}

static void
sctp_add_stream_reset_tsn(struct sctp_tmit_chunk *chk,
                          uint32_t seq)
{
	uint16_t len, old_len;
	struct sctp_stream_reset_tsn_request *req_tsn;
	struct sctp_chunkhdr *ch;

	ch = mtod(chk->data, struct sctp_chunkhdr *);
	old_len = len = SCTP_SIZE32(ntohs(ch->chunk_length));

	
	req_tsn = (struct sctp_stream_reset_tsn_request *)((caddr_t)ch + len);
	
	len = sizeof(struct sctp_stream_reset_tsn_request);
	req_tsn->ph.param_type = htons(SCTP_STR_RESET_TSN_REQUEST);
	req_tsn->ph.param_length = htons(len);
	req_tsn->request_seq = htonl(seq);

	
	ch->chunk_length = htons(len + old_len);
	chk->send_size = len + old_len;
	chk->book_size = SCTP_SIZE32(chk->send_size);
	chk->book_size_scale = 0;
	SCTP_BUF_LEN(chk->data) = SCTP_SIZE32(chk->send_size);
	return;
}

void
sctp_add_stream_reset_result(struct sctp_tmit_chunk *chk,
                             uint32_t resp_seq, uint32_t result)
{
	uint16_t len, old_len;
	struct sctp_stream_reset_response *resp;
	struct sctp_chunkhdr *ch;

	ch = mtod(chk->data, struct sctp_chunkhdr *);
	old_len = len = SCTP_SIZE32(ntohs(ch->chunk_length));

	
	resp = (struct sctp_stream_reset_response *)((caddr_t)ch + len);
	
	len = sizeof(struct sctp_stream_reset_response);
	resp->ph.param_type = htons(SCTP_STR_RESET_RESPONSE);
	resp->ph.param_length = htons(len);
	resp->response_seq = htonl(resp_seq);
	resp->result = ntohl(result);

	
	ch->chunk_length = htons(len + old_len);
	chk->book_size = len + old_len;
	chk->book_size_scale = 0;
	chk->send_size = SCTP_SIZE32(chk->book_size);
	SCTP_BUF_LEN(chk->data) = chk->send_size;
	return;
}

void
sctp_add_stream_reset_result_tsn(struct sctp_tmit_chunk *chk,
                                 uint32_t resp_seq, uint32_t result,
                                 uint32_t send_una, uint32_t recv_next)
{
	uint16_t len, old_len;
	struct sctp_stream_reset_response_tsn *resp;
	struct sctp_chunkhdr *ch;

	ch = mtod(chk->data, struct sctp_chunkhdr *);
	old_len = len = SCTP_SIZE32(ntohs(ch->chunk_length));

	
	resp = (struct sctp_stream_reset_response_tsn *)((caddr_t)ch + len);
	
	len = sizeof(struct sctp_stream_reset_response_tsn);
	resp->ph.param_type = htons(SCTP_STR_RESET_RESPONSE);
	resp->ph.param_length = htons(len);
	resp->response_seq = htonl(resp_seq);
	resp->result = htonl(result);
	resp->senders_next_tsn = htonl(send_una);
	resp->receivers_next_tsn = htonl(recv_next);

	
	ch->chunk_length = htons(len + old_len);
	chk->book_size = len + old_len;
	chk->send_size = SCTP_SIZE32(chk->book_size);
	chk->book_size_scale = 0;
	SCTP_BUF_LEN(chk->data) = chk->send_size;
	return;
}

static void
sctp_add_an_out_stream(struct sctp_tmit_chunk *chk,
		       uint32_t seq,
		       uint16_t adding)
{
	uint16_t len, old_len;
	struct sctp_chunkhdr *ch;
	struct sctp_stream_reset_add_strm *addstr;

	ch = mtod(chk->data, struct sctp_chunkhdr *);
	old_len = len = SCTP_SIZE32(ntohs(ch->chunk_length));

	
	addstr = (struct sctp_stream_reset_add_strm *)((caddr_t)ch + len);
	
	len = sizeof(struct sctp_stream_reset_add_strm);

	
	addstr->ph.param_type = htons(SCTP_STR_RESET_ADD_OUT_STREAMS);
	addstr->ph.param_length = htons(len);
	addstr->request_seq = htonl(seq);
	addstr->number_of_streams = htons(adding);
	addstr->reserved = 0;

	
	ch->chunk_length = htons(len + old_len);
	chk->send_size = len + old_len;
	chk->book_size = SCTP_SIZE32(chk->send_size);
	chk->book_size_scale = 0;
	SCTP_BUF_LEN(chk->data) = SCTP_SIZE32(chk->send_size);
	return;
}

static void
sctp_add_an_in_stream(struct sctp_tmit_chunk *chk,
                      uint32_t seq,
                      uint16_t adding)
{
	uint16_t len, old_len;
	struct sctp_chunkhdr *ch;
	struct sctp_stream_reset_add_strm *addstr;

	ch = mtod(chk->data, struct sctp_chunkhdr *);
	old_len = len = SCTP_SIZE32(ntohs(ch->chunk_length));

	
	addstr = (struct sctp_stream_reset_add_strm *)((caddr_t)ch + len);
	
	len = sizeof(struct sctp_stream_reset_add_strm);
	
	addstr->ph.param_type = htons(SCTP_STR_RESET_ADD_IN_STREAMS);
	addstr->ph.param_length = htons(len);
	addstr->request_seq = htonl(seq);
	addstr->number_of_streams = htons(adding);
	addstr->reserved = 0;

	
	ch->chunk_length = htons(len + old_len);
	chk->send_size = len + old_len;
	chk->book_size = SCTP_SIZE32(chk->send_size);
	chk->book_size_scale = 0;
	SCTP_BUF_LEN(chk->data) = SCTP_SIZE32(chk->send_size);
	return;
}

int
sctp_send_str_reset_req(struct sctp_tcb *stcb,
                        int number_entries, uint16_t *list,
                        uint8_t send_out_req,
                        uint8_t send_in_req,
                        uint8_t send_tsn_req,
                        uint8_t add_stream,
                        uint16_t adding_o,
                        uint16_t adding_i, uint8_t peer_asked)
{

	struct sctp_association *asoc;
	struct sctp_tmit_chunk *chk;
	struct sctp_chunkhdr *ch;
	uint32_t seq;

	asoc = &stcb->asoc;
	if (asoc->stream_reset_outstanding) {
		


		SCTP_LTRACE_ERR_RET(NULL, stcb, NULL, SCTP_FROM_SCTP_OUTPUT, EBUSY);
		return (EBUSY);
	}
	if ((send_out_req == 0) && (send_in_req == 0) && (send_tsn_req == 0) &&
	    (add_stream == 0)) {
		
		SCTP_LTRACE_ERR_RET(NULL, stcb, NULL, SCTP_FROM_SCTP_OUTPUT, EINVAL);
		return (EINVAL);
	}
	if (send_tsn_req && (send_out_req || send_in_req)) {
		
		SCTP_LTRACE_ERR_RET(NULL, stcb, NULL, SCTP_FROM_SCTP_OUTPUT, EINVAL);
		return (EINVAL);
	}
	sctp_alloc_a_chunk(stcb, chk);
	if (chk == NULL) {
		SCTP_LTRACE_ERR_RET(NULL, stcb, NULL, SCTP_FROM_SCTP_OUTPUT, ENOMEM);
		return (ENOMEM);
	}
	chk->copy_by_ref = 0;
	chk->rec.chunk_id.id = SCTP_STREAM_RESET;
	chk->rec.chunk_id.can_take_data = 0;
	chk->asoc = &stcb->asoc;
	chk->book_size = sizeof(struct sctp_chunkhdr);
	chk->send_size = SCTP_SIZE32(chk->book_size);
	chk->book_size_scale = 0;

	chk->data = sctp_get_mbuf_for_msg(MCLBYTES, 0, M_NOWAIT, 1, MT_DATA);
	if (chk->data == NULL) {
		sctp_free_a_chunk(stcb, chk, SCTP_SO_LOCKED);
		SCTP_LTRACE_ERR_RET(NULL, stcb, NULL, SCTP_FROM_SCTP_OUTPUT, ENOMEM);
		return (ENOMEM);
	}
	SCTP_BUF_RESV_UF(chk->data, SCTP_MIN_OVERHEAD);

	
	chk->sent = SCTP_DATAGRAM_UNSENT;
	chk->snd_count = 0;
	if (stcb->asoc.alternate) {
		chk->whoTo = stcb->asoc.alternate;
	} else {
		chk->whoTo = stcb->asoc.primary_destination;
	}
	atomic_add_int(&chk->whoTo->ref_count, 1);
	ch = mtod(chk->data, struct sctp_chunkhdr *);
	ch->chunk_type = SCTP_STREAM_RESET;
	ch->chunk_flags = 0;
	ch->chunk_length = htons(chk->book_size);
	SCTP_BUF_LEN(chk->data) = chk->send_size;

	seq = stcb->asoc.str_reset_seq_out;
	if (send_out_req) {
		sctp_add_stream_reset_out(chk, number_entries, list,
					  seq, (stcb->asoc.str_reset_seq_in - 1), (stcb->asoc.sending_seq - 1));
		asoc->stream_reset_out_is_outstanding = 1;
		seq++;
		asoc->stream_reset_outstanding++;
	}
	if ((add_stream & 1) &&
	    ((stcb->asoc.strm_realoutsize - stcb->asoc.streamoutcnt) < adding_o)) {
		
		struct sctp_stream_out *oldstream;
		struct sctp_stream_queue_pending *sp, *nsp;
		int i;

		oldstream = stcb->asoc.strmout;
		
		SCTP_MALLOC(stcb->asoc.strmout, struct sctp_stream_out *,
			    ((stcb->asoc.streamoutcnt+adding_o) * sizeof(struct sctp_stream_out)),
			    SCTP_M_STRMO);
		if (stcb->asoc.strmout == NULL) {
			uint8_t x;
			stcb->asoc.strmout = oldstream;
			
			x = add_stream & 0xfe;
			add_stream = x;
			goto skip_stuff;
		}
		


		SCTP_TCB_SEND_LOCK(stcb);
		stcb->asoc.ss_functions.sctp_ss_clear(stcb, &stcb->asoc, 0, 1);
		for (i = 0; i < stcb->asoc.streamoutcnt; i++) {
			TAILQ_INIT(&stcb->asoc.strmout[i].outqueue);
			stcb->asoc.strmout[i].chunks_on_queues = oldstream[i].chunks_on_queues;
			stcb->asoc.strmout[i].next_sequence_send = oldstream[i].next_sequence_send;
			stcb->asoc.strmout[i].last_msg_incomplete = oldstream[i].last_msg_incomplete;
			stcb->asoc.strmout[i].stream_no = i;
			stcb->asoc.ss_functions.sctp_ss_init_stream(&stcb->asoc.strmout[i], &oldstream[i]);
			
			TAILQ_FOREACH_SAFE(sp, &oldstream[i].outqueue, next, nsp) {
				TAILQ_REMOVE(&oldstream[i].outqueue, sp, next);
				TAILQ_INSERT_TAIL(&stcb->asoc.strmout[i].outqueue, sp, next);
			}
			
			if (stcb->asoc.last_out_stream == &oldstream[i]) {
				stcb->asoc.last_out_stream = &stcb->asoc.strmout[i];
			}
			if (stcb->asoc.locked_on_sending == &oldstream[i]) {
				stcb->asoc.locked_on_sending = &stcb->asoc.strmout[i];
			}
		}
		
		stcb->asoc.ss_functions.sctp_ss_init(stcb, &stcb->asoc, 1);
		for (i = stcb->asoc.streamoutcnt; i < (stcb->asoc.streamoutcnt + adding_o); i++) {
			TAILQ_INIT(&stcb->asoc.strmout[i].outqueue);
			stcb->asoc.strmout[i].chunks_on_queues = 0;
			stcb->asoc.strmout[i].next_sequence_send = 0x0;
			stcb->asoc.strmout[i].stream_no = i;
			stcb->asoc.strmout[i].last_msg_incomplete = 0;
			stcb->asoc.ss_functions.sctp_ss_init_stream(&stcb->asoc.strmout[i], NULL);
		}
		stcb->asoc.strm_realoutsize = stcb->asoc.streamoutcnt + adding_o;
		SCTP_FREE(oldstream, SCTP_M_STRMO);
		SCTP_TCB_SEND_UNLOCK(stcb);
	}
skip_stuff:
	if ((add_stream & 1) && (adding_o > 0)) {
		asoc->strm_pending_add_size = adding_o;
		asoc->peer_req_out = peer_asked;
		sctp_add_an_out_stream(chk, seq, adding_o);
		seq++;
		asoc->stream_reset_outstanding++;
	}
	if ((add_stream & 2) && (adding_i > 0)) {
		sctp_add_an_in_stream(chk, seq, adding_i);
		seq++;
		asoc->stream_reset_outstanding++;
	}
	if (send_in_req) {
		sctp_add_stream_reset_in(chk, number_entries, list, seq);
		seq++;
		asoc->stream_reset_outstanding++;
	}
	if (send_tsn_req) {
		sctp_add_stream_reset_tsn(chk, seq);
		asoc->stream_reset_outstanding++;
	}
	asoc->str_reset = chk;
	
	TAILQ_INSERT_TAIL(&asoc->control_send_queue,
			  chk,
			  sctp_next);
	asoc->ctrl_queue_cnt++;
	sctp_timer_start(SCTP_TIMER_TYPE_STRRESET, stcb->sctp_ep, stcb, chk->whoTo);
	return (0);
}

void
sctp_send_abort(struct mbuf *m, int iphlen, struct sockaddr *src, struct sockaddr *dst,
                struct sctphdr *sh, uint32_t vtag, struct mbuf *cause,
#if defined(__FreeBSD__)
                uint8_t use_mflowid, uint32_t mflowid,
#endif
                uint32_t vrf_id, uint16_t port)
{
	
	if (sctp_is_there_an_abort_here(m, iphlen, &vtag)) {
		if (cause)
			sctp_m_freem(cause);
		return;
	}
	sctp_send_resp_msg(src, dst, sh, vtag, SCTP_ABORT_ASSOCIATION, cause,
#if defined(__FreeBSD__)
	                   use_mflowid, mflowid,
#endif
	                   vrf_id, port);
	return;
}

void
sctp_send_operr_to(struct sockaddr *src, struct sockaddr *dst,
                   struct sctphdr *sh, uint32_t vtag, struct mbuf *cause,
#if defined(__FreeBSD__)
                   uint8_t use_mflowid, uint32_t mflowid,
#endif
                   uint32_t vrf_id, uint16_t port)
{
	sctp_send_resp_msg(src, dst, sh, vtag, SCTP_OPERATION_ERROR, cause,
#if defined(__FreeBSD__)
	                   use_mflowid, mflowid,
#endif
	                   vrf_id, port);
	return;
}

static struct mbuf *
sctp_copy_resume(struct uio *uio,
		 int max_send_len,
#if defined(__FreeBSD__) && __FreeBSD_version > 602000
		 int user_marks_eor,
#endif
		 int *error,
		 uint32_t *sndout,
		 struct mbuf **new_tail)
{
#if defined(__Panda__)
	struct mbuf *m;

	m = m_uiotombuf(uio, M_WAITOK, max_send_len, 0,
			(user_marks_eor ? M_EOR : 0));
	if (m == NULL) {
		SCTP_LTRACE_ERR_RET(NULL, NULL, NULL, SCTP_FROM_SCTP_OUTPUT, ENOMEM);
		*error = ENOMEM;
	} else {
		*sndout = m_length(m, NULL);
		*new_tail = m_last(m);
	}
	return (m);
#elif defined(__FreeBSD__) && __FreeBSD_version > 602000
	struct mbuf *m;

	m = m_uiotombuf(uio, M_WAITOK, max_send_len, 0,
		(M_PKTHDR | (user_marks_eor ? M_EOR : 0)));
	if (m == NULL) {
		SCTP_LTRACE_ERR_RET(NULL, NULL, NULL, SCTP_FROM_SCTP_OUTPUT, ENOMEM);
		*error = ENOMEM;
	} else {
		*sndout = m_length(m, NULL);
		*new_tail = m_last(m);
	}
	return (m);
#else
	int left, cancpy, willcpy;
	struct mbuf *m, *head;

#if defined(__APPLE__)
#if defined(APPLE_LEOPARD)
        left = min(uio->uio_resid, max_send_len);
#else
        left = min(uio_resid(uio), max_send_len);
#endif
#else
        left = min(uio->uio_resid, max_send_len);
#endif
	
	head = sctp_get_mbuf_for_msg(left, 0, M_WAITOK, 0, MT_DATA);
	if (head == NULL) {
		SCTP_LTRACE_ERR_RET(NULL, NULL, NULL, SCTP_FROM_SCTP_OUTPUT, ENOMEM);
		*error = ENOMEM;
		return (NULL);
	}
	cancpy = M_TRAILINGSPACE(head);
	willcpy = min(cancpy, left);
	*error = uiomove(mtod(head, caddr_t), willcpy, uio);
	if (*error) {
		sctp_m_freem(head);
		return (NULL);
	}
	*sndout += willcpy;
	left -= willcpy;
	SCTP_BUF_LEN(head) = willcpy;
	m = head;
	*new_tail = head;
	while (left > 0) {
		
		SCTP_BUF_NEXT(m) = sctp_get_mbuf_for_msg(left, 0, M_WAITOK, 0, MT_DATA);
		if (SCTP_BUF_NEXT(m) == NULL) {
			sctp_m_freem(head);
			*new_tail = NULL;
			SCTP_LTRACE_ERR_RET(NULL, NULL, NULL, SCTP_FROM_SCTP_OUTPUT, ENOMEM);
			*error = ENOMEM;
			return (NULL);
		}
		m = SCTP_BUF_NEXT(m);
		cancpy = M_TRAILINGSPACE(m);
		willcpy = min(cancpy, left);
		*error = uiomove(mtod(m, caddr_t), willcpy, uio);
		if (*error) {
			sctp_m_freem(head);
			*new_tail = NULL;
			SCTP_LTRACE_ERR_RET(NULL, NULL, NULL, SCTP_FROM_SCTP_OUTPUT, EFAULT);
			*error = EFAULT;
			return (NULL);
		}
		SCTP_BUF_LEN(m) = willcpy;
		left -= willcpy;
		*sndout += willcpy;
		*new_tail = m;
		if (left == 0) {
			SCTP_BUF_NEXT(m) = NULL;
		}
	}
	return (head);
#endif
}

static int
sctp_copy_one(struct sctp_stream_queue_pending *sp,
	      struct uio *uio,
	      int resv_upfront)
{
	int left;
#if defined(__Panda__)
	left = sp->length;
	sp->data = m_uiotombuf(uio, M_WAITOK, sp->length,
			       resv_upfront, 0);
	if (sp->data == NULL) {
		SCTP_LTRACE_ERR_RET(NULL, NULL, NULL, SCTP_FROM_SCTP_OUTPUT, ENOMEM);
		return (ENOMEM);
	}

	sp->tail_mbuf = m_last(sp->data);
	return (0);

#elif defined(__FreeBSD__) && __FreeBSD_version > 602000
	left = sp->length;
	sp->data = m_uiotombuf(uio, M_WAITOK, sp->length,
			       resv_upfront, 0);
	if (sp->data == NULL) {
		SCTP_LTRACE_ERR_RET(NULL, NULL, NULL, SCTP_FROM_SCTP_OUTPUT, ENOMEM);
		return (ENOMEM);
	}

	sp->tail_mbuf = m_last(sp->data);
	return (0);
#else
	int cancpy, willcpy, error;
	struct mbuf *m, *head;
	int cpsz = 0;

	
	left = sp->length;
	head = m = sctp_get_mbuf_for_msg((left + resv_upfront), 0, M_WAITOK, 0, MT_DATA);
	if (m == NULL) {
		SCTP_LTRACE_ERR_RET(NULL, NULL, NULL, SCTP_FROM_SCTP_OUTPUT, ENOMEM);
		return (ENOMEM);
	}
	



	SCTP_BUF_RESV_UF(m, resv_upfront);
	cancpy = M_TRAILINGSPACE(m);
	willcpy = min(cancpy, left);
	while (left > 0) {
		
		error = uiomove(mtod(m, caddr_t), willcpy, uio);
		if (error) {
			sctp_m_freem(head);
			return (error);
		}
		SCTP_BUF_LEN(m) = willcpy;
		left -= willcpy;
		cpsz += willcpy;
		if (left > 0) {
			SCTP_BUF_NEXT(m) = sctp_get_mbuf_for_msg(left, 0, M_WAITOK, 0, MT_DATA);
			if (SCTP_BUF_NEXT(m) == NULL) {
				



				sctp_m_freem(head);
				SCTP_LTRACE_ERR_RET(NULL, NULL, NULL, SCTP_FROM_SCTP_OUTPUT, ENOMEM);
				return (ENOMEM);
			}
			m = SCTP_BUF_NEXT(m);
			cancpy = M_TRAILINGSPACE(m);
			willcpy = min(cancpy, left);
		} else {
			sp->tail_mbuf = m;
			SCTP_BUF_NEXT(m) = NULL;
		}
	}
	sp->data = head;
	sp->length = cpsz;
	return (0);
#endif
}



static struct sctp_stream_queue_pending *
sctp_copy_it_in(struct sctp_tcb *stcb,
    struct sctp_association *asoc,
    struct sctp_sndrcvinfo *srcv,
    struct uio *uio,
    struct sctp_nets *net,
    int max_send_len,
    int user_marks_eor,
    int *error)

{
	






	struct sctp_stream_queue_pending *sp = NULL;
	int resv_in_first;

	*error = 0;
	
	if ((SCTP_GET_STATE(asoc) == SCTP_STATE_SHUTDOWN_SENT) ||
	    (SCTP_GET_STATE(asoc) == SCTP_STATE_SHUTDOWN_ACK_SENT) ||
	    (SCTP_GET_STATE(asoc) == SCTP_STATE_SHUTDOWN_RECEIVED) ||
	    (asoc->state & SCTP_STATE_SHUTDOWN_PENDING)) {
		
		SCTP_LTRACE_ERR_RET(NULL, stcb, NULL, SCTP_FROM_SCTP_OUTPUT, ECONNRESET);
		*error = ECONNRESET;
		goto out_now;
	}
	sctp_alloc_a_strmoq(stcb, sp);
	if (sp == NULL) {
		SCTP_LTRACE_ERR_RET(NULL, stcb, net, SCTP_FROM_SCTP_OUTPUT, ENOMEM);
		*error = ENOMEM;
		goto out_now;
	}
	sp->act_flags = 0;
	sp->sender_all_done = 0;
	sp->sinfo_flags = srcv->sinfo_flags;
	sp->timetolive = srcv->sinfo_timetolive;
	sp->ppid = srcv->sinfo_ppid;
	sp->context = srcv->sinfo_context;
	(void)SCTP_GETTIME_TIMEVAL(&sp->ts);

	sp->stream = srcv->sinfo_stream;
#if defined(__APPLE__)
#if defined(APPLE_LEOPARD)
	sp->length = min(uio->uio_resid, max_send_len);
#else
	sp->length = min(uio_resid(uio), max_send_len);
#endif
#else
	sp->length = min(uio->uio_resid, max_send_len);
#endif
#if defined(__APPLE__)
#if defined(APPLE_LEOPARD)
	if ((sp->length == (uint32_t)uio->uio_resid) &&
#else
	if ((sp->length == (uint32_t)uio_resid(uio)) &&
#endif
#else
	if ((sp->length == (uint32_t)uio->uio_resid) &&
#endif
	    ((user_marks_eor == 0) ||
	     (srcv->sinfo_flags & SCTP_EOF) ||
	     (user_marks_eor && (srcv->sinfo_flags & SCTP_EOR)))) {
		sp->msg_is_complete = 1;
	} else {
		sp->msg_is_complete = 0;
	}
	sp->sender_all_done = 0;
	sp->some_taken = 0;
	sp->put_last_out = 0;
	resv_in_first = sizeof(struct sctp_data_chunk);
	sp->data = sp->tail_mbuf = NULL;
	if (sp->length == 0) {
		*error = 0;
		goto skip_copy;
	}
	if (srcv->sinfo_keynumber_valid) {
		sp->auth_keyid = srcv->sinfo_keynumber;
	} else {
		sp->auth_keyid = stcb->asoc.authinfo.active_keyid;
	}
	if (sctp_auth_is_required_chunk(SCTP_DATA, stcb->asoc.peer_auth_chunks)) {
		sctp_auth_key_acquire(stcb, sp->auth_keyid);
		sp->holds_key_ref = 1;
	}
#if defined(__APPLE__)
	SCTP_SOCKET_UNLOCK(SCTP_INP_SO(stcb->sctp_ep), 0);
#endif
	*error = sctp_copy_one(sp, uio, resv_in_first);
#if defined(__APPLE__)
	SCTP_SOCKET_LOCK(SCTP_INP_SO(stcb->sctp_ep), 0);
#endif
 skip_copy:
	if (*error) {
		sctp_free_a_strmoq(stcb, sp, SCTP_SO_LOCKED);
		sp = NULL;
	} else {
		if (sp->sinfo_flags & SCTP_ADDR_OVER) {
			sp->net = net;
			atomic_add_int(&sp->net->ref_count, 1);
		} else {
			sp->net = NULL;
		}
		sctp_set_prsctp_policy(sp);
	}
out_now:
	return (sp);
}


int
sctp_sosend(struct socket *so,
            struct sockaddr *addr,
            struct uio *uio,
#ifdef __Panda__
            pakhandle_type top,
            pakhandle_type icontrol,
#else
            struct mbuf *top,
            struct mbuf *control,
#endif
#if defined(__APPLE__) || defined(__Panda__)
            int flags
#else
            int flags,
#if defined(__FreeBSD__) && __FreeBSD_version >= 500000
            struct thread *p
#elif defined(__Windows__)
            PKTHREAD p
#else
#if defined(__Userspace__)
            



#endif
            struct proc *p
#endif
#endif
)
{
#ifdef __Panda__
	struct mbuf *control = NULL;
#endif
#if defined(__APPLE__)
	struct proc *p = current_proc();
#endif
	int error, use_sndinfo = 0;
	struct sctp_sndrcvinfo sndrcvninfo;
	struct sockaddr *addr_to_use;
#if defined(INET) && defined(INET6)
	struct sockaddr_in sin;
#endif

#if defined(__APPLE__)
	SCTP_SOCKET_LOCK(so, 1);
#endif
#ifdef __Panda__
	control = SCTP_HEADER_TO_CHAIN(icontrol);
#endif
	if (control) {
		
		if (sctp_find_cmsg(SCTP_SNDRCV, (void *)&sndrcvninfo, control,
		    sizeof(sndrcvninfo))) {
			
			use_sndinfo = 1;
		}
	}
	addr_to_use = addr;
#if defined(INET) && defined(INET6)
	if ((addr) && (addr->sa_family == AF_INET6)) {
		struct sockaddr_in6 *sin6;

		sin6 = (struct sockaddr_in6 *)addr;
		if (IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr)) {
			in6_sin6_2_sin(&sin, sin6);
			addr_to_use = (struct sockaddr *)&sin;
		}
	}
#endif
	error = sctp_lower_sosend(so, addr_to_use, uio, top,
#ifdef __Panda__
				  icontrol,
#else
				  control,
#endif
				  flags,
				  use_sndinfo ? &sndrcvninfo: NULL
#if !(defined(__Panda__) || defined(__Userspace__))
				  , p
#endif
		);
#if defined(__APPLE__)
	SCTP_SOCKET_UNLOCK(so, 1);
#endif
	return (error);
}


int
sctp_lower_sosend(struct socket *so,
                  struct sockaddr *addr,
                  struct uio *uio,
#ifdef __Panda__
                  pakhandle_type i_pak,
                  pakhandle_type i_control,
#else
                  struct mbuf *i_pak,
                  struct mbuf *control,
#endif
                  int flags,
                  struct sctp_sndrcvinfo *srcv
#if !(defined( __Panda__) || defined(__Userspace__))
                  ,
#if defined(__FreeBSD__) && __FreeBSD_version >= 500000
                  struct thread *p
#elif defined(__Windows__)
                  PKTHREAD p
#else
                  struct proc *p
#endif
#endif
	)
{
	unsigned int sndlen = 0, max_len;
	int error, len;
	struct mbuf *top = NULL;
#ifdef __Panda__
	struct mbuf *control = NULL;
#endif
	int queue_only = 0, queue_only_for_init = 0;
	int free_cnt_applied = 0;
	int un_sent;
	int now_filled = 0;
	unsigned int inqueue_bytes = 0;
	struct sctp_block_entry be;
	struct sctp_inpcb *inp;
	struct sctp_tcb *stcb = NULL;
	struct timeval now;
	struct sctp_nets *net;
	struct sctp_association *asoc;
	struct sctp_inpcb *t_inp;
	int user_marks_eor;
	int create_lock_applied = 0;
	int nagle_applies = 0;
	int some_on_control = 0;
	int got_all_of_the_send = 0;
	int hold_tcblock = 0;
	int non_blocking = 0;
	uint32_t local_add_more, local_soresv = 0;
	uint16_t port;
	uint16_t sinfo_flags;
	sctp_assoc_t sinfo_assoc_id;

	error = 0;
	net = NULL;
	stcb = NULL;
	asoc = NULL;

#if defined(__APPLE__)
	sctp_lock_assert(so);
#endif
	t_inp = inp = (struct sctp_inpcb *)so->so_pcb;
	if (inp == NULL) {
		SCTP_LTRACE_ERR_RET(NULL, NULL, NULL, SCTP_FROM_SCTP_OUTPUT, EINVAL);
		error = EINVAL;
		if (i_pak) {
			SCTP_RELEASE_PKT(i_pak);
		}
		return (error);
	}
	if ((uio == NULL) && (i_pak == NULL)) {
		SCTP_LTRACE_ERR_RET(inp, stcb, net, SCTP_FROM_SCTP_OUTPUT, EINVAL);
		return (EINVAL);
	}
	user_marks_eor = sctp_is_feature_on(inp, SCTP_PCB_FLAGS_EXPLICIT_EOR);
	atomic_add_int(&inp->total_sends, 1);
	if (uio) {
#if defined(__APPLE__)
#if defined(APPLE_LEOPARD)
		if (uio->uio_resid < 0) {
#else
		if (uio_resid(uio) < 0) {
#endif
#else
		if (uio->uio_resid < 0) {
#endif
			SCTP_LTRACE_ERR_RET(inp, stcb, net, SCTP_FROM_SCTP_OUTPUT, EINVAL);
			return (EINVAL);
		}
#if defined(__APPLE__)
#if defined(APPLE_LEOPARD)
		sndlen = uio->uio_resid;
#else
		sndlen = uio_resid(uio);
#endif
#else
		sndlen = uio->uio_resid;
#endif
	} else {
		top = SCTP_HEADER_TO_CHAIN(i_pak);
#ifdef __Panda__
		



		sndlen = SCTP_APP_DATA_LEN(i_pak);
		





		if (sndlen == 0) {
			SCTP_BUF_LEN(top)  = 0;
		}
		




		SCTP_DETACH_HEADER_FROM_CHAIN(i_pak);
#else
		sndlen = SCTP_HEADER_LEN(i_pak);
#endif
	}
	SCTPDBG(SCTP_DEBUG_OUTPUT1, "Send called addr:%p send length %d\n",
		(void *)addr,
	        sndlen);
#ifdef __Panda__
	if (i_control) {
		control = SCTP_HEADER_TO_CHAIN(i_control);
	}
#endif
	if ((inp->sctp_flags & SCTP_PCB_FLAGS_TCPTYPE) &&
	    (inp->sctp_socket->so_qlimit)) {
		
		SCTP_LTRACE_ERR_RET(NULL, NULL, NULL, SCTP_FROM_SCTP_OUTPUT, ENOTCONN);
		error = ENOTCONN;
		goto out_unlocked;
	}
	



	if (addr) {
		union sctp_sockstore *raddr = (union sctp_sockstore *)addr;
		switch (raddr->sa.sa_family) {
#ifdef INET
		case AF_INET:
#ifdef HAVE_SIN_LEN
			if (raddr->sin.sin_len != sizeof(struct sockaddr_in)) {
				SCTP_LTRACE_ERR_RET(inp, stcb, net, SCTP_FROM_SCTP_OUTPUT, EINVAL);
				error = EINVAL;
				goto out_unlocked;
			}
#endif
			port = raddr->sin.sin_port;
			break;
#endif
#ifdef INET6
		case AF_INET6:
#ifdef HAVE_SIN6_LEN
			if (raddr->sin6.sin6_len != sizeof(struct sockaddr_in6)) {
				SCTP_LTRACE_ERR_RET(inp, stcb, net, SCTP_FROM_SCTP_OUTPUT, EINVAL);
				error = EINVAL;
				goto out_unlocked;
			}
#endif
			port = raddr->sin6.sin6_port;
			break;
#endif
#if defined(__Userspace__)
		case AF_CONN:
#ifdef HAVE_SCONN_LEN
			if (raddr->sconn.sconn_len != sizeof(struct sockaddr_conn)) {
				SCTP_LTRACE_ERR_RET(inp, stcb, net, SCTP_FROM_SCTP_OUTPUT, EINVAL);
				error = EINVAL;
				goto out_unlocked;
			}
#endif
			port = raddr->sconn.sconn_port;
			break;
#endif
		default:
			SCTP_LTRACE_ERR_RET(inp, stcb, net, SCTP_FROM_SCTP_OUTPUT, EAFNOSUPPORT);
			error = EAFNOSUPPORT;
			goto out_unlocked;
		}
	} else
		port = 0;

	if (srcv) {
		sinfo_flags = srcv->sinfo_flags;
		sinfo_assoc_id = srcv->sinfo_assoc_id;
		if (INVALID_SINFO_FLAG(sinfo_flags) ||
		    PR_SCTP_INVALID_POLICY(sinfo_flags)) {
			SCTP_LTRACE_ERR_RET(inp, stcb, net, SCTP_FROM_SCTP_OUTPUT, EINVAL);
			error = EINVAL;
			goto out_unlocked;
		}
		if (srcv->sinfo_flags)
			SCTP_STAT_INCR(sctps_sends_with_flags);
	} else {
		sinfo_flags = inp->def_send.sinfo_flags;
		sinfo_assoc_id = inp->def_send.sinfo_assoc_id;
	}
	if (sinfo_flags & SCTP_SENDALL) {
		
		error = sctp_sendall(inp, uio, top, srcv);
		top = NULL;
		goto out_unlocked;
	}
	if ((sinfo_flags & SCTP_ADDR_OVER) && (addr == NULL)) {
		SCTP_LTRACE_ERR_RET(inp, stcb, net, SCTP_FROM_SCTP_OUTPUT, EINVAL);
		error = EINVAL;
		goto out_unlocked;
	}
	
	if ((inp->sctp_flags & SCTP_PCB_FLAGS_CONNECTED) ||
	    (inp->sctp_flags & SCTP_PCB_FLAGS_IN_TCPPOOL)) {
		SCTP_INP_RLOCK(inp);
		stcb = LIST_FIRST(&inp->sctp_asoc_list);
		if (stcb) {
			SCTP_TCB_LOCK(stcb);
			hold_tcblock = 1;
		}
		SCTP_INP_RUNLOCK(inp);
	} else if (sinfo_assoc_id) {
		stcb = sctp_findassociation_ep_asocid(inp, sinfo_assoc_id, 0);
	} else if (addr) {
		




		SCTP_INP_WLOCK(inp);
		SCTP_INP_INCR_REF(inp);
		SCTP_INP_WUNLOCK(inp);
		stcb = sctp_findassociation_ep_addr(&t_inp, addr, &net, NULL, NULL);
		if (stcb == NULL) {
			SCTP_INP_WLOCK(inp);
			SCTP_INP_DECR_REF(inp);
			SCTP_INP_WUNLOCK(inp);
		} else {
			hold_tcblock = 1;
		}
	}
	if ((stcb == NULL) && (addr)) {
		
		SCTP_ASOC_CREATE_LOCK(inp);
		create_lock_applied = 1;
		if ((inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_GONE) ||
		    (inp->sctp_flags & SCTP_PCB_FLAGS_SOCKET_ALLGONE)) {
			
			SCTP_LTRACE_ERR_RET(NULL, NULL, NULL, SCTP_FROM_SCTP_OUTPUT, EINVAL);
			error = EINVAL;
			goto out_unlocked;

		}
		if (((inp->sctp_flags & SCTP_PCB_FLAGS_BOUND_V6) == 0) &&
		    (addr->sa_family == AF_INET6)) {
			SCTP_LTRACE_ERR_RET(inp, stcb, net, SCTP_FROM_SCTP_OUTPUT, EINVAL);
			error = EINVAL;
			goto out_unlocked;
		}
		SCTP_INP_WLOCK(inp);
		SCTP_INP_INCR_REF(inp);
		SCTP_INP_WUNLOCK(inp);
		
		stcb = sctp_findassociation_ep_addr(&t_inp, addr, &net, NULL, NULL);
		if ((stcb == NULL) && (control != NULL) && (port > 0)) {
			stcb = sctp_findassociation_cmsgs(&t_inp, port, control, &net, &error);
		}
		if (stcb == NULL) {
			SCTP_INP_WLOCK(inp);
			SCTP_INP_DECR_REF(inp);
			SCTP_INP_WUNLOCK(inp);
		} else {
			hold_tcblock = 1;
		}
		if (error) {
			goto out_unlocked;
		}
		if (t_inp != inp) {
			SCTP_LTRACE_ERR_RET(inp, stcb, net, SCTP_FROM_SCTP_OUTPUT, ENOTCONN);
			error = ENOTCONN;
			goto out_unlocked;
		}
	}
	if (stcb == NULL) {
		if (addr == NULL) {
			SCTP_LTRACE_ERR_RET(inp, stcb, net, SCTP_FROM_SCTP_OUTPUT, ENOENT);
			error = ENOENT;
			goto out_unlocked;
		} else {
			
			uint32_t vrf_id;

			if ((sinfo_flags & SCTP_ABORT) ||
			    ((sinfo_flags & SCTP_EOF) && (sndlen == 0))) {
				



				SCTP_LTRACE_ERR_RET(inp, stcb, net, SCTP_FROM_SCTP_OUTPUT, ENOENT);
				error = ENOENT;
				goto out_unlocked;
			}
			
			vrf_id = inp->def_vrf_id;
#ifdef INVARIANTS
			if (create_lock_applied == 0) {
				panic("Error, should hold create lock and I don't?");
			}
#endif
			stcb = sctp_aloc_assoc(inp, addr, &error, 0, vrf_id,
#if !(defined( __Panda__) || defined(__Userspace__))
					       p
#else
					       (struct proc *)NULL
#endif
				);
			if (stcb == NULL) {
				
				goto out_unlocked;
			}
			if (stcb->sctp_ep->sctp_flags & SCTP_PCB_FLAGS_TCPTYPE) {
				stcb->sctp_ep->sctp_flags |= SCTP_PCB_FLAGS_CONNECTED;
				
				soisconnecting(so);
			}
			hold_tcblock = 1;
			if (create_lock_applied) {
				SCTP_ASOC_CREATE_UNLOCK(inp);
				create_lock_applied = 0;
			} else {
				SCTP_PRINTF("Huh-3? create lock should have been on??\n");
			}
			
			queue_only = 1;
			asoc = &stcb->asoc;
			SCTP_SET_STATE(asoc, SCTP_STATE_COOKIE_WAIT);
			(void)SCTP_GETTIME_TIMEVAL(&asoc->time_entered);

			
			sctp_initialize_auth_params(inp, stcb);

			if (control) {
				if (sctp_process_cmsgs_for_init(stcb, control, &error)) {
					sctp_free_assoc(inp, stcb, SCTP_PCBFREE_FORCE, SCTP_FROM_SCTP_OUTPUT + SCTP_LOC_7);
					hold_tcblock = 0;
					stcb = NULL;
					goto out_unlocked;
				}
			}
			
			queue_only_for_init = 1;
			





		}
	} else
		asoc = &stcb->asoc;
	if (srcv == NULL)
		srcv = (struct sctp_sndrcvinfo *)&asoc->def_send;
	if (srcv->sinfo_flags & SCTP_ADDR_OVER) {
		if (addr)
			net = sctp_findnet(stcb, addr);
		else
			net = NULL;
		if ((net == NULL) ||
		    ((port != 0) && (port != stcb->rport))) {
			SCTP_LTRACE_ERR_RET(inp, stcb, net, SCTP_FROM_SCTP_OUTPUT, EINVAL);
			error = EINVAL;
			goto out_unlocked;
		}
	} else {
		if (stcb->asoc.alternate) {
			net = stcb->asoc.alternate;
		} else {
			net = stcb->asoc.primary_destination;
		}
	}
	atomic_add_int(&stcb->total_sends, 1);
	
	atomic_add_int(&asoc->refcnt, 1);
	free_cnt_applied = 1;

	if (sctp_is_feature_on(inp, SCTP_PCB_FLAGS_NO_FRAGMENT)) {
		if (sndlen > asoc->smallest_mtu) {
			SCTP_LTRACE_ERR_RET(inp, stcb, net, SCTP_FROM_SCTP_OUTPUT, EMSGSIZE);
			error = EMSGSIZE;
			goto out_unlocked;
		}
	}
#if defined(__Userspace__)
	if (inp->recv_callback) {
		non_blocking = 1;
	}
#else
	if (SCTP_SO_IS_NBIO(so)
#if defined(__FreeBSD__) && __FreeBSD_version >= 500000
	     || (flags & MSG_NBIO)
#endif
	    ) {
		non_blocking = 1;
	}
#endif
	
	if (non_blocking) {
		if (hold_tcblock == 0) {
			SCTP_TCB_LOCK(stcb);
			hold_tcblock = 1;
		}
		inqueue_bytes = stcb->asoc.total_output_queue_size - (stcb->asoc.chunks_on_out_queue * sizeof(struct sctp_data_chunk));
		if ((SCTP_SB_LIMIT_SND(so) <  (sndlen + inqueue_bytes + stcb->asoc.sb_send_resv)) ||
		    (stcb->asoc.chunks_on_out_queue >= SCTP_BASE_SYSCTL(sctp_max_chunks_on_queue))) {
			SCTP_LTRACE_ERR_RET(inp, stcb, net, SCTP_FROM_SCTP_OUTPUT, EWOULDBLOCK);
			if (sndlen > SCTP_SB_LIMIT_SND(so))
				error = EMSGSIZE;
			else
				error = EWOULDBLOCK;
			goto out_unlocked;
		}
		stcb->asoc.sb_send_resv += sndlen;
		SCTP_TCB_UNLOCK(stcb);
		hold_tcblock = 0;
	} else {
		atomic_add_int(&stcb->asoc.sb_send_resv, sndlen);
	}
	local_soresv = sndlen;
	if (stcb->asoc.state & SCTP_STATE_ABOUT_TO_BE_FREED) {
		SCTP_LTRACE_ERR_RET(NULL, stcb, NULL, SCTP_FROM_SCTP_OUTPUT, ECONNRESET);
		error = ECONNRESET;
		goto out_unlocked;
	}
	if (create_lock_applied) {
		SCTP_ASOC_CREATE_UNLOCK(inp);
		create_lock_applied = 0;
	}
	if (asoc->stream_reset_outstanding) {
		


		SCTP_LTRACE_ERR_RET(inp, stcb, net, SCTP_FROM_SCTP_OUTPUT, EAGAIN);
		error = EAGAIN;
		goto out_unlocked;
	}
	if ((SCTP_GET_STATE(asoc) == SCTP_STATE_COOKIE_WAIT) ||
	    (SCTP_GET_STATE(asoc) == SCTP_STATE_COOKIE_ECHOED)) {
		queue_only = 1;
	}
	
	if (control) {
		sctp_m_freem(control);
		control = NULL;
	}
	if ((SCTP_GET_STATE(asoc) == SCTP_STATE_SHUTDOWN_SENT) ||
	    (SCTP_GET_STATE(asoc) == SCTP_STATE_SHUTDOWN_RECEIVED) ||
	    (SCTP_GET_STATE(asoc) == SCTP_STATE_SHUTDOWN_ACK_SENT) ||
	    (asoc->state & SCTP_STATE_SHUTDOWN_PENDING)) {
		if (srcv->sinfo_flags & SCTP_ABORT) {
			;
		} else {
			SCTP_LTRACE_ERR_RET(NULL, stcb, NULL, SCTP_FROM_SCTP_OUTPUT, ECONNRESET);
			error = ECONNRESET;
			goto out_unlocked;
		}
	}
	
#if !(defined(__Panda__) || defined(__Windows__) || defined(__Userspace__))
	if (p) {
#if defined(__FreeBSD__) && __FreeBSD_version >= 603000
		p->td_ru.ru_msgsnd++;
#elif defined(__FreeBSD__) && __FreeBSD_version >= 500000
		p->td_proc->p_stats->p_ru.ru_msgsnd++;
#else
		p->p_stats->p_ru.ru_msgsnd++;
#endif
	}
#endif
	
	if (srcv->sinfo_flags & SCTP_ABORT) {
		struct mbuf *mm;
		int tot_demand, tot_out = 0, max_out;

		SCTP_STAT_INCR(sctps_sends_with_abort);
		if ((SCTP_GET_STATE(asoc) == SCTP_STATE_COOKIE_WAIT) ||
		    (SCTP_GET_STATE(asoc) == SCTP_STATE_COOKIE_ECHOED)) {
			
			
			SCTP_LTRACE_ERR_RET(inp, stcb, net, SCTP_FROM_SCTP_OUTPUT, EINVAL);
			error = EINVAL;
			goto out;
		}
		if (hold_tcblock) {
			SCTP_TCB_UNLOCK(stcb);
			hold_tcblock = 0;
		}
		if (top) {
			struct mbuf *cntm = NULL;

			mm = sctp_get_mbuf_for_msg(sizeof(struct sctp_paramhdr), 0, M_WAITOK, 1, MT_DATA);
			if (sndlen != 0) {
				for (cntm = top; cntm; cntm = SCTP_BUF_NEXT(cntm)) {
					tot_out += SCTP_BUF_LEN(cntm);
				}
			}
		} else {
			
			tot_out = sndlen;
			tot_demand = (tot_out + sizeof(struct sctp_paramhdr));
			if (tot_demand > SCTP_DEFAULT_ADD_MORE) {
				
				SCTP_LTRACE_ERR_RET(NULL, stcb, net, SCTP_FROM_SCTP_OUTPUT, EMSGSIZE);
				error = EMSGSIZE;
				goto out;
			}
			mm = sctp_get_mbuf_for_msg(tot_demand, 0, M_WAITOK, 1, MT_DATA);
		}
		if (mm == NULL) {
			SCTP_LTRACE_ERR_RET(NULL, stcb, net, SCTP_FROM_SCTP_OUTPUT, ENOMEM);
			error = ENOMEM;
			goto out;
		}
		max_out = asoc->smallest_mtu - sizeof(struct sctp_paramhdr);
		max_out -= sizeof(struct sctp_abort_msg);
		if (tot_out > max_out) {
			tot_out = max_out;
		}
		if (mm) {
			struct sctp_paramhdr *ph;

			
			ph = mtod(mm, struct sctp_paramhdr *);
			ph->param_type = htons(SCTP_CAUSE_USER_INITIATED_ABT);
			ph->param_length = htons(sizeof(struct sctp_paramhdr) + tot_out);
			ph++;
			SCTP_BUF_LEN(mm) = tot_out + sizeof(struct sctp_paramhdr);
			if (top == NULL) {
#if defined(__APPLE__)
				SCTP_SOCKET_UNLOCK(so, 0);
#endif
				error = uiomove((caddr_t)ph, (int)tot_out, uio);
#if defined(__APPLE__)
				SCTP_SOCKET_LOCK(so, 0);
#endif
				if (error) {
					




					sctp_m_freem(mm);
					mm = NULL;
				}
			} else {
				if (sndlen != 0) {
					SCTP_BUF_NEXT(mm) = top;
				}
			}
		}
		if (hold_tcblock == 0) {
			SCTP_TCB_LOCK(stcb);
		}
		atomic_add_int(&stcb->asoc.refcnt, -1);
		free_cnt_applied = 0;
		
		sctp_abort_an_association(stcb->sctp_ep, stcb, mm, SCTP_SO_LOCKED);
		
		hold_tcblock = 0;
		stcb = NULL;
		




		if (sndlen != 0) {
			top = NULL;
		}
		goto out_unlocked;
	}
	
	inqueue_bytes = stcb->asoc.total_output_queue_size - (stcb->asoc.chunks_on_out_queue * sizeof(struct sctp_data_chunk));
	if (SCTP_SB_LIMIT_SND(so) > inqueue_bytes) {
		if (non_blocking) {
			
			max_len = sndlen;
		} else {
			max_len = SCTP_SB_LIMIT_SND(so) - inqueue_bytes;
		}
	} else {
		max_len = 0;
	}
	if (hold_tcblock) {
		SCTP_TCB_UNLOCK(stcb);
		hold_tcblock = 0;
	}
	
	if (srcv->sinfo_stream >= asoc->streamoutcnt) {
		
		SCTP_LTRACE_ERR_RET(inp, stcb, net, SCTP_FROM_SCTP_OUTPUT, EINVAL);
		error = EINVAL;
		goto out_unlocked;
	}
	if (asoc->strmout == NULL) {
		
		SCTP_LTRACE_ERR_RET(inp, stcb, net, SCTP_FROM_SCTP_OUTPUT, EFAULT);
		error = EFAULT;
		goto out_unlocked;
	}

	
	if ((user_marks_eor == 0) &&
	    (sndlen > SCTP_SB_LIMIT_SND(stcb->sctp_socket))) {
		
		SCTP_LTRACE_ERR_RET(NULL, stcb, net, SCTP_FROM_SCTP_OUTPUT, EMSGSIZE);
		error = EMSGSIZE;
		goto out_unlocked;
	}
	if ((uio == NULL) && user_marks_eor) {
		



		SCTP_LTRACE_ERR_RET(NULL, stcb, net, SCTP_FROM_SCTP_OUTPUT, EINVAL);
		error = EINVAL;
		goto out_unlocked;
	}

	if (user_marks_eor) {
		local_add_more = min(SCTP_SB_LIMIT_SND(so), SCTP_BASE_SYSCTL(sctp_add_more_threshold));
	} else {
		



		local_add_more = sndlen;
	}
	len = 0;
	if (non_blocking) {
		goto skip_preblock;
	}
	if (((max_len <= local_add_more) &&
	     (SCTP_SB_LIMIT_SND(so) >= local_add_more)) ||
	    (max_len == 0) ||
	    ((stcb->asoc.chunks_on_out_queue+stcb->asoc.stream_queue_cnt) >= SCTP_BASE_SYSCTL(sctp_max_chunks_on_queue))) {
		
		SOCKBUF_LOCK(&so->so_snd);
		inqueue_bytes = stcb->asoc.total_output_queue_size - (stcb->asoc.chunks_on_out_queue * sizeof(struct sctp_data_chunk));
		while ((SCTP_SB_LIMIT_SND(so) < (inqueue_bytes + local_add_more)) ||
		       ((stcb->asoc.stream_queue_cnt+stcb->asoc.chunks_on_out_queue) >= SCTP_BASE_SYSCTL(sctp_max_chunks_on_queue))) {
			SCTPDBG(SCTP_DEBUG_OUTPUT1,"pre_block limit:%u <(inq:%d + %d) || (%d+%d > %d)\n",
			        (unsigned int)SCTP_SB_LIMIT_SND(so),
			        inqueue_bytes,
			        local_add_more,
			        stcb->asoc.stream_queue_cnt,
			        stcb->asoc.chunks_on_out_queue,
			        SCTP_BASE_SYSCTL(sctp_max_chunks_on_queue));
			if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_BLK_LOGGING_ENABLE) {
				sctp_log_block(SCTP_BLOCK_LOG_INTO_BLKA, asoc, sndlen);
			}
			be.error = 0;
#if !defined(__Panda__) && !defined(__Windows__)
			stcb->block_entry = &be;
#endif
			error = sbwait(&so->so_snd);
			stcb->block_entry = NULL;
			if (error || so->so_error || be.error) {
				if (error == 0) {
					if (so->so_error)
						error = so->so_error;
					if (be.error) {
						error = be.error;
					}
				}
				SOCKBUF_UNLOCK(&so->so_snd);
				goto out_unlocked;
			}
			if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_BLK_LOGGING_ENABLE) {
				sctp_log_block(SCTP_BLOCK_LOG_OUTOF_BLK,
				               asoc, stcb->asoc.total_output_queue_size);
			}
			if (stcb->asoc.state & SCTP_STATE_ABOUT_TO_BE_FREED) {
				goto out_unlocked;
			}
			inqueue_bytes = stcb->asoc.total_output_queue_size - (stcb->asoc.chunks_on_out_queue * sizeof(struct sctp_data_chunk));
		}
		if (SCTP_SB_LIMIT_SND(so) > inqueue_bytes) {
			max_len = SCTP_SB_LIMIT_SND(so) -  inqueue_bytes;
		} else {
			max_len = 0;
		}
		SOCKBUF_UNLOCK(&so->so_snd);
	}

skip_preblock:
	if (stcb->asoc.state & SCTP_STATE_ABOUT_TO_BE_FREED) {
		goto out_unlocked;
	}
#if defined(__APPLE__)
	error = sblock(&so->so_snd, SBLOCKWAIT(flags));
#endif
	



	if (sndlen == 0) {
		if (srcv->sinfo_flags & SCTP_EOF) {
			got_all_of_the_send = 1;
			goto dataless_eof;
		} else {
			SCTP_LTRACE_ERR_RET(inp, stcb, net, SCTP_FROM_SCTP_OUTPUT, EINVAL);
			error = EINVAL;
			goto out;
		}
	}
	if (top == NULL) {
		struct sctp_stream_queue_pending *sp;
		struct sctp_stream_out *strm;
		uint32_t sndout;

		SCTP_TCB_SEND_LOCK(stcb);
		if ((asoc->stream_locked) &&
		    (asoc->stream_locked_on  != srcv->sinfo_stream)) {
			SCTP_TCB_SEND_UNLOCK(stcb);
			SCTP_LTRACE_ERR_RET(inp, stcb, net, SCTP_FROM_SCTP_OUTPUT, EINVAL);
			error = EINVAL;
			goto out;
		}
		SCTP_TCB_SEND_UNLOCK(stcb);

		strm = &stcb->asoc.strmout[srcv->sinfo_stream];
		if (strm->last_msg_incomplete == 0) {
		do_a_copy_in:
			sp = sctp_copy_it_in(stcb, asoc, srcv, uio, net, max_len, user_marks_eor, &error);
			if ((sp == NULL) || (error)) {
				goto out;
			}
			SCTP_TCB_SEND_LOCK(stcb);
			if (sp->msg_is_complete) {
				strm->last_msg_incomplete = 0;
				asoc->stream_locked = 0;
			} else {
				


				strm->last_msg_incomplete = 1;
				asoc->stream_locked = 1;
				asoc->stream_locked_on  = srcv->sinfo_stream;
				sp->sender_all_done = 0;
			}
			sctp_snd_sb_alloc(stcb, sp->length);
			atomic_add_int(&asoc->stream_queue_cnt, 1);
			if (srcv->sinfo_flags & SCTP_UNORDERED) {
				SCTP_STAT_INCR(sctps_sends_with_unord);
			}
			TAILQ_INSERT_TAIL(&strm->outqueue, sp, next);
			stcb->asoc.ss_functions.sctp_ss_add_to_stream(stcb, asoc, strm, sp, 1);
			SCTP_TCB_SEND_UNLOCK(stcb);
		} else {
			SCTP_TCB_SEND_LOCK(stcb);
			sp = TAILQ_LAST(&strm->outqueue, sctp_streamhead);
			SCTP_TCB_SEND_UNLOCK(stcb);
			if (sp == NULL) {
				
#ifdef INVARIANTS
				panic("Warning: Last msg marked incomplete, yet nothing left?");
#else
				SCTP_PRINTF("Warning: Last msg marked incomplete, yet nothing left?\n");
				strm->last_msg_incomplete = 0;
#endif
				goto do_a_copy_in;

			}
		}
#if defined(__APPLE__)
#if defined(APPLE_LEOPARD)
		while (uio->uio_resid > 0) {
#else
		while (uio_resid(uio) > 0) {
#endif
#else
		while (uio->uio_resid > 0) {
#endif

			struct mbuf *new_tail, *mm;

			if (SCTP_SB_LIMIT_SND(so) > stcb->asoc.total_output_queue_size)
				max_len = SCTP_SB_LIMIT_SND(so) - stcb->asoc.total_output_queue_size;
			else
				max_len = 0;

			if ((max_len > SCTP_BASE_SYSCTL(sctp_add_more_threshold)) ||
			    (max_len && (SCTP_SB_LIMIT_SND(so) < SCTP_BASE_SYSCTL(sctp_add_more_threshold))) ||
#if defined(__APPLE__)
#if defined(APPLE_LEOPARD)
			    (uio->uio_resid && (uio->uio_resid <= (int)max_len))) {
#else
			    (uio_resid(uio) && (uio_resid(uio) <= (int)max_len))) {
#endif
#else
			    (uio->uio_resid && (uio->uio_resid <= (int)max_len))) {
#endif
				sndout = 0;
				new_tail = NULL;
				if (hold_tcblock) {
					SCTP_TCB_UNLOCK(stcb);
					hold_tcblock = 0;
				}
#if defined(__APPLE__)
				SCTP_SOCKET_UNLOCK(so, 0);
#endif
#if defined(__FreeBSD__) && __FreeBSD_version > 602000
				    mm = sctp_copy_resume(uio, max_len, user_marks_eor, &error, &sndout, &new_tail);
#else
				    mm = sctp_copy_resume(uio, max_len, &error, &sndout, &new_tail);
#endif
#if defined(__APPLE__)
				SCTP_SOCKET_LOCK(so, 0);
#endif
				if ((mm == NULL) || error) {
					if (mm) {
						sctp_m_freem(mm);
					}
					goto out;
				}
				
				SCTP_TCB_SEND_LOCK(stcb);
				if (stcb->asoc.state & SCTP_STATE_ABOUT_TO_BE_FREED) {
					


					sctp_m_freem(mm);
					if (stcb->asoc.state & SCTP_PCB_FLAGS_WAS_ABORTED) {
						SCTP_LTRACE_ERR_RET(NULL, stcb, NULL, SCTP_FROM_SCTP_OUTPUT, ECONNRESET);
						error = ECONNRESET;
					}
					SCTP_TCB_SEND_UNLOCK(stcb);
					goto out;
				}
				if (sp->tail_mbuf) {
					
					SCTP_BUF_NEXT(sp->tail_mbuf) = mm;
					sp->tail_mbuf = new_tail;
				} else {
					
					sp->data = mm;
					sp->tail_mbuf = new_tail;
				}
				sctp_snd_sb_alloc(stcb, sndout);
				atomic_add_int(&sp->length,sndout);
				len += sndout;

				
#if defined(__APPLE__)
#if defined(APPLE_LEOPARD)
				if ((uio->uio_resid == 0) &&
#else
				if ((uio_resid(uio) == 0) &&
#endif
#else
				if ((uio->uio_resid == 0) &&
#endif
				    ((user_marks_eor == 0) ||
				     (srcv->sinfo_flags & SCTP_EOF) ||
				     (user_marks_eor && (srcv->sinfo_flags & SCTP_EOR)))) {
					sp->msg_is_complete = 1;
				} else {
					sp->msg_is_complete = 0;
				}
				SCTP_TCB_SEND_UNLOCK(stcb);
			}
#if defined(__APPLE__)
#if defined(APPLE_LEOPARD)
			if (uio->uio_resid == 0) {
#else
			if (uio_resid(uio) == 0) {
#endif
#else
			if (uio->uio_resid == 0) {
#endif

				continue;
			}
			
			if ((asoc->peer_supports_prsctp) && (asoc->sent_queue_cnt_removeable > 0)) {
				
				if (hold_tcblock == 0) {
					SCTP_TCB_LOCK(stcb);
					hold_tcblock = 1;
				}
				sctp_prune_prsctp(stcb, asoc, srcv, sndlen);
				inqueue_bytes = stcb->asoc.total_output_queue_size - (stcb->asoc.chunks_on_out_queue * sizeof(struct sctp_data_chunk));
				if (SCTP_SB_LIMIT_SND(so) > stcb->asoc.total_output_queue_size)
					max_len = SCTP_SB_LIMIT_SND(so) - inqueue_bytes;
				else
					max_len = 0;
				if (max_len > 0) {
					continue;
				}
				SCTP_TCB_UNLOCK(stcb);
				hold_tcblock = 0;
			}
			
			if (non_blocking) {
				
				goto skip_out_eof;
			}
			
			if (queue_only_for_init) {
				if (hold_tcblock == 0) {
					SCTP_TCB_LOCK(stcb);
					hold_tcblock = 1;
				}
				if (SCTP_GET_STATE(&stcb->asoc) == SCTP_STATE_OPEN) {
					
					queue_only = 0;
				} else {
					sctp_send_initiate(inp, stcb, SCTP_SO_LOCKED);
					SCTP_SET_STATE(asoc, SCTP_STATE_COOKIE_WAIT);
					queue_only = 1;
				}
			}
			if ((net->flight_size > net->cwnd) &&
			    (asoc->sctp_cmt_on_off == 0)) {
				SCTP_STAT_INCR(sctps_send_cwnd_avoid);
				queue_only = 1;
			} else if (asoc->ifp_had_enobuf) {
				SCTP_STAT_INCR(sctps_ifnomemqueued);
				if (net->flight_size > (2 * net->mtu)) {
					queue_only = 1;
				}
				asoc->ifp_had_enobuf = 0;
			}
			un_sent = ((stcb->asoc.total_output_queue_size - stcb->asoc.total_flight) +
			           (stcb->asoc.stream_queue_cnt * sizeof(struct sctp_data_chunk)));
			if ((sctp_is_feature_off(inp, SCTP_PCB_FLAGS_NODELAY)) &&
			    (stcb->asoc.total_flight > 0) &&
			    (stcb->asoc.stream_queue_cnt < SCTP_MAX_DATA_BUNDLING) &&
			    (un_sent < (int)(stcb->asoc.smallest_mtu - SCTP_MIN_OVERHEAD))) {

				




				if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_NAGLE_LOGGING_ENABLE) {
					sctp_log_nagle_event(stcb, SCTP_NAGLE_APPLIED);
				}
				SCTP_STAT_INCR(sctps_naglequeued);
				nagle_applies = 1;
			} else {
				if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_NAGLE_LOGGING_ENABLE) {
					if (sctp_is_feature_off(inp, SCTP_PCB_FLAGS_NODELAY))
						sctp_log_nagle_event(stcb, SCTP_NAGLE_SKIPPED);
				}
				SCTP_STAT_INCR(sctps_naglesent);
				nagle_applies = 0;
			}
			if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_BLK_LOGGING_ENABLE) {

				sctp_misc_ints(SCTP_CWNDLOG_PRESEND, queue_only_for_init, queue_only,
					       nagle_applies, un_sent);
				sctp_misc_ints(SCTP_CWNDLOG_PRESEND, stcb->asoc.total_output_queue_size,
					       stcb->asoc.total_flight,
					       stcb->asoc.chunks_on_out_queue, stcb->asoc.total_flight_count);
			}
			if (queue_only_for_init)
				queue_only_for_init = 0;
			if ((queue_only == 0) && (nagle_applies == 0)) {
				






				if (hold_tcblock == 0) {
					if (SCTP_TCB_TRYLOCK(stcb)) {
						hold_tcblock = 1;
						sctp_chunk_output(inp,
								  stcb,
								  SCTP_OUTPUT_FROM_USR_SEND, SCTP_SO_LOCKED);
					}
				} else {
					sctp_chunk_output(inp,
							  stcb,
							  SCTP_OUTPUT_FROM_USR_SEND, SCTP_SO_LOCKED);
				}
				if (hold_tcblock == 1) {
					SCTP_TCB_UNLOCK(stcb);
					hold_tcblock = 0;
				}
			}
			SOCKBUF_LOCK(&so->so_snd);
			













			if (SCTP_SB_LIMIT_SND(so) <= (stcb->asoc.total_output_queue_size +
						      min(SCTP_BASE_SYSCTL(sctp_add_more_threshold), SCTP_SB_LIMIT_SND(so)))) {
				if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_BLK_LOGGING_ENABLE) {
#if defined(__APPLE__)
#if defined(APPLE_LEOPARD)
					sctp_log_block(SCTP_BLOCK_LOG_INTO_BLK,
						       asoc, uio->uio_resid);
#else
					sctp_log_block(SCTP_BLOCK_LOG_INTO_BLK,
						       asoc, uio_resid(uio));
#endif
#else
					sctp_log_block(SCTP_BLOCK_LOG_INTO_BLK,
						       asoc, uio->uio_resid);
#endif
				}
				be.error = 0;
#if !defined(__Panda__) && !defined(__Windows__)
				stcb->block_entry = &be;
#endif
#if defined(__APPLE__)
				sbunlock(&so->so_snd, 1);
#endif
				error = sbwait(&so->so_snd);
				stcb->block_entry = NULL;

				if (error || so->so_error || be.error) {
					if (error == 0) {
						if (so->so_error)
							error = so->so_error;
						if (be.error) {
							error = be.error;
						}
					}
					SOCKBUF_UNLOCK(&so->so_snd);
					goto out_unlocked;
				}

#if defined(__APPLE__)
				error = sblock(&so->so_snd, SBLOCKWAIT(flags));
#endif
				if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_BLK_LOGGING_ENABLE) {
					sctp_log_block(SCTP_BLOCK_LOG_OUTOF_BLK,
						       asoc, stcb->asoc.total_output_queue_size);
				}
			}
			SOCKBUF_UNLOCK(&so->so_snd);
			if (stcb->asoc.state & SCTP_STATE_ABOUT_TO_BE_FREED) {
				goto out_unlocked;
			}
		}
		SCTP_TCB_SEND_LOCK(stcb);
		if (sp) {
			if (sp->msg_is_complete == 0) {
				strm->last_msg_incomplete = 1;
				asoc->stream_locked = 1;
				asoc->stream_locked_on  = srcv->sinfo_stream;
			} else {
				sp->sender_all_done = 1;
				strm->last_msg_incomplete = 0;
				asoc->stream_locked = 0;
			}
		} else {
			SCTP_PRINTF("Huh no sp TSNH?\n");
			strm->last_msg_incomplete = 0;
			asoc->stream_locked = 0;
		}
		SCTP_TCB_SEND_UNLOCK(stcb);
#if defined(__APPLE__)
#if defined(APPLE_LEOPARD)
		if (uio->uio_resid == 0) {
#else
		if (uio_resid(uio) == 0) {
#endif
#else
		if (uio->uio_resid == 0) {
#endif
			got_all_of_the_send = 1;
		}
	} else {
		
		error = sctp_msg_append(stcb, net, top, srcv, 0);
		top = NULL;
		if (srcv->sinfo_flags & SCTP_EOF) {
			





			got_all_of_the_send = 1;
		}
	}
	if (error) {
		goto out;
	}
dataless_eof:
	
	if ((srcv->sinfo_flags & SCTP_EOF) &&
	    (got_all_of_the_send == 1)) {
		int cnt;
		SCTP_STAT_INCR(sctps_sends_with_eof);
		error = 0;
		if (hold_tcblock == 0) {
			SCTP_TCB_LOCK(stcb);
			hold_tcblock = 1;
		}
		cnt = sctp_is_there_unsent_data(stcb, SCTP_SO_LOCKED);
		if (TAILQ_EMPTY(&asoc->send_queue) &&
		    TAILQ_EMPTY(&asoc->sent_queue) &&
		    (cnt == 0)) {
			if (asoc->locked_on_sending) {
				goto abort_anyway;
			}
			
			if ((SCTP_GET_STATE(asoc) != SCTP_STATE_SHUTDOWN_SENT) &&
			    (SCTP_GET_STATE(asoc) != SCTP_STATE_SHUTDOWN_RECEIVED) &&
			    (SCTP_GET_STATE(asoc) != SCTP_STATE_SHUTDOWN_ACK_SENT)) {
				struct sctp_nets *netp;

				
				if (SCTP_GET_STATE(asoc) == SCTP_STATE_OPEN) {
					SCTP_STAT_DECR_GAUGE32(sctps_currestab);
				}
				SCTP_SET_STATE(asoc, SCTP_STATE_SHUTDOWN_SENT);
				SCTP_CLEAR_SUBSTATE(asoc, SCTP_STATE_SHUTDOWN_PENDING);
				sctp_stop_timers_for_shutdown(stcb);
				if (stcb->asoc.alternate) {
					netp = stcb->asoc.alternate;
				} else {
					netp = stcb->asoc.primary_destination;
				}
				sctp_send_shutdown(stcb, netp);
				sctp_timer_start(SCTP_TIMER_TYPE_SHUTDOWN, stcb->sctp_ep, stcb,
				                 netp);
				sctp_timer_start(SCTP_TIMER_TYPE_SHUTDOWNGUARD, stcb->sctp_ep, stcb,
				                 asoc->primary_destination);
			}
		} else {
			



			





			if ((SCTP_GET_STATE(asoc) != SCTP_STATE_SHUTDOWN_SENT) &&
			    (SCTP_GET_STATE(asoc) != SCTP_STATE_SHUTDOWN_RECEIVED) &&
			    (SCTP_GET_STATE(asoc) != SCTP_STATE_SHUTDOWN_ACK_SENT)) {
				if (hold_tcblock == 0) {
					SCTP_TCB_LOCK(stcb);
					hold_tcblock = 1;
				}
				if (asoc->locked_on_sending) {
					
					struct sctp_stream_queue_pending *sp;
					sp = TAILQ_LAST(&asoc->locked_on_sending->outqueue, sctp_streamhead);
					if (sp) {
						if ((sp->length == 0) && (sp->msg_is_complete == 0))
							asoc->state |= SCTP_STATE_PARTIAL_MSG_LEFT;
					}
				}
				asoc->state |= SCTP_STATE_SHUTDOWN_PENDING;
				if (TAILQ_EMPTY(&asoc->send_queue) &&
				    TAILQ_EMPTY(&asoc->sent_queue) &&
				    (asoc->state & SCTP_STATE_PARTIAL_MSG_LEFT)) {
				abort_anyway:
					if (free_cnt_applied) {
						atomic_add_int(&stcb->asoc.refcnt, -1);
						free_cnt_applied = 0;
					}
					sctp_abort_an_association(stcb->sctp_ep, stcb,
					                          NULL, SCTP_SO_LOCKED);
					
					hold_tcblock = 0;
					stcb = NULL;
					goto out;
				}
				sctp_timer_start(SCTP_TIMER_TYPE_SHUTDOWNGUARD, stcb->sctp_ep, stcb,
				                 asoc->primary_destination);
				sctp_feature_off(inp, SCTP_PCB_FLAGS_NODELAY);
			}
		}
	}
skip_out_eof:
	if (!TAILQ_EMPTY(&stcb->asoc.control_send_queue)) {
		some_on_control = 1;
	}
	if (queue_only_for_init) {
		if (hold_tcblock == 0) {
			SCTP_TCB_LOCK(stcb);
			hold_tcblock = 1;
		}
		if (SCTP_GET_STATE(&stcb->asoc) == SCTP_STATE_OPEN) {
			
			queue_only = 0;
		} else {
			sctp_send_initiate(inp, stcb, SCTP_SO_LOCKED);
			SCTP_SET_STATE(&stcb->asoc, SCTP_STATE_COOKIE_WAIT);
			queue_only = 1;
		}
	}
	if ((net->flight_size > net->cwnd) &&
	    (stcb->asoc.sctp_cmt_on_off == 0)) {
		SCTP_STAT_INCR(sctps_send_cwnd_avoid);
		queue_only = 1;
	} else if (asoc->ifp_had_enobuf) {
		SCTP_STAT_INCR(sctps_ifnomemqueued);
		if (net->flight_size > (2 * net->mtu)) {
			queue_only = 1;
		}
		asoc->ifp_had_enobuf = 0;
	}
	un_sent = ((stcb->asoc.total_output_queue_size - stcb->asoc.total_flight) +
	           (stcb->asoc.stream_queue_cnt * sizeof(struct sctp_data_chunk)));
	if ((sctp_is_feature_off(inp, SCTP_PCB_FLAGS_NODELAY)) &&
	    (stcb->asoc.total_flight > 0) &&
	    (stcb->asoc.stream_queue_cnt < SCTP_MAX_DATA_BUNDLING) &&
	    (un_sent < (int)(stcb->asoc.smallest_mtu - SCTP_MIN_OVERHEAD))) {
		




		if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_NAGLE_LOGGING_ENABLE) {
			sctp_log_nagle_event(stcb, SCTP_NAGLE_APPLIED);
		}
		SCTP_STAT_INCR(sctps_naglequeued);
		nagle_applies = 1;
	} else {
		if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_NAGLE_LOGGING_ENABLE) {
			if (sctp_is_feature_off(inp, SCTP_PCB_FLAGS_NODELAY))
				sctp_log_nagle_event(stcb, SCTP_NAGLE_SKIPPED);
		}
		SCTP_STAT_INCR(sctps_naglesent);
		nagle_applies = 0;
	}
	if (SCTP_BASE_SYSCTL(sctp_logging_level) & SCTP_BLK_LOGGING_ENABLE) {
		sctp_misc_ints(SCTP_CWNDLOG_PRESEND, queue_only_for_init, queue_only,
		               nagle_applies, un_sent);
		sctp_misc_ints(SCTP_CWNDLOG_PRESEND, stcb->asoc.total_output_queue_size,
		               stcb->asoc.total_flight,
		               stcb->asoc.chunks_on_out_queue, stcb->asoc.total_flight_count);
	}
	if ((queue_only == 0) && (nagle_applies == 0) && (stcb->asoc.peers_rwnd && un_sent)) {
		
		if (hold_tcblock == 0) {
			
			if (SCTP_TCB_TRYLOCK(stcb)) {
				sctp_chunk_output(inp, stcb, SCTP_OUTPUT_FROM_USR_SEND, SCTP_SO_LOCKED);
				hold_tcblock = 1;
			}
		} else {
			sctp_chunk_output(inp, stcb, SCTP_OUTPUT_FROM_USR_SEND, SCTP_SO_LOCKED);
		}
	} else if ((queue_only == 0) &&
	           (stcb->asoc.peers_rwnd == 0) &&
	           (stcb->asoc.total_flight == 0)) {
		
		if (hold_tcblock == 0) {
			hold_tcblock = 1;
			SCTP_TCB_LOCK(stcb);
		}
		sctp_chunk_output(inp, stcb, SCTP_OUTPUT_FROM_USR_SEND, SCTP_SO_LOCKED);
	} else if (some_on_control) {
		int num_out, reason, frag_point;

		
		if (hold_tcblock == 0) {
			hold_tcblock = 1;
			SCTP_TCB_LOCK(stcb);
		}
		frag_point = sctp_get_frag_point(stcb, &stcb->asoc);
		(void)sctp_med_chunk_output(inp, stcb, &stcb->asoc, &num_out,
		                            &reason, 1, 1, &now, &now_filled, frag_point, SCTP_SO_LOCKED);
	}
	SCTPDBG(SCTP_DEBUG_OUTPUT1, "USR Send complete qo:%d prw:%d unsent:%d tf:%d cooq:%d toqs:%d err:%d\n",
	        queue_only, stcb->asoc.peers_rwnd, un_sent,
		stcb->asoc.total_flight, stcb->asoc.chunks_on_out_queue,
	        stcb->asoc.total_output_queue_size, error);

out:
#if defined(__APPLE__)
	sbunlock(&so->so_snd, 1);
#endif
out_unlocked:

	if (local_soresv && stcb) {
		atomic_subtract_int(&stcb->asoc.sb_send_resv, sndlen);
	}
	if (create_lock_applied) {
		SCTP_ASOC_CREATE_UNLOCK(inp);
	}
	if ((stcb) && hold_tcblock) {
		SCTP_TCB_UNLOCK(stcb);
	}
	if (stcb && free_cnt_applied) {
		atomic_add_int(&stcb->asoc.refcnt, -1);
	}
#ifdef INVARIANTS
#if !defined(__APPLE__)
	if (stcb) {
		if (mtx_owned(&stcb->tcb_mtx)) {
			panic("Leaving with tcb mtx owned?");
		}
		if (mtx_owned(&stcb->tcb_send_mtx)) {
			panic("Leaving with tcb send mtx owned?");
		}
	}
#endif
#endif
#ifdef __Panda__
	








	if (top) {
		if ((error == EAGAIN) || (error == ENOMEM)) {
			SCTP_ATTACH_CHAIN(i_pak, top, sndlen);
			top = NULL;
		} else {
			(void)SCTP_RELEASE_HEADER(i_pak);
		}
	} else {
		



		if (i_pak) {
			(void)SCTP_RELEASE_HEADER(i_pak);
		}
	}
#endif
#ifdef INVARIANTS
	if (inp) {
		sctp_validate_no_locks(inp);
	} else {
		SCTP_PRINTF("Warning - inp is NULL so cant validate locks\n");
	}
#endif
	if (top) {
		sctp_m_freem(top);
	}
	if (control) {
		sctp_m_freem(control);
	}
	return (error);
}





struct mbuf *
sctp_add_auth_chunk(struct mbuf *m, struct mbuf **m_end,
    struct sctp_auth_chunk **auth_ret, uint32_t * offset,
    struct sctp_tcb *stcb, uint8_t chunk)
{
	struct mbuf *m_auth;
	struct sctp_auth_chunk *auth;
	int chunk_len;
	struct mbuf *cn;

	if ((m_end == NULL) || (auth_ret == NULL) || (offset == NULL) ||
	    (stcb == NULL))
		return (m);

	
	if (SCTP_BASE_SYSCTL(sctp_auth_disable))
		return (m);

	
	if (!stcb->asoc.peer_supports_auth) {
		return (m);
	}
	
	if (!sctp_auth_is_required_chunk(chunk, stcb->asoc.peer_auth_chunks)) {
		return (m);
	}
	m_auth = sctp_get_mbuf_for_msg(sizeof(*auth), 0, M_NOWAIT, 1, MT_HEADER);
	if (m_auth == NULL) {
		
		return (m);
	}
	
	if (m == NULL)
		SCTP_BUF_RESV_UF(m_auth, SCTP_MIN_OVERHEAD);
	
	auth = mtod(m_auth, struct sctp_auth_chunk *);
	bzero(auth, sizeof(*auth));
	auth->ch.chunk_type = SCTP_AUTHENTICATION;
	auth->ch.chunk_flags = 0;
	chunk_len = sizeof(*auth) +
	    sctp_get_hmac_digest_len(stcb->asoc.peer_hmac_id);
	auth->ch.chunk_length = htons(chunk_len);
	auth->hmac_id = htons(stcb->asoc.peer_hmac_id);
	

	
	*offset = 0;
	for (cn = m; cn; cn = SCTP_BUF_NEXT(cn)) {
		*offset += SCTP_BUF_LEN(cn);
	}

	
	SCTP_BUF_LEN(m_auth) = chunk_len;
	m = sctp_copy_mbufchain(m_auth, m, m_end, 1, chunk_len, 0);
	if (auth_ret != NULL)
		*auth_ret = auth;

	return (m);
}

#if defined(__FreeBSD__)  || defined(__APPLE__)
#ifdef INET6
int
sctp_v6src_match_nexthop(struct sockaddr_in6 *src6, sctp_route_t *ro)
{
	struct nd_prefix *pfx = NULL;
	struct nd_pfxrouter *pfxrtr = NULL;
	struct sockaddr_in6 gw6;

	if (ro == NULL || ro->ro_rt == NULL || src6->sin6_family != AF_INET6)
		return (0);

	
	LIST_FOREACH(pfx, &MODULE_GLOBAL(nd_prefix), ndpr_entry) {
		if (pfx->ndpr_stateflags & NDPRF_DETACHED)
			continue;
		if (IN6_ARE_MASKED_ADDR_EQUAL(&pfx->ndpr_prefix.sin6_addr,
		    &src6->sin6_addr, &pfx->ndpr_mask))
			break;
	}
	
	if (pfx == NULL) {
		SCTPDBG(SCTP_DEBUG_OUTPUT2, "No prefix entry for ");
		SCTPDBG_ADDR(SCTP_DEBUG_OUTPUT2, (struct sockaddr *)src6);
		return (0);
	}

	SCTPDBG(SCTP_DEBUG_OUTPUT2, "v6src_match_nexthop(), Prefix entry is ");
	SCTPDBG_ADDR(SCTP_DEBUG_OUTPUT2, (struct sockaddr *)src6);

	
	LIST_FOREACH(pfxrtr, &pfx->ndpr_advrtrs, pfr_entry) {
		memset(&gw6, 0, sizeof(struct sockaddr_in6));
		gw6.sin6_family = AF_INET6;
#ifdef HAVE_SIN6_LEN
		gw6.sin6_len = sizeof(struct sockaddr_in6);
#endif
		memcpy(&gw6.sin6_addr, &pfxrtr->router->rtaddr,
		    sizeof(struct in6_addr));
		SCTPDBG(SCTP_DEBUG_OUTPUT2, "prefix router is ");
		SCTPDBG_ADDR(SCTP_DEBUG_OUTPUT2, (struct sockaddr *)&gw6);
		SCTPDBG(SCTP_DEBUG_OUTPUT2, "installed router is ");
		SCTPDBG_ADDR(SCTP_DEBUG_OUTPUT2, ro->ro_rt->rt_gateway);
		if (sctp_cmpaddr((struct sockaddr *)&gw6,
				ro->ro_rt->rt_gateway)) {
			SCTPDBG(SCTP_DEBUG_OUTPUT2, "pfxrouter is installed\n");
			return (1);
		}
	}
	SCTPDBG(SCTP_DEBUG_OUTPUT2, "pfxrouter is not installed\n");
	return (0);
}
#endif

int
sctp_v4src_match_nexthop(struct sctp_ifa *sifa, sctp_route_t *ro)
{
#ifdef INET
	struct sockaddr_in *sin, *mask;
	struct ifaddr *ifa;
	struct in_addr srcnetaddr, gwnetaddr;

	if (ro == NULL || ro->ro_rt == NULL ||
	    sifa->address.sa.sa_family != AF_INET) {
		return (0);
	}
	ifa = (struct ifaddr *)sifa->ifa;
	mask = (struct sockaddr_in *)(ifa->ifa_netmask);
	sin = (struct sockaddr_in *)&sifa->address.sin;
	srcnetaddr.s_addr = (sin->sin_addr.s_addr & mask->sin_addr.s_addr);
	SCTPDBG(SCTP_DEBUG_OUTPUT1, "match_nexthop4: src address is ");
	SCTPDBG_ADDR(SCTP_DEBUG_OUTPUT2, &sifa->address.sa);
	SCTPDBG(SCTP_DEBUG_OUTPUT1, "network address is %x\n", srcnetaddr.s_addr);

	sin = (struct sockaddr_in *)ro->ro_rt->rt_gateway;
	gwnetaddr.s_addr = (sin->sin_addr.s_addr & mask->sin_addr.s_addr);
	SCTPDBG(SCTP_DEBUG_OUTPUT1, "match_nexthop4: nexthop is ");
	SCTPDBG_ADDR(SCTP_DEBUG_OUTPUT2, ro->ro_rt->rt_gateway);
	SCTPDBG(SCTP_DEBUG_OUTPUT1, "network address is %x\n", gwnetaddr.s_addr);
	if (srcnetaddr.s_addr == gwnetaddr.s_addr) {
		return (1);
	}
#endif
	return (0);
}
#elif defined(__Userspace__)

int
sctp_v6src_match_nexthop(struct sockaddr_in6 *src6, sctp_route_t *ro)
{
    return (0);
}
int
sctp_v4src_match_nexthop(struct sctp_ifa *sifa, sctp_route_t *ro)
{
    return (0);
}

#endif
