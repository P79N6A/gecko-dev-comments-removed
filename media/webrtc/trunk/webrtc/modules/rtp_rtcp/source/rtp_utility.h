









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_UTILITY_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_RTP_UTILITY_H_

#include <stddef.h> 

#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp_defines.h"
#include "webrtc/modules/rtp_rtcp/interface/receive_statistics.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_header_extension.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_rtcp_config.h"
#include "webrtc/typedefs.h"

namespace webrtc {

const uint8_t kRtpMarkerBitMask = 0x80;

RtpData* NullObjectRtpData();
RtpFeedback* NullObjectRtpFeedback();
RtpAudioFeedback* NullObjectRtpAudioFeedback();
ReceiveStatistics* NullObjectReceiveStatistics();

namespace RtpUtility {
    
    const uint32_t NTP_JAN_1970 = 2208988800UL;

    
    const double NTP_FRAC = 4.294967296E+9;

    struct Payload
    {
        char name[RTP_PAYLOAD_NAME_SIZE];
        bool audio;
        PayloadUnion typeSpecific;
    };

    typedef std::map<int8_t, Payload*> PayloadTypeMap;

    
    
    uint32_t GetCurrentRTP(Clock* clock, uint32_t freq);

    
    uint32_t ConvertNTPTimeToRTP(uint32_t NTPsec,
                                 uint32_t NTPfrac,
                                 uint32_t freq);

    uint32_t pow2(uint8_t exp);

    
    
    
    bool OldTimestamp(uint32_t newTimestamp,
                      uint32_t existingTimestamp,
                      bool* wrapped);

    bool StringCompare(const char* str1,
                       const char* str2,
                       const uint32_t length);

    void AssignUWord32ToBuffer(uint8_t* dataBuffer, uint32_t value);
    void AssignUWord24ToBuffer(uint8_t* dataBuffer, uint32_t value);
    void AssignUWord16ToBuffer(uint8_t* dataBuffer, uint16_t value);

    




    uint16_t BufferToUWord16(const uint8_t* dataBuffer);

    




    uint32_t BufferToUWord24(const uint8_t* dataBuffer);

    




    uint32_t BufferToUWord32(const uint8_t* dataBuffer);

    class RtpHeaderParser {
    public:
     RtpHeaderParser(const uint8_t* rtpData, size_t rtpDataLength);
     ~RtpHeaderParser();

        bool RTCP() const;
        bool ParseRtcp(RTPHeader* header) const;
        bool Parse(RTPHeader& parsedPacket,
                   RtpHeaderExtensionMap* ptrExtensionMap = NULL) const;

    private:
        void ParseOneByteExtensionHeader(
            RTPHeader& parsedPacket,
            const RtpHeaderExtensionMap* ptrExtensionMap,
            const uint8_t* ptrRTPDataExtensionEnd,
            const uint8_t* ptr) const;

        uint8_t ParsePaddingBytes(
            const uint8_t* ptrRTPDataExtensionEnd,
            const uint8_t* ptr) const;

        const uint8_t* const _ptrRTPDataBegin;
        const uint8_t* const _ptrRTPDataEnd;
    };
}  
}  

#endif 
