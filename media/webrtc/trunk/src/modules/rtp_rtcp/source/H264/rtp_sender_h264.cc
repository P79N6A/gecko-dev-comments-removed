









#include "rtp_sender_h264.h"

#include "rtp_utility.h"

namespace webrtc {
RTPSenderH264::RTPSenderH264(RTPSenderInterface* rtpSender) :
    
    _rtpSender(*rtpSender),
    _h264Mode(H264_SINGLE_NAL_MODE),
    _h264SendPPS_SPS(true),
    _h264SVCPayloadType(-1),
    _h264SVCRelaySequenceNumber(0),
    _h264SVCRelayTimeStamp(0),
    _h264SVCRelayLayerComplete(false),

    _useHighestSendLayer(false),
    _highestDependencyLayerOld(MAX_NUMBER_OF_TEMPORAL_ID-1),
    _highestDependencyQualityIDOld(MAX_NUMBER_OF_DEPENDENCY_QUALITY_ID-1),
    _highestDependencyLayer(0),
    _highestDependencyQualityID(0),
    _highestTemporalLayer(0)
{
}

RTPSenderH264::~RTPSenderH264()
{
}

WebRtc_Word32
RTPSenderH264::Init()
{
    _h264SendPPS_SPS = true;
    _h264Mode = H264_SINGLE_NAL_MODE;
    return 0;
}


























bool
RTPSenderH264::AddH264SVCNALUHeader(const H264_SVC_NALUHeader& svc,
                                    WebRtc_UWord8* databuffer,
                                    WebRtc_Word32& curByte) const
{
   
   
   
   
   

   
   
   
   
   
   
   
   
   
   
   

   
   databuffer[curByte++] = (svc.r << 7)              + (svc.idr << 6)           + (svc.priorityID & 0x3F);
   databuffer[curByte++] = (svc.interLayerPred << 7) + (svc.dependencyID << 4)  + (svc.qualityID & 0x0F);
   databuffer[curByte++] = (svc.temporalID << 5)     + (svc.useRefBasePic << 4) + (svc.discardable << 3) +
                           (svc.output << 2)         + (svc.rr & 0x03);
   return true;
}

WebRtc_Word32
RTPSenderH264::AddH264PACSINALU(const bool firstPacketInNALU,
                                const bool lastPacketInNALU,
                                const H264_PACSI_NALU& pacsi,
                                const H264_SVC_NALUHeader& svc,
                                const WebRtc_UWord16 DONC,
                                WebRtc_UWord8* databuffer,
                                WebRtc_Word32& curByte) const
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    


    
    
    
    

    
    const bool addDONC = false;

    if (svc.length == 0 || pacsi.NALlength == 0)
    {
      return 0;
    }

    WebRtc_Word32 startByte = curByte;

    
    databuffer[curByte++] = 30; 

    
    AddH264SVCNALUHeader(svc, databuffer, curByte);

    
    databuffer[curByte++] = (pacsi.X << 7) +
                            (pacsi.Y << 6) +
                            (addDONC << 5) +
                            (pacsi.A << 4) +
                            (pacsi.P << 3) +
                            (pacsi.C << 2) +
                            firstPacketInNALU?(pacsi.S << 1):0 +
                            lastPacketInNALU?(pacsi.E):0;

    
    if (pacsi.Y)
    {
        databuffer[curByte++] = pacsi.TL0picIDx;
        databuffer[curByte++] = (WebRtc_UWord8)(pacsi.IDRpicID >> 8);
        databuffer[curByte++] = (WebRtc_UWord8)(pacsi.IDRpicID);
    }
    
    if (addDONC) 
    {
        databuffer[curByte++] = (WebRtc_UWord8)(DONC >> 8);
        databuffer[curByte++] = (WebRtc_UWord8)(DONC);
    }

    
    if(firstPacketInNALU) 
    {
        
        for (WebRtc_UWord32 i = 0; i < pacsi.numSEINALUs; i++)
        {
            
            databuffer[curByte++] = (WebRtc_UWord8)(pacsi.seiMessageLength[i] >> 8);
            databuffer[curByte++] = (WebRtc_UWord8)(pacsi.seiMessageLength[i]);

            
            memcpy(databuffer + curByte, pacsi.seiMessageData[i], pacsi.seiMessageLength[i]);
            curByte += pacsi.seiMessageLength[i];
        }
    }
    return curByte - startByte;
}

WebRtc_Word32
RTPSenderH264::SetH264RelaySequenceNumber(const WebRtc_UWord16 seqNum)
{
    _h264SVCRelaySequenceNumber = seqNum;
    return 0;
}

WebRtc_Word32
RTPSenderH264::SetH264RelayCompleteLayer(const bool complete)
{
    _h264SVCRelayLayerComplete = complete;
    return 0;
}








WebRtc_Word32
RTPSenderH264::SendH264FillerData(const WebRtcRTPHeader* rtpHeader,
                                  const WebRtc_UWord16 bytesToSend,
                                  const WebRtc_UWord32 ssrc)
{
    WebRtc_UWord16 fillerLength = bytesToSend - 12 - 1;

    if (fillerLength > WEBRTC_IP_PACKET_SIZE - 12 - 1)
    {
        return 0;
    }

    if (fillerLength == 0)
    {
        
        fillerLength = 1;
    }

    
    WebRtc_UWord8 dataBuffer[WEBRTC_IP_PACKET_SIZE];

    dataBuffer[0] = static_cast<WebRtc_UWord8>(0x80);            
    dataBuffer[1] = rtpHeader->header.payloadType;
    ModuleRTPUtility::AssignUWord16ToBuffer(dataBuffer+2, _rtpSender.IncrementSequenceNumber()); 
    ModuleRTPUtility::AssignUWord32ToBuffer(dataBuffer+4, rtpHeader->header.timestamp);
    ModuleRTPUtility::AssignUWord32ToBuffer(dataBuffer+8, rtpHeader->header.ssrc);

    
    dataBuffer[12] = 12;        

    
    memset(dataBuffer + 12 + 1, 0xff, fillerLength);

    return _rtpSender.SendToNetwork(dataBuffer,
                        fillerLength,
                        12 + 1);
}

WebRtc_Word32
RTPSenderH264::SendH264FillerData(const WebRtc_UWord32 captureTimestamp,
                                  const WebRtc_UWord8 payloadType,
                                  const WebRtc_UWord32 bytes
                                  )
{

    const WebRtc_UWord16 rtpHeaderLength = _rtpSender.RTPHeaderLength();
    WebRtc_UWord16 maxLength = _rtpSender.MaxPayloadLength() - FECPacketOverhead() - _rtpSender.RTPHeaderLength();

    WebRtc_Word32 bytesToSend=bytes;
    WebRtc_UWord16 fillerLength=0;

    WebRtc_UWord8 dataBuffer[WEBRTC_IP_PACKET_SIZE];

    while(bytesToSend>0)
    {
        fillerLength=maxLength;
        if(fillerLength<maxLength)
        {
            fillerLength = (WebRtc_UWord16) bytesToSend;
        }

        bytesToSend-=fillerLength;

        if (fillerLength > WEBRTC_IP_PACKET_SIZE - 12 - 1)
        {
            return 0;
        }

        if (fillerLength == 0)
        {
            
            fillerLength = 1;
        }

        
        
        _rtpSender.BuildRTPheader(dataBuffer, payloadType, false,captureTimestamp, true, true);

        
        dataBuffer[12] = 12;        

        
        
        memset(dataBuffer + 12 + 1, 0xff, fillerLength-1);

        if( _rtpSender.SendToNetwork(dataBuffer,
                            fillerLength,
                            12)<0)
        {

            return -1;;
        }
    }
    return 0;
}

WebRtc_Word32
RTPSenderH264::SendH264SVCRelayPacket(const WebRtcRTPHeader* rtpHeader,
                                      const WebRtc_UWord8* incomingRTPPacket,
                                      const WebRtc_UWord16 incomingRTPPacketSize,
                                      const WebRtc_UWord32 ssrc,
                                      const bool higestLayer)
{
    if (rtpHeader->header.sequenceNumber != (WebRtc_UWord16)(_h264SVCRelaySequenceNumber + 1))
    {
         
         _rtpSender.IncrementSequenceNumber();
    }
    _h264SVCRelaySequenceNumber = rtpHeader->header.sequenceNumber;


    if (rtpHeader->header.timestamp != _h264SVCRelayTimeStamp)
    {
        
        _h264SVCRelayLayerComplete = false;
    }

    if (rtpHeader->header.timestamp == _h264SVCRelayTimeStamp &&
        _h264SVCRelayLayerComplete)
    {
        
        
        
        return 0;
    }
    _h264SVCRelayTimeStamp = rtpHeader->header.timestamp;

    
    
    

    WebRtc_UWord8 dataBuffer[WEBRTC_IP_PACKET_SIZE];
    memcpy(dataBuffer, incomingRTPPacket, incomingRTPPacketSize);

    
    

    
    if(_h264SVCPayloadType != -1)
    {
        dataBuffer[1] &= kRtpMarkerBitMask;
        dataBuffer[1] += _h264SVCPayloadType;
    }

    
    
    ModuleRTPUtility::AssignUWord16ToBuffer(dataBuffer+2, _rtpSender.IncrementSequenceNumber()); 
    

    
    
    
    

    if(higestLayer && rtpHeader->type.Video.codecHeader.H264.relayE)
    {
        
        dataBuffer[1] |= kRtpMarkerBitMask;

        
        _h264SVCRelayLayerComplete = true;
    }
    return _rtpSender.SendToNetwork(dataBuffer,
                         incomingRTPPacketSize - rtpHeader->header.headerLength,
                         rtpHeader->header.headerLength);
}

WebRtc_Word32
RTPSenderH264::SendH264_STAP_A(const FrameType frameType,
                                const H264Info* ptrH264Info,
                                WebRtc_UWord16 &idxNALU,
                                const WebRtc_Word8 payloadType,
                                const WebRtc_UWord32 captureTimeStamp,
                                bool& switchToFUA,
                                WebRtc_Word32 &payloadBytesToSend,
                                const WebRtc_UWord8*& data,
                                const WebRtc_UWord16 rtpHeaderLength)
{
    const WebRtc_Word32 H264_NALU_LENGTH = 2;

    WebRtc_UWord16 h264HeaderLength = 1; 
    WebRtc_UWord16 maxPayloadLengthSTAP_A = _rtpSender.MaxPayloadLength() -
                                          FECPacketOverhead() - rtpHeaderLength -
                                          h264HeaderLength - H264_NALU_LENGTH;

    WebRtc_Word32 dataOffset = rtpHeaderLength + h264HeaderLength;
    WebRtc_UWord8 NRI = 0;
    WebRtc_UWord16 payloadBytesInPacket = 0;
    WebRtc_UWord8 dataBuffer[WEBRTC_IP_PACKET_SIZE];

    if (ptrH264Info->payloadSize[idxNALU] > maxPayloadLengthSTAP_A)
    {
        
        switchToFUA = true;
    } else
    {
        
        do
        {
            if(!_h264SendPPS_SPS)
            {
                
                if(ptrH264Info->type[idxNALU] == 7 || ptrH264Info->type[idxNALU] == 8)
                {
                    payloadBytesToSend -= ptrH264Info->payloadSize[idxNALU] + ptrH264Info->startCodeSize[idxNALU];
                    data += ptrH264Info->payloadSize[idxNALU] + ptrH264Info->startCodeSize[idxNALU];
                    idxNALU++;
                    continue;
                }
            }
            if(ptrH264Info->payloadSize[idxNALU] + payloadBytesInPacket <= maxPayloadLengthSTAP_A)
            {
                if(ptrH264Info->NRI[idxNALU] > NRI)
                {
                    NRI = ptrH264Info->NRI[idxNALU];
                }
                
                dataBuffer[dataOffset] = (WebRtc_UWord8)(ptrH264Info->payloadSize[idxNALU] >> 8);
                dataOffset++;
                dataBuffer[dataOffset] = (WebRtc_UWord8)(ptrH264Info->payloadSize[idxNALU] & 0xff);
                dataOffset++;
                
                memcpy(&dataBuffer[dataOffset], &data[ptrH264Info->startCodeSize[idxNALU]], ptrH264Info->payloadSize[idxNALU]);
                dataOffset += ptrH264Info->payloadSize[idxNALU];
                data += ptrH264Info->payloadSize[idxNALU] + ptrH264Info->startCodeSize[idxNALU];
                payloadBytesInPacket += (WebRtc_UWord16)(ptrH264Info->payloadSize[idxNALU] + H264_NALU_LENGTH);
                payloadBytesToSend -= ptrH264Info->payloadSize[idxNALU] + ptrH264Info->startCodeSize[idxNALU];
            } else
            {
                
                break;
            }
            idxNALU++;
        }while(payloadBytesToSend);
    }

    
    
    if (payloadBytesInPacket)
    {
        
        _rtpSender.BuildRTPheader(dataBuffer, payloadType, (payloadBytesToSend==0)?true:false, captureTimeStamp);
        dataBuffer[rtpHeaderLength] = 24 + NRI; 
        WebRtc_UWord16 payloadLength = payloadBytesInPacket + h264HeaderLength;

        if(-1 == SendVideoPacket(frameType, dataBuffer, payloadLength, rtpHeaderLength))
        {
            return -1;
        }
    }
    return 0;
} 


WebRtc_Word32
RTPSenderH264::SendH264_STAP_A_PACSI(const FrameType frameType,
                                      const H264Info* ptrH264Info,
                                      WebRtc_UWord16 &idxNALU,
                                      const WebRtc_Word8 payloadType,
                                      const WebRtc_UWord32 captureTimeStamp,
                                      bool& switchToFUA,
                                      WebRtc_Word32 &payloadBytesToSend,
                                      const WebRtc_UWord8*& data,
                                      const WebRtc_UWord16 rtpHeaderLength,
                                      WebRtc_UWord16& decodingOrderNumber)
{
    const WebRtc_Word32 H264_NALU_LENGTH = 2;

    WebRtc_UWord16 h264HeaderLength = 1; 
    WebRtc_UWord16 maxPayloadLengthSTAP_A = _rtpSender.MaxPayloadLength() - FECPacketOverhead() - rtpHeaderLength - h264HeaderLength - H264_NALU_LENGTH;
    WebRtc_Word32 dataOffset = rtpHeaderLength + h264HeaderLength;
    WebRtc_UWord8 NRI = 0;
    WebRtc_UWord16 payloadBytesInPacket = 0;
    WebRtc_UWord8 dataBuffer[WEBRTC_IP_PACKET_SIZE];
    bool firstNALUNotIDR = true; 

    
    WebRtc_Word32 lengthPACSI = 0;
    WebRtc_UWord32 PACSI_NALlength = ptrH264Info->PACSI[idxNALU].NALlength;
    if (PACSI_NALlength > maxPayloadLengthSTAP_A)
    {
        return -1;
    }
    dataBuffer[dataOffset++] = (WebRtc_UWord8)(PACSI_NALlength >> 8);
    dataBuffer[dataOffset++] = (WebRtc_UWord8)(PACSI_NALlength & 0xff);

    
    WebRtc_Word32 lengthPASCINALU = AddH264PACSINALU(true,
                                                   false,
                                                   ptrH264Info->PACSI[idxNALU],
                                                   ptrH264Info->SVCheader[idxNALU],
                           decodingOrderNumber,
                           dataBuffer,
                                                   dataOffset);
    if (lengthPASCINALU <= 0)
    {
        return -1;
    }
    decodingOrderNumber++;

    lengthPACSI = H264_NALU_LENGTH + lengthPASCINALU;
    maxPayloadLengthSTAP_A -= (WebRtc_UWord16)lengthPACSI;
    if (ptrH264Info->payloadSize[idxNALU] > maxPayloadLengthSTAP_A)
    {
        
        switchToFUA = true;
        return 0;
    }
    if(!ptrH264Info->SVCheader[idxNALU].idr)
    {
        firstNALUNotIDR = true;
    }

    WebRtc_UWord32 layer = (ptrH264Info->SVCheader[idxNALU].dependencyID << 16)+
                         (ptrH264Info->SVCheader[idxNALU].qualityID << 8) +
                          ptrH264Info->SVCheader[idxNALU].temporalID;

    {
        
        

        do
        {
            if(!_h264SendPPS_SPS)
            {
                
                
                if(ptrH264Info->type[idxNALU] == 7 || ptrH264Info->type[idxNALU] == 8)
                {
                    payloadBytesToSend -= ptrH264Info->payloadSize[idxNALU] + ptrH264Info->startCodeSize[idxNALU];
                    data += ptrH264Info->payloadSize[idxNALU] + ptrH264Info->startCodeSize[idxNALU];
                    idxNALU++;
                    continue;
                }
            }
            
            if(ptrH264Info->type[idxNALU] == 6)
            {
                
                payloadBytesToSend -= ptrH264Info->payloadSize[idxNALU] + ptrH264Info->startCodeSize[idxNALU];
                data += ptrH264Info->payloadSize[idxNALU] + ptrH264Info->startCodeSize[idxNALU];
                idxNALU++;
                continue;
            }

            const WebRtc_UWord32 layerNALU = (ptrH264Info->SVCheader[idxNALU].dependencyID << 16)+
                                           (ptrH264Info->SVCheader[idxNALU].qualityID << 8) +
                                            ptrH264Info->SVCheader[idxNALU].temporalID;

            
            if( ptrH264Info->payloadSize[idxNALU] + payloadBytesInPacket <= maxPayloadLengthSTAP_A &&
                layerNALU == layer)
            {
                if(ptrH264Info->NRI[idxNALU] > NRI)
                {
                    NRI = ptrH264Info->NRI[idxNALU];
                }
                
                dataBuffer[dataOffset] = (WebRtc_UWord8)(ptrH264Info->payloadSize[idxNALU] >> 8);
                dataOffset++;
                dataBuffer[dataOffset] = (WebRtc_UWord8)(ptrH264Info->payloadSize[idxNALU] & 0xff);
                dataOffset++;
                
                memcpy(&dataBuffer[dataOffset], &data[ptrH264Info->startCodeSize[idxNALU]], ptrH264Info->payloadSize[idxNALU]);
                dataOffset += ptrH264Info->payloadSize[idxNALU];
                data += ptrH264Info->payloadSize[idxNALU] + ptrH264Info->startCodeSize[idxNALU];
                payloadBytesInPacket += (WebRtc_UWord16)(ptrH264Info->payloadSize[idxNALU] + H264_NALU_LENGTH);
                payloadBytesToSend -= ptrH264Info->payloadSize[idxNALU] + ptrH264Info->startCodeSize[idxNALU];
            } else
            {
                
                

                
                

                if(_useHighestSendLayer && layerNALU != layer)
                {
                    
                    
                    const WebRtc_UWord8 dependencyQualityID = (ptrH264Info->SVCheader[idxNALU].dependencyID << 4) + ptrH264Info->SVCheader[idxNALU].qualityID;

                    bool highestLayer;
                    if(SendH264SVCLayer(frameType,
                                        ptrH264Info->SVCheader[idxNALU].temporalID,
                                        dependencyQualityID,
                                        highestLayer) == false)
                    {
                        
                        payloadBytesToSend = 0;
                    }
                }
                break;
            }
            idxNALU++;

        }while(payloadBytesToSend);
    }

    
    if (payloadBytesInPacket)
    {
        
        _rtpSender.BuildRTPheader(dataBuffer, payloadType, (payloadBytesToSend==0)?true:false, captureTimeStamp);

        dataBuffer[rtpHeaderLength] = 24 + NRI; 

        
        dataBuffer[rtpHeaderLength + H264_NALU_LENGTH + 1] &= 0x1f;   
        dataBuffer[rtpHeaderLength + H264_NALU_LENGTH + 1] |= NRI;

        if(ptrH264Info->PACSI[idxNALU-1].E)
        {
            
            dataBuffer[rtpHeaderLength + H264_NALU_LENGTH + 5] |= 0x01;
        }
        if(firstNALUNotIDR)
        {
            
            bool setIBit = false;
            for(int i = 0; i < idxNALU; i++)
            {
                if(ptrH264Info->SVCheader[i].idr)
                {
                    setIBit = true;
                    break;
                }
            }
            if(setIBit)
            {
                
                dataBuffer[rtpHeaderLength + H264_NALU_LENGTH + 2] |= 0x40;
            }
        }
        const WebRtc_UWord16 payloadLength = payloadBytesInPacket + h264HeaderLength + (WebRtc_UWord16)lengthPACSI;
        if(-1 == SendVideoPacket(frameType,
                                 dataBuffer,
                                 payloadLength,
                                 rtpHeaderLength,
                                 layer==0))
        {
            return -1;
        }
    }
    return 0;
} 

WebRtc_Word32
RTPSenderH264::SendH264_FU_A(const FrameType frameType,
                              const H264Info* ptrH264Info,
                              WebRtc_UWord16 &idxNALU,
                              const WebRtc_Word8 payloadType,
                              const WebRtc_UWord32 captureTimeStamp,
                              WebRtc_Word32 &payloadBytesToSend,
                              const WebRtc_UWord8*& data,
                              const WebRtc_UWord16 rtpHeaderLength,
                              WebRtc_UWord16& decodingOrderNumber,
                              const bool sendSVCPACSI)
{

    
    WebRtc_UWord16 maxPayloadLength = _rtpSender.MaxPayloadLength() - FECPacketOverhead() - rtpHeaderLength;
    WebRtc_UWord8 dataBuffer[WEBRTC_IP_PACKET_SIZE];
    WebRtc_UWord32 payloadBytesRemainingInNALU = ptrH264Info->payloadSize[idxNALU];

    bool isBaseLayer=false;

    if(payloadBytesRemainingInNALU > maxPayloadLength)
    {
        
        const WebRtc_UWord16 H264_FUA_LENGTH = 2; 

        if(sendSVCPACSI)
        {
            SendH264_SinglePACSI(frameType,
                                 ptrH264Info,
                                 idxNALU,
                                 payloadType,
                                 captureTimeStamp,
                                 true,
                                 false);

            WebRtc_UWord32 layer = (ptrH264Info->SVCheader[idxNALU].dependencyID << 16)+
                                 (ptrH264Info->SVCheader[idxNALU].qualityID << 8) +
                                  ptrH264Info->SVCheader[idxNALU].temporalID;
            isBaseLayer=(layer==0);
        }

        
        _rtpSender.BuildRTPheader(dataBuffer,payloadType, false, captureTimeStamp);

        WebRtc_UWord16 maxPayloadLengthFU_A = maxPayloadLength - H264_FUA_LENGTH ;
        WebRtc_UWord8 fuaIndc = 28 + ptrH264Info->NRI[idxNALU];
        dataBuffer[rtpHeaderLength] = fuaIndc;                                                     
        dataBuffer[rtpHeaderLength+1] = (WebRtc_UWord8)(ptrH264Info->type[idxNALU] + 0x80); 

        memcpy(&dataBuffer[rtpHeaderLength + H264_FUA_LENGTH], &data[ptrH264Info->startCodeSize[idxNALU]+1], maxPayloadLengthFU_A);
        WebRtc_UWord16 payloadLength = maxPayloadLengthFU_A + H264_FUA_LENGTH;
        if(-1 == SendVideoPacket(frameType, dataBuffer, payloadLength, rtpHeaderLength, isBaseLayer))
        {
            return -1;
        }

        
        data += maxPayloadLengthFU_A + 1 + ptrH264Info->startCodeSize[idxNALU];             
        payloadBytesToSend -= maxPayloadLengthFU_A+1+ptrH264Info->startCodeSize[idxNALU];
        payloadBytesRemainingInNALU -= maxPayloadLengthFU_A+1;

        
        while(payloadBytesRemainingInNALU  > maxPayloadLengthFU_A)
        {
            if(sendSVCPACSI)
            {
                SendH264_SinglePACSI(frameType,
                                     ptrH264Info,
                                     idxNALU,
                                     payloadType,
                                     captureTimeStamp,
                                     false,
                                     false);
            }

            
            _rtpSender.BuildRTPheader(dataBuffer, payloadType, false, captureTimeStamp);

            dataBuffer[rtpHeaderLength] = (WebRtc_UWord8)fuaIndc;           
            dataBuffer[rtpHeaderLength+1] = ptrH264Info->type[idxNALU];   

            memcpy(&dataBuffer[rtpHeaderLength+H264_FUA_LENGTH], data, maxPayloadLengthFU_A);
            payloadLength = maxPayloadLengthFU_A + H264_FUA_LENGTH;

            if(-1 == SendVideoPacket(frameType, dataBuffer, payloadLength, rtpHeaderLength,isBaseLayer))
            {
                return -1;
            }
            data += maxPayloadLengthFU_A; 
            payloadBytesToSend -= maxPayloadLengthFU_A;
            payloadBytesRemainingInNALU -= maxPayloadLengthFU_A;
            dataBuffer[rtpHeaderLength] = fuaIndc;                         
            dataBuffer[rtpHeaderLength+1] = ptrH264Info->type[idxNALU];    
        }
        if(sendSVCPACSI)
        {
            SendH264_SinglePACSI(frameType,
                                 ptrH264Info,
                                 idxNALU,
                                 payloadType,
                                 captureTimeStamp,
                                 false,
                                 true); 

            if(_useHighestSendLayer && idxNALU+1 < ptrH264Info->numNALUs)
            {
                
                

                
                const WebRtc_UWord8 dependencyQualityID = (ptrH264Info->SVCheader[idxNALU+1].dependencyID << 4) +
                                                         ptrH264Info->SVCheader[idxNALU+1].qualityID;

                bool highestLayer;
                if(SendH264SVCLayer(frameType,
                                    ptrH264Info->SVCheader[idxNALU+1].temporalID,
                                    dependencyQualityID,
                                    highestLayer) == false)
                {
                    
                    payloadBytesToSend = payloadBytesRemainingInNALU;
                }
            }
        }
        
        _rtpSender.BuildRTPheader(dataBuffer, payloadType,(payloadBytesToSend == (WebRtc_Word32)payloadBytesRemainingInNALU)?true:false, captureTimeStamp);
        dataBuffer[rtpHeaderLength+1] = ptrH264Info->type[idxNALU] + 0x40; 

        memcpy(&dataBuffer[rtpHeaderLength+H264_FUA_LENGTH], data, payloadBytesRemainingInNALU);
        payloadLength = (WebRtc_UWord16)payloadBytesRemainingInNALU + H264_FUA_LENGTH;
        payloadBytesToSend -= payloadBytesRemainingInNALU;
        if(payloadBytesToSend != 0)
        {
            data += payloadBytesRemainingInNALU; 
        }
        idxNALU++;
        if(-1 == SendVideoPacket(frameType, dataBuffer, payloadLength, rtpHeaderLength,isBaseLayer))
        {
            return -1;
        }
    } else
    {
        
        return SendH264_SingleMode(frameType,
                                   ptrH264Info,
                                   idxNALU,
                                   payloadType,
                                   captureTimeStamp,
                                   payloadBytesToSend,
                                   data,
                                   rtpHeaderLength,
                                   sendSVCPACSI);
    }
    
    return 0;
}

WebRtc_Word32
RTPSenderH264::SendH264_SingleMode(const FrameType frameType,
                                    const H264Info* ptrH264Info,
                                    WebRtc_UWord16 &idxNALU,
                                    const WebRtc_Word8 payloadType,
                                    const WebRtc_UWord32 captureTimeStamp,
                                    WebRtc_Word32 &payloadBytesToSend,
                                    const WebRtc_UWord8*& data,
                                    const WebRtc_UWord16 rtpHeaderLength,
                                    WebRtc_UWord16& decodingOrderNumber,
                                    const bool sendSVCPACSI)
{
    
    
    const WebRtc_UWord16 maxPayloadLength = WEBRTC_IP_PACKET_SIZE - _rtpSender.PacketOverHead() - FECPacketOverhead() - rtpHeaderLength;
    WebRtc_UWord8 dataBuffer[WEBRTC_IP_PACKET_SIZE];
    bool isBaseLayer=false;

    if(ptrH264Info->payloadSize[idxNALU] > maxPayloadLength)
    {
        return -3;
    }
    if(!_h264SendPPS_SPS)
    {
        
        if(ptrH264Info->type[idxNALU] == 7 || ptrH264Info->type[idxNALU] == 8)
        {
            payloadBytesToSend -= ptrH264Info->payloadSize[idxNALU] + ptrH264Info->startCodeSize[idxNALU];
            data += ptrH264Info->payloadSize[idxNALU] + ptrH264Info->startCodeSize[idxNALU];
            idxNALU++;
            return 0;
        }
    }
    if(sendSVCPACSI)
    {
        SendH264_SinglePACSI(frameType,
                             ptrH264Info,
                             idxNALU,
                             payloadType,
                             captureTimeStamp,
                             true,
                             true);

        WebRtc_UWord32 layer = (ptrH264Info->SVCheader[idxNALU].dependencyID << 16)+
                             (ptrH264Info->SVCheader[idxNALU].qualityID << 8) +
                              ptrH264Info->SVCheader[idxNALU].temporalID;
        isBaseLayer=(layer==0);
    }

    
    memcpy(&dataBuffer[rtpHeaderLength], &data[ptrH264Info->startCodeSize[idxNALU]], ptrH264Info->payloadSize[idxNALU]);

    WebRtc_UWord16 payloadBytesInPacket = (WebRtc_UWord16)ptrH264Info->payloadSize[idxNALU];
    payloadBytesToSend -= ptrH264Info->payloadSize[idxNALU] + ptrH264Info->startCodeSize[idxNALU]; 

    
    _rtpSender.BuildRTPheader(dataBuffer,payloadType,(payloadBytesToSend ==0)?true:false, captureTimeStamp);

    dataBuffer[rtpHeaderLength] &= 0x1f; 
    dataBuffer[rtpHeaderLength] |= ptrH264Info->NRI[idxNALU]; 
    if(payloadBytesToSend > 0)
    {
        data += ptrH264Info->payloadSize[idxNALU] + ptrH264Info->startCodeSize[idxNALU];
    }
    idxNALU++;
    if(-1 == SendVideoPacket(frameType, dataBuffer, payloadBytesInPacket, rtpHeaderLength,isBaseLayer))
    {
        return -1;
    }
    return 0;
}

WebRtc_Word32
RTPSenderH264::SendH264_SinglePACSI(const FrameType frameType,
                                    const H264Info* ptrH264Info,
                                     const WebRtc_UWord16 idxNALU,
                                     const WebRtc_Word8 payloadType,
                                     const WebRtc_UWord32 captureTimeStamp,
                                     const bool firstPacketInNALU,
                                     const bool lastPacketInNALU);
{
    
    WebRtc_UWord8 dataBuffer[WEBRTC_IP_PACKET_SIZE];
    WebRtc_UWord16 rtpHeaderLength = (WebRtc_UWord16)_rtpSender.BuildRTPheader(dataBuffer, payloadType,false, captureTimeStamp);
    WebRtc_Word32 dataOffset = rtpHeaderLength;

    WebRtc_Word32 lengthPASCINALU = AddH264PACSINALU(firstPacketInNALU,
                                                   lastPacketInNALU,
                                                   ptrH264Info->PACSI[idxNALU],
                                                   ptrH264Info->SVCheader[idxNALU],
                                                   decodingOrderNumber,
                                                   dataBuffer,
                                                   dataOffset);

    if (lengthPASCINALU <= 0)
    {
        return -1;
    }
    decodingOrderNumber++;

    WebRtc_UWord16 payloadBytesInPacket = (WebRtc_UWord16)lengthPASCINALU;

    
    dataBuffer[rtpHeaderLength] &= 0x1f;        
    dataBuffer[rtpHeaderLength] |= ptrH264Info->NRI[idxNALU]; 

    const WebRtc_UWord32 layer = (ptrH264Info->SVCheader[idxNALU].dependencyID << 16)+
                               (ptrH264Info->SVCheader[idxNALU].qualityID << 8) +
                                ptrH264Info->SVCheader[idxNALU].temporalID;

    if (-1 == SendVideoPacket(frameType, dataBuffer, payloadBytesInPacket, rtpHeaderLength,layer==0))
    {
        return -1;
    }
    return 0;
}




WebRtc_Word32
RTPSenderH264::SendH264SVC(const FrameType frameType,
                            const WebRtc_Word8 payloadType,
                            const WebRtc_UWord32 captureTimeStamp,
                            const WebRtc_UWord8* payloadData,
                            const WebRtc_UWord32 payloadSize,
                            H264Information& h264Information,
                            WebRtc_UWord16& decodingOrderNumber)
{
    WebRtc_Word32 payloadBytesToSend = payloadSize;
    const WebRtc_UWord16 rtpHeaderLength = _rtpSender.RTPHeaderLength();

    const H264Info* ptrH264Info = NULL;
    if (h264Information.GetInfo(payloadData,payloadSize, ptrH264Info) == -1)
    {
        return -1;
    }
    if(_useHighestSendLayer)
    {
        
        
        const WebRtc_UWord8 dependencyQualityID = (ptrH264Info->SVCheader[0].dependencyID << 4) + ptrH264Info->SVCheader[0].qualityID;

        bool dummyHighestLayer;
        if(SendH264SVCLayer(frameType,
                            ptrH264Info->SVCheader[0].temporalID,
                            dependencyQualityID,
                            dummyHighestLayer) == false)
        {
            
            return 0;
        }
    }

    WebRtc_UWord16 idxNALU = 0;
    while (payloadBytesToSend > 0)
    {
        bool switchToFUA = false;
        if (SendH264_STAP_A_PACSI(frameType,
                                  ptrH264Info,
                                  idxNALU,
                                  payloadType,
                                  captureTimeStamp,
                                  switchToFUA,
                                  payloadBytesToSend,
                                  payloadData,
                                  rtpHeaderLength,
                                  decodingOrderNumber) != 0)
        {
            return -1;
        }
        if(switchToFUA)
        {
            
            if (SendH264_FU_A(frameType,
                              ptrH264Info,
                              idxNALU,
                              payloadType,
                              captureTimeStamp,
                              payloadBytesToSend,
                              payloadData,
                              rtpHeaderLength,
                              true) != 0)
            {
                return -1;
            }
        }
    }
    return 0;
}

WebRtc_Word32
RTPSenderH264::SetH264PacketizationMode(const H264PacketizationMode mode)
{
    _h264Mode = mode;
    return 0;
}

WebRtc_Word32
RTPSenderH264::SetH264SendModeNALU_PPS_SPS(const bool dontSend)
{
    _h264SendPPS_SPS = !dontSend;
    return 0;
}

bool
RTPSenderH264::SendH264SVCLayer(const FrameType frameType,
                                  const WebRtc_UWord8 temporalID,
                                  const WebRtc_UWord8 dependencyQualityID,
                                  bool& higestLayer)
{
    WebRtc_UWord8 dependencyID  = dependencyQualityID >> 4;

    
    if( _highestDependencyLayer != _highestDependencyLayerOld)
    {
        
        if(frameType == kVideoFrameKey)
        {
            
            if(_highestDependencyLayer > _highestDependencyLayerOld)
            {
                
                

                if( dependencyID > _highestDependencyLayerOld &&
                    dependencyID <= _highestDependencyLayer)
                {
                    _highestDependencyLayerOld = dependencyID;
                    _highestDependencyQualityIDOld = _highestDependencyQualityID;

                    if( dependencyID == _highestDependencyLayer &&
                        dependencyQualityID == _highestDependencyQualityID)
                    {
                        higestLayer = true;
                    }
                    
                    return true;
                }
            }
            if(_highestDependencyLayer < _highestDependencyLayerOld)
            {
                
                
                if( dependencyID <= _highestDependencyLayer)
                {
                    _highestDependencyLayerOld = dependencyID;
                    _highestDependencyQualityIDOld = _highestDependencyQualityID;
                    if( dependencyID == _highestDependencyLayer &&
                        dependencyQualityID == _highestDependencyQualityID)
                    {
                        higestLayer = true;
                    }
                    
                    return true;
                }
            }
        } else
        {
            
            if(_highestDependencyLayer > _highestDependencyLayerOld)
            {
                
                

                
                
                
                
                if( _highestTemporalLayer < temporalID ||
                    _highestDependencyLayerOld < dependencyID ||
                    _highestDependencyQualityIDOld < dependencyQualityID)
                {
                    
                    return false;
                }
                
                if( dependencyID == _highestDependencyLayerOld &&
                    dependencyQualityID == _highestDependencyQualityIDOld)
                {
                    higestLayer = true;
                }
            } else
            {
                
                
                
                if(temporalID > 0)
                {
                    
                    return false;
                }
                
                
                
                if( _highestDependencyLayerOld < dependencyID ||
                    (_highestDependencyQualityIDOld & 0xf0) < dependencyQualityID)
                {
                    
                    return false;
                }
                if( dependencyID == _highestDependencyLayerOld &&
                    dependencyQualityID == (_highestDependencyQualityIDOld & 0xf0))
                {
                    higestLayer = true;
                }
            }
        }
    } else
    {
        
        if( _highestTemporalLayer < temporalID ||
            _highestDependencyLayer < dependencyID ||
            _highestDependencyQualityID < dependencyQualityID)
        {
            
            return false;
        }
        if( dependencyID == _highestDependencyLayer &&
            dependencyQualityID == _highestDependencyQualityID)
        {
            higestLayer = true;
        }
    }
    return true;
}

WebRtc_Word32
RTPSenderH264::SetHighestSendLayer(const WebRtc_UWord8 dependencyQualityLayer,
                                   const WebRtc_UWord8 temporalLayer)
{
    const WebRtc_UWord8 dependencyLayer = (dependencyQualityLayer >> 4);

    if(_highestDependencyLayerOld != _highestDependencyLayer)
    {
        
    } else
    {
        if(_highestDependencyLayer == dependencyLayer)
        {
            
            
            _highestDependencyQualityIDOld = dependencyQualityLayer;
        }else
        {
            
            _highestDependencyQualityIDOld = _highestDependencyQualityID;
        }
    }
    _useHighestSendLayer = true;
    _highestDependencyLayer = dependencyLayer;
    _highestDependencyQualityID = dependencyQualityLayer;
    _highestTemporalLayer = temporalLayer;
    return 0;
}

WebRtc_Word32
RTPSenderH264::HighestSendLayer(WebRtc_UWord8& dependencyQualityLayer,
                                WebRtc_UWord8& temporalLayer)
{
    if (!_useHighestSendLayer)
    {
        
        return -1;
    }
    dependencyQualityLayer = _highestDependencyQualityID;
    temporalLayer = _highestTemporalLayer;
    return 0;
}



WebRtc_Word32
RTPSenderH264::SendH264(const FrameType frameType,
                        const WebRtc_Word8 payloadType,
                        const WebRtc_UWord32 captureTimeStamp,
                        const WebRtc_UWord8* payloadData,
                        const WebRtc_UWord32 payloadSize,
                        H264Information& h264Information)
{
    WebRtc_Word32 payloadBytesToSend = payloadSize;
    const WebRtc_UWord8* data = payloadData;
    bool switchToFUA = false;
    const WebRtc_UWord16 rtpHeaderLength = _rtpSender.RTPHeaderLength();

    const H264Info* ptrH264Info = NULL;
    if (h264Information.GetInfo(payloadData,payloadSize, ptrH264Info) == -1)
    {
        return -1;
    }
    WebRtc_UWord16 idxNALU = 0;
    WebRtc_UWord16 DONCdummy = 0;

    while (payloadBytesToSend > 0)
    {
        switch(_h264Mode)
        {
        case H264_NON_INTERLEAVED_MODE:

            if(!switchToFUA)
            {
                if(SendH264_STAP_A(frameType,
                                   ptrH264Info,
                                   idxNALU,
                                   payloadType,
                                   captureTimeStamp,
                                   switchToFUA,
                                   payloadBytesToSend,
                                   data,
                                   rtpHeaderLength) != 0)
                {
                    return -1;
                }
            }
            else
            {
                
                if(SendH264_FU_A(frameType,
                                 ptrH264Info,
                                 idxNALU,
                                 payloadType,
                                 captureTimeStamp,
                                 payloadBytesToSend,
                                 data,
                                 rtpHeaderLength,
                                 DONCdummy) != 0)
                {
                    return -1;
                }
                
                switchToFUA = false;
            }
            break;
        case H264_SINGLE_NAL_MODE:
            {
                
                if(SendH264_SingleMode(frameType,
                                       ptrH264Info,
                                       idxNALU,
                                       payloadType,
                                       captureTimeStamp,
                                       payloadBytesToSend,
                                       data,
                                       rtpHeaderLength,
                                       DONCdummy) != 0)
                {
                    return -1;
                }
                break;
            }
        case H264_INTERLEAVED_MODE:
            
            assert(false);
            return -1;
        }
    }
    return 0;
}
} 
