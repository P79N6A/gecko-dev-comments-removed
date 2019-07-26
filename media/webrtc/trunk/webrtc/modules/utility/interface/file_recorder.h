









#ifndef WEBRTC_MODULES_UTILITY_INTERFACE_FILE_RECORDER_H_
#define WEBRTC_MODULES_UTILITY_INTERFACE_FILE_RECORDER_H_

#include "webrtc/common_types.h"
#include "webrtc/common_video/interface/i420_video_frame.h"
#include "webrtc/engine_configurations.h"
#include "webrtc/modules/audio_coding/main/interface/audio_coding_module_typedefs.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/modules/media_file/interface/media_file_defines.h"
#include "webrtc/system_wrappers/interface/tick_util.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class FileRecorder
{
public:

    
    
    static FileRecorder* CreateFileRecorder(const uint32_t instanceID,
                                            const FileFormats fileFormat);

    static void DestroyFileRecorder(FileRecorder* recorder);

    virtual int32_t RegisterModuleFileCallback(
        FileCallback* callback) = 0;

    virtual FileFormats RecordingFileFormat() const = 0;

    virtual int32_t StartRecordingAudioFile(
        const char* fileName,
        const CodecInst& codecInst,
        uint32_t notification,
        ACMAMRPackingFormat amrFormat = AMRFileStorage) = 0;

    virtual int32_t StartRecordingAudioFile(
        OutStream& destStream,
        const CodecInst& codecInst,
        uint32_t notification,
        ACMAMRPackingFormat amrFormat = AMRFileStorage) = 0;

    
    
    virtual int32_t StopRecording() = 0;

    
    
    virtual bool IsRecording() const = 0;

    virtual int32_t codec_info(CodecInst& codecInst) const = 0;

    
    virtual int32_t RecordAudioToFile(
        const AudioFrame& frame,
        const TickTime* playoutTS = NULL) = 0;

    
    
    
    
    
    
    virtual int32_t StartRecordingVideoFile(
        const char* fileName,
        const CodecInst& audioCodecInst,
        const VideoCodec& videoCodecInst,
        ACMAMRPackingFormat amrFormat = AMRFileStorage,
        bool videoOnly = false) = 0;

    
    virtual int32_t RecordVideoToFile(
        const I420VideoFrame& videoFrame) = 0;

protected:
    virtual ~FileRecorder() {}

};
}  
#endif 
