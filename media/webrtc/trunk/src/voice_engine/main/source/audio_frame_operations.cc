









#include "audio_frame_operations.h"
#include "module_common_types.h"

namespace webrtc {

namespace voe {

WebRtc_Word32 
AudioFrameOperations::MonoToStereo(AudioFrame& audioFrame)
{
    if (audioFrame._audioChannel != 1)
    {
        return -1;
    }
    if ((audioFrame._payloadDataLengthInSamples << 1) >=
        AudioFrame::kMaxAudioFrameSizeSamples)
    {
        
        return -1;
    }

    WebRtc_Word16* payloadCopy =
        new WebRtc_Word16[audioFrame._payloadDataLengthInSamples];
    memcpy(payloadCopy, audioFrame._payloadData,
           sizeof(WebRtc_Word16)*audioFrame._payloadDataLengthInSamples);

    for (int i = 0; i < audioFrame._payloadDataLengthInSamples; i++)
    {
        audioFrame._payloadData[2*i]   = payloadCopy[i];
        audioFrame._payloadData[2*i+1] = payloadCopy[i];
    }

    audioFrame._audioChannel = 2;

    delete [] payloadCopy;
    return 0;
}

WebRtc_Word32 
AudioFrameOperations::StereoToMono(AudioFrame& audioFrame)
{
    if (audioFrame._audioChannel != 2)
    {
        return -1;
    }

    for (int i = 0; i < audioFrame._payloadDataLengthInSamples; i++)
    {
        audioFrame._payloadData[i] = (audioFrame._payloadData[2*i] >> 1) +
            (audioFrame._payloadData[2*i+1] >> 1);
    }

    audioFrame._audioChannel = 1;

    return 0;
}

WebRtc_Word32 
AudioFrameOperations::Mute(AudioFrame& audioFrame)
{
    const int sizeInBytes = sizeof(WebRtc_Word16) *
        audioFrame._payloadDataLengthInSamples * audioFrame._audioChannel;
    memset(audioFrame._payloadData, 0, sizeInBytes);
    audioFrame._energy = 0;
    return 0;
}

WebRtc_Word32 
AudioFrameOperations::Scale(const float left,
                            const float right,
                            AudioFrame& audioFrame)
{
    if (audioFrame._audioChannel == 1)
    {
        assert(false);
        return -1;
    }

    for (int i = 0; i < audioFrame._payloadDataLengthInSamples; i++)
    {
        audioFrame._payloadData[2*i] =
            (WebRtc_Word16)(left*audioFrame._payloadData[2*i]);
        audioFrame._payloadData[2*i+1] =
            (WebRtc_Word16)(right*audioFrame._payloadData[2*i+1]);
    }
    return 0;
}

WebRtc_Word32 
AudioFrameOperations::ScaleWithSat(const float scale, AudioFrame& audioFrame)
{
    WebRtc_Word32 tmp(0);

    
    for (int i = 0;
        i < audioFrame._payloadDataLengthInSamples * audioFrame._audioChannel;
        i++)
    {
        tmp = static_cast<WebRtc_Word32> (scale * audioFrame._payloadData[i]);
        if (tmp < -32768)
        {
            audioFrame._payloadData[i] = -32768;
        }
        else if (tmp > 32767)
        {
            audioFrame._payloadData[i] = 32767;
        }
        else
        {
            audioFrame._payloadData[i] = static_cast<WebRtc_Word16> (tmp);
        }
    }
    return 0;
}

}  

}  

