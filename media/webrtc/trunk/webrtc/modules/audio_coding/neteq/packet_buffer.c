













#include <assert.h>
#include "packet_buffer.h"

#include <string.h> 

#include "signal_processing_library.h"

#include "mcu_dsp_common.h"

#include "neteq_error_codes.h"

#ifdef NETEQ_DELAY_LOGGING

#include "delay_logging.h"
#include <stdio.h>

extern FILE *delay_fid2; 
extern uint32_t tot_received_packets;
#endif 


int WebRtcNetEQ_PacketBufferInit(PacketBuf_t *bufferInst, int maxNoOfPackets,
                                 int16_t *pw16_memory, int memorySize)
{
    int i;
    int pos = 0;

    
    if ((memorySize < PBUFFER_MIN_MEMORY_SIZE) || (pw16_memory == NULL)
        || (maxNoOfPackets < 2) || (maxNoOfPackets > 600))
    {
        
        return (PBUFFER_INIT_ERROR);
    }

    
    WebRtcSpl_MemSetW16((int16_t*) bufferInst, 0,
        sizeof(PacketBuf_t) / sizeof(int16_t));

    
    WebRtcSpl_MemSetW16((int16_t*) pw16_memory, 0, memorySize);

    
    bufferInst->maxInsertPositions = maxNoOfPackets;

    
    




    bufferInst->timeStamp = (uint32_t*) &pw16_memory[pos];
    pos += maxNoOfPackets << 1; 

    bufferInst->payloadLocation = (int16_t**) &pw16_memory[pos];
    pos += maxNoOfPackets * (sizeof(int16_t*) / sizeof(int16_t)); 

    bufferInst->seqNumber = (uint16_t*) &pw16_memory[pos];
    pos += maxNoOfPackets; 

    bufferInst->payloadType = &pw16_memory[pos];
    pos += maxNoOfPackets; 

    bufferInst->payloadLengthBytes = &pw16_memory[pos];
    pos += maxNoOfPackets; 

    bufferInst->rcuPlCntr = &pw16_memory[pos];
    pos += maxNoOfPackets; 

    bufferInst->waitingTime = (int*) (&pw16_memory[pos]);
    
    pos += maxNoOfPackets *
        sizeof(*bufferInst->waitingTime) / sizeof(*pw16_memory);

    
    bufferInst->startPayloadMemory = &pw16_memory[pos];
    bufferInst->currentMemoryPos = bufferInst->startPayloadMemory;
    bufferInst->memorySizeW16 = (memorySize - pos); 

    
    for (i = 0; i < bufferInst->maxInsertPositions; i++)
    {
        bufferInst->payloadType[i] = -1;
    }

    
    bufferInst->numPacketsInBuffer = 0;
    bufferInst->packSizeSamples = 0;
    bufferInst->insertPosition = 0;

    
    bufferInst->discardedPackets = 0;

    return (0);
}


int WebRtcNetEQ_PacketBufferFlush(PacketBuf_t *bufferInst)
{
    int i;

    
    if (bufferInst->startPayloadMemory == NULL)
    {
        
        

        return (0);
    }

    
    WebRtcSpl_MemSetW16(bufferInst->payloadLengthBytes, 0, bufferInst->maxInsertPositions);

    
    bufferInst->numPacketsInBuffer = 0;
    bufferInst->currentMemoryPos = bufferInst->startPayloadMemory;
    bufferInst->insertPosition = 0;

    
    for (i = (bufferInst->maxInsertPositions - 1); i >= 0; i--)
    {
        bufferInst->payloadType[i] = -1;
        bufferInst->timeStamp[i] = 0;
        bufferInst->seqNumber[i] = 0;
    }

    return (0);
}


int WebRtcNetEQ_PacketBufferInsert(PacketBuf_t *bufferInst, const RTPPacket_t *RTPpacket,
                                   int16_t *flushed, int av_sync)
{
    int nextPos;
    int i;

#ifdef NETEQ_DELAY_LOGGING
    
    int temp_var;
#endif 

    
    *flushed = 0;

    
    if (bufferInst->startPayloadMemory == NULL)
    {
        
        return (-1);
    }

    

    if ((RTPpacket->payloadLen > (bufferInst->memorySizeW16 << 1)) || (RTPpacket->payloadLen
        <= 0))
    {
        
        return (-1);
    }

    










    if (av_sync) {
      for (i = 0; i < bufferInst->maxInsertPositions; ++i) {
        
        if (bufferInst->seqNumber[i] == RTPpacket->seqNumber &&
            bufferInst->payloadLengthBytes[i] > 0) {
          if (WebRtcNetEQ_IsSyncPayload(RTPpacket->payload,
                                        RTPpacket->payloadLen)) {
            return 0;
          }

          if (WebRtcNetEQ_IsSyncPayload(bufferInst->payloadLocation[i],
                                        bufferInst->payloadLengthBytes[i])) {
            
            bufferInst->payloadType[i] = -1;
            bufferInst->payloadLengthBytes[i] = 0;

            
            bufferInst->numPacketsInBuffer--;
            

            break;  
          }
        }
      }
    }

    
    if (bufferInst->numPacketsInBuffer != 0)
    {
        
        bufferInst->insertPosition++;
        if (bufferInst->insertPosition >= bufferInst->maxInsertPositions)
        {
            
            bufferInst->insertPosition = 0;
        }

        
        if (bufferInst->currentMemoryPos + ((RTPpacket->payloadLen + 1) >> 1)
            >= &bufferInst->startPayloadMemory[bufferInst->memorySizeW16])
        {
            int16_t *tempMemAddress;

            



            bufferInst->currentMemoryPos = bufferInst->startPayloadMemory;

            



            tempMemAddress = &bufferInst->startPayloadMemory[bufferInst->memorySizeW16];
            nextPos = -1;

            
            for (i = 0; i < bufferInst->maxInsertPositions; i++)
            {
                

                if (bufferInst->payloadLengthBytes[i] != 0 && bufferInst->payloadLocation[i]
                    < tempMemAddress)
                {
                    tempMemAddress = bufferInst->payloadLocation[i];
                    nextPos = i;
                }
            }

            
            if (nextPos == -1)
            {
                
                WebRtcNetEQ_PacketBufferFlush(bufferInst);
                *flushed = 1;
                return (-1);
            }
        }
        else
        {
            

            
            nextPos = bufferInst->insertPosition + 1;

            
            while ((bufferInst->payloadLengthBytes[nextPos] == 0) && (nextPos
                < bufferInst->maxInsertPositions))
            {
                nextPos++;
            }

            if (nextPos == bufferInst->maxInsertPositions)
            {
                



                nextPos = 0;

                
                while (bufferInst->payloadLengthBytes[nextPos] == 0)
                {
                    nextPos++;
                }
            }
        } 

        



        if ((bufferInst->currentMemoryPos <= bufferInst->payloadLocation[nextPos])
            && ((&bufferInst->currentMemoryPos[(RTPpacket->payloadLen + 1) >> 1])
                > bufferInst->payloadLocation[nextPos]))
        {
            
            WebRtcNetEQ_PacketBufferFlush(bufferInst);
            *flushed = 1;
        }

        if (bufferInst->payloadLengthBytes[bufferInst->insertPosition] != 0)
        {
            
            WebRtcNetEQ_PacketBufferFlush(bufferInst);
            *flushed = 1;
        }

    }
    else
    {
        
        bufferInst->currentMemoryPos = bufferInst->startPayloadMemory;
        bufferInst->insertPosition = 0;
    }

    
    if (RTPpacket->starts_byte1 == 0)
    {
        

        WEBRTC_SPL_MEMCPY_W8(bufferInst->currentMemoryPos,
            RTPpacket->payload, RTPpacket->payloadLen);
    }
    else
    {
        
        for (i = 0; i < RTPpacket->payloadLen; i++)
        {
            

            WEBRTC_SPL_SET_BYTE(bufferInst->currentMemoryPos,
                (WEBRTC_SPL_GET_BYTE(RTPpacket->payload, (i + 1))), i);
        }
    }

    
    bufferInst->payloadLocation[bufferInst->insertPosition] = bufferInst->currentMemoryPos;
    bufferInst->payloadLengthBytes[bufferInst->insertPosition] = RTPpacket->payloadLen;
    bufferInst->payloadType[bufferInst->insertPosition] = RTPpacket->payloadType;
    bufferInst->seqNumber[bufferInst->insertPosition] = RTPpacket->seqNumber;
    bufferInst->timeStamp[bufferInst->insertPosition] = RTPpacket->timeStamp;
    bufferInst->rcuPlCntr[bufferInst->insertPosition] = RTPpacket->rcuPlCntr;
    bufferInst->waitingTime[bufferInst->insertPosition] = 0;
    
    bufferInst->numPacketsInBuffer++;
    bufferInst->currentMemoryPos += (RTPpacket->payloadLen + 1) >> 1;

#ifdef NETEQ_DELAY_LOGGING
    
    if (*flushed)
    {
        temp_var = NETEQ_DELAY_LOGGING_SIGNAL_FLUSH;
        if (fwrite(&temp_var, sizeof(int), 1, delay_fid2) != 1) {
          return -1;
        }
    }
    temp_var = NETEQ_DELAY_LOGGING_SIGNAL_RECIN;
    if ((fwrite(&temp_var, sizeof(int),
                1, delay_fid2) != 1) ||
        (fwrite(&RTPpacket->timeStamp, sizeof(uint32_t),
                1, delay_fid2) != 1) ||
        (fwrite(&RTPpacket->seqNumber, sizeof(uint16_t),
                1, delay_fid2) != 1) ||
        (fwrite(&RTPpacket->payloadType, sizeof(int),
                1, delay_fid2) != 1) ||
        (fwrite(&RTPpacket->payloadLen, sizeof(int16_t),
                1, delay_fid2) != 1)) {
      return -1;
    }
    tot_received_packets++;
#endif 

    return (0);
}


int WebRtcNetEQ_PacketBufferExtract(PacketBuf_t *bufferInst, RTPPacket_t *RTPpacket,
                                    int bufferPosition, int *waitingTime)
{

    
    if (bufferInst->startPayloadMemory == NULL)
    {
        
        return (PBUFFER_NOT_INITIALIZED);
    }

    if (bufferPosition < 0 || bufferPosition >= bufferInst->maxInsertPositions)
    {
        
        return (NETEQ_OTHER_ERROR);
    }

    
    if (bufferInst->payloadLengthBytes[bufferPosition] <= 0)
    {
        
        RTPpacket->payloadLen = 0; 
        return (PBUFFER_NONEXISTING_PACKET); 
    }

    

    

    WEBRTC_SPL_MEMCPY_W16((int16_t*) RTPpacket->payload,
        bufferInst->payloadLocation[bufferPosition],
        (bufferInst->payloadLengthBytes[bufferPosition] + 1) >> 1); 

    
    RTPpacket->payloadLen = bufferInst->payloadLengthBytes[bufferPosition];
    RTPpacket->payloadType = bufferInst->payloadType[bufferPosition];
    RTPpacket->seqNumber = bufferInst->seqNumber[bufferPosition];
    RTPpacket->timeStamp = bufferInst->timeStamp[bufferPosition];
    RTPpacket->rcuPlCntr = bufferInst->rcuPlCntr[bufferPosition];
    *waitingTime = bufferInst->waitingTime[bufferPosition];
    RTPpacket->starts_byte1 = 0; 

    
    bufferInst->payloadType[bufferPosition] = -1;
    bufferInst->payloadLengthBytes[bufferPosition] = 0;
    bufferInst->seqNumber[bufferPosition] = 0;
    bufferInst->timeStamp[bufferPosition] = 0;
    bufferInst->waitingTime[bufferPosition] = 0;
    bufferInst->payloadLocation[bufferPosition] = bufferInst->startPayloadMemory;

    
    bufferInst->numPacketsInBuffer--;

    return (0);
}

int WebRtcNetEQ_PacketBufferFindLowestTimestamp(PacketBuf_t* buffer_inst,
                                                uint32_t current_time_stamp,
                                                uint32_t* time_stamp,
                                                int* buffer_position,
                                                int erase_old_packets,
                                                int16_t* payload_type) {
  int32_t time_stamp_diff = WEBRTC_SPL_WORD32_MAX;  
  int32_t new_diff;
  int i;
  int16_t rcu_payload_cntr;
  if (buffer_inst->startPayloadMemory == NULL) {
    
    return PBUFFER_NOT_INITIALIZED;
  }

  
  *time_stamp = 0;
  *payload_type = -1;  
  *buffer_position = -1;  
  rcu_payload_cntr = WEBRTC_SPL_WORD16_MAX;  

  
  if (buffer_inst->numPacketsInBuffer <= 0) {
    return 0;
  }

  
  if (erase_old_packets) {  
    for (i = 0; i < buffer_inst->maxInsertPositions; ++i) {
      
      new_diff = (int32_t)(buffer_inst->timeStamp[i] - current_time_stamp);

      
      if ((new_diff < 0)  
          && (new_diff > -30000)  
          && (buffer_inst->payloadLengthBytes[i] > 0)) {  
        

        
        buffer_inst->payloadType[i] = -1;
        buffer_inst->payloadLengthBytes[i] = 0;

        
        buffer_inst->numPacketsInBuffer--;
        
        buffer_inst->discardedPackets++;
      } else if (((new_diff < time_stamp_diff) 
                  || ((new_diff == time_stamp_diff)
                      && (buffer_inst->rcuPlCntr[i] < rcu_payload_cntr)))
                      && (buffer_inst->payloadLengthBytes[i] > 0)) {
        


 
        
        *buffer_position = i;
        time_stamp_diff = new_diff;
        *payload_type = buffer_inst->payloadType[i];
        rcu_payload_cntr = buffer_inst->rcuPlCntr[i];
      }
    }
  } else {
    for (i = 0; i < buffer_inst->maxInsertPositions; ++i) {
      
      new_diff = (int32_t)(buffer_inst->timeStamp[i] - current_time_stamp);

      
      if (((new_diff < time_stamp_diff) 
           || ((new_diff == time_stamp_diff)
               && (buffer_inst->rcuPlCntr[i] < rcu_payload_cntr)))
               && (buffer_inst->payloadLengthBytes[i] > 0)) {
        


 
        
        *buffer_position = i;
        time_stamp_diff = new_diff;
        *payload_type = buffer_inst->payloadType[i];
        rcu_payload_cntr = buffer_inst->rcuPlCntr[i];
      }
    }
  }

  
  if (*buffer_position >= 0) {
    
    *time_stamp = buffer_inst->timeStamp[*buffer_position];
  }

  return 0;
}

int WebRtcNetEQ_PacketBufferGetPacketSize(const PacketBuf_t* buffer_inst,
                                          int buffer_pos,
                                          const CodecDbInst_t* codec_database,
                                          int codec_pos, int last_duration,
                                          int av_sync) {
  if (codec_database->funcDurationEst[codec_pos] == NULL) {
    return last_duration;
  }

  if (av_sync != 0 &&
      WebRtcNetEQ_IsSyncPayload(buffer_inst->payloadLocation[buffer_pos],
                                buffer_inst->payloadLengthBytes[buffer_pos])) {
    
    return last_duration;
  }

  return (*codec_database->funcDurationEst[codec_pos])(
    codec_database->codec_state[codec_pos],
    (const uint8_t *)buffer_inst->payloadLocation[buffer_pos],
    buffer_inst->payloadLengthBytes[buffer_pos]);
}

int32_t WebRtcNetEQ_PacketBufferGetSize(const PacketBuf_t* buffer_inst,
                                        const CodecDbInst_t* codec_database,
                                        int av_sync) {
  int i, count;
  int last_duration;
  int last_codec_pos;
  int last_payload_type;
  int32_t size_samples;

  count = 0;
  last_duration = buffer_inst->packSizeSamples;
  last_codec_pos = -1;
  last_payload_type = -1;
  size_samples = 0;

  
  for (i = 0; i < buffer_inst->maxInsertPositions; i++) {
    
    if (buffer_inst->payloadLengthBytes[i] != 0) {
      int payload_type;
      int codec_pos;
      
      payload_type = buffer_inst->payloadType[i];
      
      if(payload_type == last_payload_type) {
        codec_pos = last_codec_pos;
      }
      else {
        codec_pos = WebRtcNetEQ_DbGetCodec(codec_database,
          payload_type);
        if (codec_pos >= 0) {
          codec_pos = codec_database->position[codec_pos];
        }
      }
      last_codec_pos = codec_pos;
      last_payload_type = payload_type;
      if (codec_pos >= 0) {
        






        
        int temp_last_duration = WebRtcNetEQ_PacketBufferGetPacketSize(
            buffer_inst, i, codec_database, codec_pos,
            last_duration, av_sync);
        if (temp_last_duration >= 0)
          last_duration = temp_last_duration;
      }
      
      size_samples += last_duration;
      count++;
    }
  }

  
  if (size_samples < 0) {
    size_samples = 0;
  }
  return size_samples;
}

void WebRtcNetEQ_IncrementWaitingTimes(PacketBuf_t *buffer_inst) {
  int i;
  
  for (i = 0; i < buffer_inst->maxInsertPositions; ++i) {
    
    if (buffer_inst->payloadLengthBytes[i] != 0) {
      buffer_inst->waitingTime[i]++;
    }
  }
}

int WebRtcNetEQ_GetDefaultCodecSettings(const enum WebRtcNetEQDecoder *codecID,
                                        int noOfCodecs, int *maxBytes,
                                        int *maxSlots,
                                        int* per_slot_overhead_bytes)
{
    int i;
    int ok = 0;
    int16_t w16_tmp;
    int16_t codecBytes;
    int16_t codecBuffers;

    
    *maxBytes = 0;
    *maxSlots = 0;

    
    for (i = 0; i < noOfCodecs; i++)
    {
        

        if ((codecID[i] == kDecoderPCMu) || (codecID[i] == kDecoderPCMu_2ch))
        {
            codecBytes = 1680; 
            codecBuffers = 30; 
        }
        else if ((codecID[i] == kDecoderPCMa) ||
            (codecID[i] == kDecoderPCMa_2ch))
        {
            codecBytes = 1680; 
            codecBuffers = 30; 
        }
        else if (codecID[i] == kDecoderILBC)
        {
            codecBytes = 380; 
            codecBuffers = 10;
        }
        else if (codecID[i] == kDecoderISAC)
        {
            codecBytes = 960; 
            codecBuffers = 8;
        }
        else if ((codecID[i] == kDecoderISACswb) ||
            (codecID[i] == kDecoderISACfb))
        {
            codecBytes = 1560; 
            codecBuffers = 8;
        }
        else if (codecID[i] == kDecoderOpus)
        {
            codecBytes = 15300; 
            codecBuffers = 30;  
        }
        else if ((codecID[i] == kDecoderPCM16B) ||
            (codecID[i] == kDecoderPCM16B_2ch))
        {
            codecBytes = 3360; 
            codecBuffers = 15;
        }
        else if ((codecID[i] == kDecoderPCM16Bwb) ||
            (codecID[i] == kDecoderPCM16Bwb_2ch))
        {
            codecBytes = 6720; 
            codecBuffers = 15;
        }
        else if ((codecID[i] == kDecoderPCM16Bswb32kHz) ||
            (codecID[i] == kDecoderPCM16Bswb32kHz_2ch))
        {
            codecBytes = 13440; 
            codecBuffers = 15;
        }
        else if (codecID[i] == kDecoderPCM16Bswb48kHz)
        {
            codecBytes = 20160; 
            codecBuffers = 15;
        }
        else if ((codecID[i] == kDecoderG722) ||
            (codecID[i] == kDecoderG722_2ch))
        {
            codecBytes = 1680; 
            codecBuffers = 15;
        }
        else if (codecID[i] == kDecoderRED)
        {
            codecBytes = 0; 
            codecBuffers = 0;
        }
        else if (codecID[i] == kDecoderAVT)
        {
            codecBytes = 0; 
            codecBuffers = 0;
        }
        else if (codecID[i] == kDecoderCNG)
        {
            codecBytes = 0; 
            codecBuffers = 0;
        }
        else if (codecID[i] == kDecoderG729)
        {
            codecBytes = 210; 
            codecBuffers = 20; 
        }
        else if (codecID[i] == kDecoderG729_1)
        {
            codecBytes = 840; 
            codecBuffers = 10; 
        }
        else if (codecID[i] == kDecoderG726_16)
        {
            codecBytes = 400; 
            codecBuffers = 10;
        }
        else if (codecID[i] == kDecoderG726_24)
        {
            codecBytes = 600; 
            codecBuffers = 10;
        }
        else if (codecID[i] == kDecoderG726_32)
        {
            codecBytes = 800; 
            codecBuffers = 10;
        }
        else if (codecID[i] == kDecoderG726_40)
        {
            codecBytes = 1000; 
            codecBuffers = 10;
        }
        else if (codecID[i] == kDecoderG722_1_16)
        {
            codecBytes = 420; 
            codecBuffers = 10;
        }
        else if (codecID[i] == kDecoderG722_1_24)
        {
            codecBytes = 630; 
            codecBuffers = 10;
        }
        else if (codecID[i] == kDecoderG722_1_32)
        {
            codecBytes = 840; 
            codecBuffers = 10;
        }
        else if (codecID[i] == kDecoderG722_1C_24)
        {
            codecBytes = 630; 
            codecBuffers = 10;
        }
        else if (codecID[i] == kDecoderG722_1C_32)
        {
            codecBytes = 840; 
            codecBuffers = 10;
        }
        else if (codecID[i] == kDecoderG722_1C_48)
        {
            codecBytes = 1260; 
            codecBuffers = 10;
        }
        else if (codecID[i] == kDecoderSPEEX_8)
        {
            codecBytes = 1250; 
            codecBuffers = 10;
        }
        else if (codecID[i] == kDecoderSPEEX_16)
        {
            codecBytes = 1250; 
            codecBuffers = 10;
        }
        else if ((codecID[i] == kDecoderCELT_32) ||
            (codecID[i] == kDecoderCELT_32_2ch))
        {
            codecBytes = 1250; 
            codecBuffers = 10;
        }
        else if (codecID[i] == kDecoderGSMFR)
        {
            codecBytes = 340; 
            codecBuffers = 10;
        }
        else if (codecID[i] == kDecoderAMR)
        {
            codecBytes = 384; 
            codecBuffers = 10;
        }
        else if (codecID[i] == kDecoderAMRWB)
        {
            codecBytes = 744;
            codecBuffers = 10;
        }
        else if (codecID[i] == kDecoderArbitrary)
        {
            codecBytes = 6720; 
            codecBuffers = 15;
        }
        else
        {
            
            codecBytes = 0;
            codecBuffers = 0;
            ok = CODEC_DB_UNKNOWN_CODEC;
        }

        
        *maxBytes = WEBRTC_SPL_MAX((*maxBytes), codecBytes);
        *maxSlots = WEBRTC_SPL_MAX((*maxSlots), codecBuffers);

    } 

    



    w16_tmp = (sizeof(uint32_t) 
    + sizeof(int16_t*) 
    + sizeof(uint16_t) 
    + sizeof(int16_t)  
    + sizeof(int16_t)  
    + sizeof(int16_t)  
    + sizeof(int));          
    
    *maxBytes += w16_tmp * (*maxSlots);

    *per_slot_overhead_bytes = w16_tmp;
    return ok;
}
