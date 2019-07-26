













#ifndef RTP_H
#define RTP_H

#include "typedefs.h"

#include "codec_db.h"

typedef struct
{
    uint16_t seqNumber;
    uint32_t timeStamp;
    uint32_t ssrc;
    int payloadType;
    const int16_t *payload;
    int16_t payloadLen;
    int16_t starts_byte1;
    int16_t rcuPlCntr;
} RTPPacket_t;

















int WebRtcNetEQ_RTPPayloadInfo(int16_t* pw16_Datagram, int i_DatagramLen,
                               RTPPacket_t* RTPheader);























int WebRtcNetEQ_RedundancySplit(RTPPacket_t* RTPheader[], int i_MaximumPayloads,
                                int *i_No_Of_Payloads);

#endif
