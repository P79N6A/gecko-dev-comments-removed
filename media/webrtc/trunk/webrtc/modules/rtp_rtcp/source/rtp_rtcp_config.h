









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_RTCP_CONFIG_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_RTCP_CONFIG_H_


namespace webrtc {
enum { kRtpRtcpMaxIdleTimeProcess = 5,
       kRtpRtcpBitrateProcessTimeMs = 10,
       kRtpRtcpPacketTimeoutProcessTimeMs = 100 };

enum { NACK_PACKETS_MAX_SIZE    = 256 }; 
enum { NACK_BYTECOUNT_SIZE      = 60};   

enum { RTCP_INTERVAL_VIDEO_MS       = 1000 };
enum { RTCP_INTERVAL_AUDIO_MS       = 5000 };
enum { RTCP_SEND_BEFORE_KEY_FRAME_MS= 100 };
enum { RTCP_MAX_REPORT_BLOCKS       = 31};      
enum { RTCP_MIN_FRAME_LENGTH_MS     = 17};
enum { kRtcpAppCode_DATA_SIZE           = 32*4};    
enum { RTCP_RPSI_DATA_SIZE          = 30};
enum { RTCP_NUMBER_OF_SR            = 60 };

enum { MAX_NUMBER_OF_TEMPORAL_ID    = 8 };          
enum { MAX_NUMBER_OF_DEPENDENCY_QUALITY_ID  = 128 };
enum { MAX_NUMBER_OF_REMB_FEEDBACK_SSRCS = 255 };

enum { BW_HISTORY_SIZE          = 35};

#define MIN_AUDIO_BW_MANAGEMENT_BITRATE   6
#define MIN_VIDEO_BW_MANAGEMENT_BITRATE   30

enum { DTMF_OUTBAND_MAX         = 20};

enum { RTP_MAX_BURST_SLEEP_TIME = 500 };
enum { RTP_AUDIO_LEVEL_UNIQUE_ID = 0xbede };
enum { RTP_MAX_PACKETS_PER_FRAME= 512 }; 
} 


#endif 
