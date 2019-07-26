













#include "rtp.h"

#include "typedefs.h" 

#include "neteq_error_codes.h"

int WebRtcNetEQ_RTPPayloadInfo(WebRtc_Word16* pw16_Datagram, int i_DatagramLen,
                               RTPPacket_t* RTPheader)
{
    int i_P, i_X, i_CC, i_startPosition;
    int i_IPver;
    int i_extlength = -1; 
    int i_padlength = 0; 

    if (i_DatagramLen < 12)
    {
        return RTP_TOO_SHORT_PACKET;
    }

#ifdef WEBRTC_BIG_ENDIAN
    i_IPver = (((WebRtc_UWord16) (pw16_Datagram[0] & 0xC000)) >> 14); 
    i_P = (((WebRtc_UWord16) (pw16_Datagram[0] & 0x2000)) >> 13); 
    i_X = (((WebRtc_UWord16) (pw16_Datagram[0] & 0x1000)) >> 12); 
    i_CC = ((WebRtc_UWord16) (pw16_Datagram[0] >> 8) & 0xF); 
    RTPheader->payloadType = pw16_Datagram[0] & 0x7F; 
    RTPheader->seqNumber = pw16_Datagram[1]; 
    RTPheader->timeStamp = ((((WebRtc_UWord32) ((WebRtc_UWord16) pw16_Datagram[2])) << 16)
        | (WebRtc_UWord16) (pw16_Datagram[3])); 
    RTPheader->ssrc = (((WebRtc_UWord32) pw16_Datagram[4]) << 16)
        + (((WebRtc_UWord32) pw16_Datagram[5])); 

    if (i_X == 1)
    {
        
        i_extlength = pw16_Datagram[7 + 2 * i_CC];
    }
    if (i_P == 1)
    {
        
        if (i_DatagramLen & 0x1)
        {
            
            i_padlength = (((WebRtc_UWord16) pw16_Datagram[i_DatagramLen >> 1]) >> 8);
        }
        else
        {
            
            i_padlength = ((pw16_Datagram[(i_DatagramLen >> 1) - 1]) & 0xFF);
        }
    }
#else 
    i_IPver = (((WebRtc_UWord16) (pw16_Datagram[0] & 0xC0)) >> 6); 
    i_P = (((WebRtc_UWord16) (pw16_Datagram[0] & 0x20)) >> 5); 
    i_X = (((WebRtc_UWord16) (pw16_Datagram[0] & 0x10)) >> 4); 
    i_CC = (WebRtc_UWord16) (pw16_Datagram[0] & 0xF); 
    RTPheader->payloadType = (pw16_Datagram[0] >> 8) & 0x7F; 
    RTPheader->seqNumber = (((((WebRtc_UWord16) pw16_Datagram[1]) >> 8) & 0xFF)
        | (((WebRtc_UWord16) (pw16_Datagram[1] & 0xFF)) << 8)); 
    RTPheader->timeStamp = ((((WebRtc_UWord16) pw16_Datagram[2]) & 0xFF) << 24)
        | ((((WebRtc_UWord16) pw16_Datagram[2]) & 0xFF00) << 8)
        | ((((WebRtc_UWord16) pw16_Datagram[3]) >> 8) & 0xFF)
        | ((((WebRtc_UWord16) pw16_Datagram[3]) & 0xFF) << 8); 
    RTPheader->ssrc = ((((WebRtc_UWord16) pw16_Datagram[4]) & 0xFF) << 24)
        | ((((WebRtc_UWord16) pw16_Datagram[4]) & 0xFF00) << 8)
        | ((((WebRtc_UWord16) pw16_Datagram[5]) >> 8) & 0xFF)
        | ((((WebRtc_UWord16) pw16_Datagram[5]) & 0xFF) << 8); 

    if (i_X == 1)
    {
        
        i_extlength = (((((WebRtc_UWord16) pw16_Datagram[7 + 2 * i_CC]) >> 8) & 0xFF)
            | (((WebRtc_UWord16) (pw16_Datagram[7 + 2 * i_CC] & 0xFF)) << 8));
    }
    if (i_P == 1)
    {
        
        if (i_DatagramLen & 0x1)
        {
            
            i_padlength = (pw16_Datagram[i_DatagramLen >> 1] & 0xFF);
        }
        else
        {
            
            i_padlength = (((WebRtc_UWord16) pw16_Datagram[(i_DatagramLen >> 1) - 1]) >> 8);
        }
    }
#endif

    i_startPosition = 12 + 4 * (i_extlength + 1) + 4 * i_CC;
    RTPheader->payload = &pw16_Datagram[i_startPosition >> 1];
    RTPheader->payloadLen = i_DatagramLen - i_startPosition - i_padlength;
    RTPheader->starts_byte1 = 0;

    if ((i_IPver != 2) || (RTPheader->payloadLen <= 0) || (RTPheader->payloadLen >= 16000)
        || (i_startPosition < 12) || (i_startPosition > i_DatagramLen))
    {
        return RTP_CORRUPT_PACKET;
    }

    return 0;
}

#ifdef NETEQ_RED_CODEC

int WebRtcNetEQ_RedundancySplit(RTPPacket_t* RTPheader[], int i_MaximumPayloads,
                                int *i_No_Of_Payloads)
{
    const WebRtc_Word16 *pw16_data = RTPheader[0]->payload; 
    WebRtc_UWord16 uw16_offsetTimeStamp = 65535, uw16_secondPayload = 65535;
    int i_blockLength, i_k;
    int i_discardedBlockLength = 0;
    int singlePayload = 0;

#ifdef WEBRTC_BIG_ENDIAN
    if ((pw16_data[0] & 0x8000) == 0)
    {
        
        singlePayload = 1;
        
        i_blockLength = -4;
        RTPheader[0]->payloadType = ((((WebRtc_UWord16)pw16_data[0]) & 0x7F00) >> 8);
    }
    else
    {
        
        while (((pw16_data[2] & 0x8000) != 0) &&
            (pw16_data<((RTPheader[0]->payload)+((RTPheader[0]->payloadLen+1)>>1))))
        {
            i_discardedBlockLength += (4+(((WebRtc_UWord16)pw16_data[1]) & 0x3FF));
            pw16_data+=2;
        }
        if (pw16_data>=(RTPheader[0]->payload+((RTPheader[0]->payloadLen+1)>>1)))
        {
            return RED_SPLIT_ERROR2; 
        }
        singlePayload = 0; 
        uw16_secondPayload = ((((WebRtc_UWord16)pw16_data[0]) & 0x7F00) >> 8);
        RTPheader[0]->payloadType = ((((WebRtc_UWord16)pw16_data[2]) & 0x7F00) >> 8);
        uw16_offsetTimeStamp = ((((WebRtc_UWord16)pw16_data[0]) & 0xFF) << 6) +
        ((((WebRtc_UWord16)pw16_data[1]) & 0xFC00) >> 10);
        i_blockLength = (((WebRtc_UWord16)pw16_data[1]) & 0x3FF);
    }
#else 
    if ((pw16_data[0] & 0x80) == 0)
    {
        
        singlePayload = 1;
        
        i_blockLength = -4;
        RTPheader[0]->payloadType = (((WebRtc_UWord16) pw16_data[0]) & 0x7F);
    }
    else
    {
        
        while (((pw16_data[2] & 0x80) != 0) && (pw16_data < ((RTPheader[0]->payload)
            + ((RTPheader[0]->payloadLen + 1) >> 1))))
        {
            i_discardedBlockLength += (4 + ((((WebRtc_UWord16) pw16_data[1]) & 0x3) << 8)
                + ((((WebRtc_UWord16) pw16_data[1]) & 0xFF00) >> 8));
            pw16_data += 2;
        }
        if (pw16_data >= (RTPheader[0]->payload + ((RTPheader[0]->payloadLen + 1) >> 1)))
        {
            return RED_SPLIT_ERROR2; ;
        }
        singlePayload = 0; 
        uw16_secondPayload = (((WebRtc_UWord16) pw16_data[0]) & 0x7F);
        RTPheader[0]->payloadType = (((WebRtc_UWord16) pw16_data[2]) & 0x7F);
        uw16_offsetTimeStamp = ((((WebRtc_UWord16) pw16_data[0]) & 0xFF00) >> 2)
            + ((((WebRtc_UWord16) pw16_data[1]) & 0xFC) >> 2);
        i_blockLength = ((((WebRtc_UWord16) pw16_data[1]) & 0x3) << 8)
            + ((((WebRtc_UWord16) pw16_data[1]) & 0xFF00) >> 8);
    }
#endif

    if (i_MaximumPayloads < 2 || singlePayload == 1)
    {
        
        for (i_k = 1; i_k < i_MaximumPayloads; i_k++)
        {
            RTPheader[i_k]->payloadType = -1;
            RTPheader[i_k]->payloadLen = 0;
        }

        
        pw16_data = &pw16_data[(5 + i_blockLength) >> 1];
        RTPheader[0]->starts_byte1 = (5 + i_blockLength) & 0x1;
        RTPheader[0]->payloadLen = RTPheader[0]->payloadLen - (i_blockLength + 5)
            - i_discardedBlockLength;
        RTPheader[0]->payload = pw16_data;

        *i_No_Of_Payloads = 1;

    }
    else
    {
        
        RTPheader[1]->payloadType = uw16_secondPayload;
        RTPheader[1]->payload = &pw16_data[5 >> 1];
        RTPheader[1]->starts_byte1 = 5 & 0x1;
        RTPheader[1]->seqNumber = RTPheader[0]->seqNumber;
        RTPheader[1]->timeStamp = RTPheader[0]->timeStamp - uw16_offsetTimeStamp;
        RTPheader[1]->ssrc = RTPheader[0]->ssrc;
        RTPheader[1]->payloadLen = i_blockLength;

        
        RTPheader[0]->payload = &pw16_data[(5 + i_blockLength) >> 1];
        RTPheader[0]->starts_byte1 = (5 + i_blockLength) & 0x1;
        RTPheader[0]->payloadLen = RTPheader[0]->payloadLen - (i_blockLength + 5)
            - i_discardedBlockLength;

        
        for (i_k = 2; i_k < i_MaximumPayloads; i_k++)
        {
            RTPheader[i_k]->payloadType = -1;
            RTPheader[i_k]->payloadLen = 0;
        }

        *i_No_Of_Payloads = 2;
    }
    return 0;
}

#endif

