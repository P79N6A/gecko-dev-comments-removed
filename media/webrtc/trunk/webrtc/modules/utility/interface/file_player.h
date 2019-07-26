









#ifndef WEBRTC_MODULES_UTILITY_INTERFACE_FILE_PLAYER_H_
#define WEBRTC_MODULES_UTILITY_INTERFACE_FILE_PLAYER_H_

#include "common_types.h"
#include "common_video/interface/i420_video_frame.h"
#include "engine_configurations.h"
#include "module_common_types.h"
#include "typedefs.h"

namespace webrtc {
class FileCallback;

class FilePlayer
{
public:
    
    enum {MAX_AUDIO_BUFFER_IN_SAMPLES = 60*32};
    enum {MAX_AUDIO_BUFFER_IN_BYTES = MAX_AUDIO_BUFFER_IN_SAMPLES*2};

    
    
    static FilePlayer* CreateFilePlayer(const WebRtc_UWord32 instanceID,
                                        const FileFormats fileFormat);

    static void DestroyFilePlayer(FilePlayer* player);

    
    
    
    virtual int Get10msAudioFromFile(
        int16_t* outBuffer,
        int& lengthInSamples,
        int frequencyInHz) = 0;

    
    virtual WebRtc_Word32 RegisterModuleFileCallback(
        FileCallback* callback) = 0;

    
    
    virtual WebRtc_Word32 StartPlayingFile(
        const char* fileName,
        bool loop,
        WebRtc_UWord32 startPosition,
        float volumeScaling,
        WebRtc_UWord32 notification,
        WebRtc_UWord32 stopPosition = 0,
        const CodecInst* codecInst = NULL) = 0;

    
    virtual WebRtc_Word32 StartPlayingFile(
        InStream& sourceStream,
        WebRtc_UWord32 startPosition,
        float volumeScaling,
        WebRtc_UWord32 notification,
        WebRtc_UWord32 stopPosition = 0,
        const CodecInst* codecInst = NULL) = 0;

    virtual WebRtc_Word32 StopPlayingFile() = 0;

    virtual bool IsPlayingFile() const = 0;

    virtual WebRtc_Word32 GetPlayoutPosition(WebRtc_UWord32& durationMs) = 0;

    
    virtual WebRtc_Word32 AudioCodec(CodecInst& audioCodec) const = 0;

    virtual WebRtc_Word32 Frequency() const = 0;

    
    virtual WebRtc_Word32 SetAudioScaling(float scaleFactor) = 0;

    
    
    
    
    virtual WebRtc_Word32 TimeUntilNextVideoFrame() { return -1;}

    virtual WebRtc_Word32 StartPlayingVideoFile(
        const char* ,
        bool ,
        bool ) { return -1;}

    virtual WebRtc_Word32 video_codec_info(VideoCodec& ) const
    {return -1;}

    virtual WebRtc_Word32 GetVideoFromFile(I420VideoFrame& )
    { return -1;}

    
    
    virtual WebRtc_Word32 GetVideoFromFile(I420VideoFrame& ,
                                           const WebRtc_UWord32 ,
                                           const WebRtc_UWord32 )
    {return -1;}
protected:
    virtual ~FilePlayer() {}

};
} 
#endif 
