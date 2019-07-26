













#include "dtmf_buffer.h"

#include "typedefs.h" 
#include "signal_processing_library.h"

#include "neteq_error_codes.h"


#ifdef NETEQ_ATEVENT_DECODE

int16_t WebRtcNetEQ_DtmfRemoveEvent(dtmf_inst_t *DTMFdec_inst)
{

    int i;
    for (i = 0; i < 3; i++)
    {
        DTMFdec_inst->EventQueue[i] = DTMFdec_inst->EventQueue[i + 1];
        DTMFdec_inst->EventQueueVolume[i] = DTMFdec_inst->EventQueueVolume[i + 1];
        DTMFdec_inst->EventQueueEnded[i] = DTMFdec_inst->EventQueueEnded[i + 1];
        DTMFdec_inst->EventQueueStartTime[i] = DTMFdec_inst->EventQueueStartTime[i + 1];
        DTMFdec_inst->EventQueueEndTime[i] = DTMFdec_inst->EventQueueEndTime[i + 1];
    }
    DTMFdec_inst->EventBufferSize--;
    DTMFdec_inst->EventQueue[3] = -1;
    DTMFdec_inst->EventQueueVolume[3] = 0;
    DTMFdec_inst->EventQueueEnded[3] = 0;
    DTMFdec_inst->EventQueueStartTime[3] = 0;
    DTMFdec_inst->EventQueueEndTime[3] = 0;

    return 0;
}

int16_t WebRtcNetEQ_DtmfDecoderInit(dtmf_inst_t *DTMFdec_inst, uint16_t fs,
                                    int16_t MaxPLCtime)
{
    int i;
    if (((fs != 8000) && (fs != 16000) && (fs != 32000) && (fs != 48000)) || (MaxPLCtime < 0))
    {
        return DTMF_DEC_PARAMETER_ERROR;
    }
    if (fs == 8000)
        DTMFdec_inst->framelen = 80;
    else if (fs == 16000)
        DTMFdec_inst->framelen = 160;
    else if (fs == 32000)
        DTMFdec_inst->framelen = 320;
    else
        
        DTMFdec_inst->framelen = 480;

    DTMFdec_inst->MaxPLCtime = MaxPLCtime;
    DTMFdec_inst->CurrentPLCtime = 0;
    DTMFdec_inst->EventBufferSize = 0;
    for (i = 0; i < 4; i++)
    {
        DTMFdec_inst->EventQueue[i] = -1;
        DTMFdec_inst->EventQueueVolume[i] = 0;
        DTMFdec_inst->EventQueueEnded[i] = 0;
        DTMFdec_inst->EventQueueStartTime[i] = 0;
        DTMFdec_inst->EventQueueEndTime[i] = 0;
    }
    return 0;
}

int16_t WebRtcNetEQ_DtmfInsertEvent(dtmf_inst_t *DTMFdec_inst,
                                    const int16_t *encoded, int16_t len,
                                    uint32_t timeStamp)
{

    int i;
    int16_t value;
    const int16_t *EventStart;
    int16_t endEvent;
    int16_t Volume;
    int16_t Duration;
    int16_t position = -1;

    
    if (len == 4)
    {
        EventStart = encoded;
#ifdef WEBRTC_ARCH_BIG_ENDIAN
        value=((*EventStart)>>8);
        endEvent=((*EventStart)&0x80)>>7;
        Volume=((*EventStart)&0x3F);
        Duration=EventStart[1];
#else
        value = ((*EventStart) & 0xFF);
        endEvent = ((*EventStart) & 0x8000) >> 15;
        Volume = ((*EventStart) & 0x3F00) >> 8;
        Duration = (((((uint16_t) EventStart[1]) >> 8) & 0xFF)
            | (((uint16_t) (EventStart[1] & 0xFF)) << 8));
#endif
        
        if ((value < 0) || (value > 15))
        {
            return 0;
        }

        
        if (Volume > 36)
        {
            return 0;
        }

        
        for (i = 0; i < DTMFdec_inst->EventBufferSize; i++)
        {
            

            if ((DTMFdec_inst->EventQueue[i] == value) && (!DTMFdec_inst->EventQueueEnded[i]
                || endEvent)) position = i;
        }
        if (position > -1)
        {
            DTMFdec_inst->EventQueueVolume[position] = Volume;
            if ((timeStamp + Duration) > DTMFdec_inst->EventQueueEndTime[position]) DTMFdec_inst->EventQueueEndTime[position]
                = DTMFdec_inst->EventQueueStartTime[position] + Duration;
            if (endEvent) DTMFdec_inst->EventQueueEnded[position] = 1;
        }
        else
        {
            if (DTMFdec_inst->EventBufferSize == MAX_DTMF_QUEUE_SIZE)
            { 
                
                DTMFdec_inst->EventBufferSize--;
            }
            
            DTMFdec_inst->EventQueue[DTMFdec_inst->EventBufferSize] = value;
            DTMFdec_inst->EventQueueVolume[DTMFdec_inst->EventBufferSize] = Volume;
            DTMFdec_inst->EventQueueEnded[DTMFdec_inst->EventBufferSize] = endEvent;
            DTMFdec_inst->EventQueueStartTime[DTMFdec_inst->EventBufferSize] = timeStamp;
            DTMFdec_inst->EventQueueEndTime[DTMFdec_inst->EventBufferSize] = timeStamp
                + Duration;
            DTMFdec_inst->EventBufferSize++;
        }
        return 0;
    }
    return DTMF_INSERT_ERROR;
}

int16_t WebRtcNetEQ_DtmfDecode(dtmf_inst_t *DTMFdec_inst, int16_t *event,
                               int16_t *volume, uint32_t currTimeStamp)
{

    if (DTMFdec_inst->EventBufferSize < 1) return 0; 

    
    if (currTimeStamp < DTMFdec_inst->EventQueueStartTime[0])
    {
        
        return 0;
    }

    
    *event = DTMFdec_inst->EventQueue[0];
    *volume = DTMFdec_inst->EventQueueVolume[0];

    if (DTMFdec_inst->EventQueueEndTime[0] >= (currTimeStamp + DTMFdec_inst->framelen))
    {

        

        DTMFdec_inst->CurrentPLCtime = 0;
        if ((DTMFdec_inst->EventQueueEndTime[0] == (currTimeStamp + DTMFdec_inst->framelen))
            && (DTMFdec_inst->EventQueueEnded[0]))
        { 
            
            WebRtcNetEQ_DtmfRemoveEvent(DTMFdec_inst);
        }
        return DTMFdec_inst->framelen;

    }
    else
    {
        if ((DTMFdec_inst->EventQueueEnded[0]) || (DTMFdec_inst->EventQueue[1] > -1))
        {
            




            
            WebRtcNetEQ_DtmfRemoveEvent(DTMFdec_inst);
            DTMFdec_inst->CurrentPLCtime = 0;

            return DTMFdec_inst->framelen;

        }
        else
        {
            
            DTMFdec_inst->CurrentPLCtime = (int16_t) (currTimeStamp
                - DTMFdec_inst->EventQueueEndTime[0]);

            if ((DTMFdec_inst->CurrentPLCtime > DTMFdec_inst->MaxPLCtime)
                || (DTMFdec_inst->CurrentPLCtime < -DTMFdec_inst->MaxPLCtime))
            {
                
                WebRtcNetEQ_DtmfRemoveEvent(DTMFdec_inst);
                DTMFdec_inst->CurrentPLCtime = 0;
            }

            
            if ((DTMFdec_inst->EventQueue[1] > -1) && (DTMFdec_inst->EventQueueStartTime[1]
                >= (currTimeStamp + DTMFdec_inst->framelen)))
            {
                
                WebRtcNetEQ_DtmfRemoveEvent(DTMFdec_inst);
                DTMFdec_inst->CurrentPLCtime = 0;
            }

            return DTMFdec_inst->framelen;
        }
    }
}

#endif
