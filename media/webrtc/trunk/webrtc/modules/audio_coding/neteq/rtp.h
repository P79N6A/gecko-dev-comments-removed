













#ifndef RTP_H
#define RTP_H

#include "typedefs.h"

#include "codec_db.h"

typedef struct
{
    WebRtc_UWord16 seqNumber;
    WebRtc_UWord32 timeStamp;
    WebRtc_UWord32 ssrc;
    int payloadType;
    const WebRtc_Word16 *payload;
    WebRtc_Word16 payloadLen;
    WebRtc_Word16 starts_byte1;
    WebRtc_Word16 rcuPlCntr;
} RTPPacket_t;

















int WebRtcNetEQ_RTPPayloadInfo(WebRtc_Word16* pw16_Datagram, int i_DatagramLen,
                               RTPPacket_t* RTPheader);























int WebRtcNetEQ_RedundancySplit(RTPPacket_t* RTPheader[], int i_MaximumPayloads,
                                int *i_No_Of_Payloads);

#endif
