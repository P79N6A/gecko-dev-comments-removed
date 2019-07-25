













#ifndef BUFFER_STATS_H
#define BUFFER_STATS_H

#include "automode.h"
#include "webrtc_neteq.h" 


#define BUFSTATS_DO_NORMAL					0
#define BUFSTATS_DO_ACCELERATE				1
#define BUFSTATS_DO_MERGE					2
#define BUFSTATS_DO_EXPAND					3
#define BUFSTAT_REINIT						4
#define BUFSTATS_DO_RFC3389CNG_PACKET		5
#define BUFSTATS_DO_RFC3389CNG_NOPACKET		6
#define BUFSTATS_DO_INTERNAL_CNG_NOPACKET	7
#define BUFSTATS_DO_PREEMPTIVE_EXPAND		8
#define BUFSTAT_REINIT_DECODER              9
#define BUFSTATS_DO_DTMF_ONLY               10

#define BUFSTATS_DO_ALTERNATIVE_PLC				   11
#define BUFSTATS_DO_ALTERNATIVE_PLC_INC_TS		   12
#define BUFSTATS_DO_AUDIO_REPETITION			   13
#define BUFSTATS_DO_AUDIO_REPETITION_INC_TS		   14


#define REINIT_AFTER_EXPANDS 100


#define MAX_WAIT_FOR_PACKET 10


#define CNG_OFF 0
#define CNG_RFC3389_ON 1
#define CNG_INTERNAL_ON 2

typedef struct
{

    
    WebRtc_Word16 w16_cngOn; 
    WebRtc_Word16 w16_noExpand;
    WebRtc_Word32 uw32_CNGplayedTS;

    
    WebRtc_UWord16 avgDelayMsQ8;
    WebRtc_Word16 maxDelayMs;

    AutomodeInst_t Automode_inst;

} BufstatsInst_t;





















WebRtc_UWord16 WebRtcNetEQ_BufstatsDecision(BufstatsInst_t *inst, WebRtc_Word16 frameSize,
                                            WebRtc_Word32 cur_size, WebRtc_UWord32 targetTS,
                                            WebRtc_UWord32 availableTS, int noPacket,
                                            int cngPacket, int prevPlayMode,
                                            enum WebRtcNetEQPlayoutMode playoutMode,
                                            int timestampsPerCall, int NoOfExpandCalls,
                                            WebRtc_Word16 fs_mult,
                                            WebRtc_Word16 lastModeBGNonly, int playDtmf);

#endif
