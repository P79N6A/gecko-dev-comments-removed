









#ifndef WEBRTC_MODULES_UTILITY_INTERFACE_FILE_PLAYER_H_
#define WEBRTC_MODULES_UTILITY_INTERFACE_FILE_PLAYER_H_

#include "webrtc/common_types.h"
#include "webrtc/common_video/interface/i420_video_frame.h"
#include "webrtc/engine_configurations.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/typedefs.h"

namespace webrtc {
class FileCallback;

class FilePlayer
{
public:
    
    enum {MAX_AUDIO_BUFFER_IN_SAMPLES = 60*32};
    enum {MAX_AUDIO_BUFFER_IN_BYTES = MAX_AUDIO_BUFFER_IN_SAMPLES*2};

    
    
    static FilePlayer* CreateFilePlayer(const uint32_t instanceID,
                                        const FileFormats fileFormat);

    static void DestroyFilePlayer(FilePlayer* player);

    
    
    
    virtual int Get10msAudioFromFile(
        int16_t* outBuffer,
        int& lengthInSamples,
        int frequencyInHz) = 0;

    
    virtual int32_t RegisterModuleFileCallback(
        FileCallback* callback) = 0;

    
    
    virtual int32_t StartPlayingFile(
        const char* fileName,
        bool loop,
        uint32_t startPosition,
        float volumeScaling,
        uint32_t notification,
        uint32_t stopPosition = 0,
        const CodecInst* codecInst = NULL) = 0;

    
    virtual int32_t StartPlayingFile(
        InStream& sourceStream,
        uint32_t startPosition,
        float volumeScaling,
        uint32_t notification,
        uint32_t stopPosition = 0,
        const CodecInst* codecInst = NULL) = 0;

    virtual int32_t StopPlayingFile() = 0;

    virtual bool IsPlayingFile() const = 0;

    virtual int32_t GetPlayoutPosition(uint32_t& durationMs) = 0;

    
    virtual int32_t AudioCodec(CodecInst& audioCodec) const = 0;

    virtual int32_t Frequency() const = 0;

    
    virtual int32_t SetAudioScaling(float scaleFactor) = 0;

    
    
    
    
    virtual int32_t TimeUntilNextVideoFrame() { return -1;}

    virtual int32_t StartPlayingVideoFile(
        const char* ,
        bool ,
        bool ) { return -1;}

    virtual int32_t video_codec_info(VideoCodec& ) const
    {return -1;}

    virtual int32_t GetVideoFromFile(I420VideoFrame& )
    { return -1;}

    
    
    virtual int32_t GetVideoFromFile(I420VideoFrame& ,
                                     const uint32_t ,
                                     const uint32_t )
    {return -1;}
protected:
    virtual ~FilePlayer() {}

};
}  
#endif 
