









#ifndef WEBRTC_MODULES_UTILITY_INTERFACE_FILE_RECORDER_H_
#define WEBRTC_MODULES_UTILITY_INTERFACE_FILE_RECORDER_H_

#include "common_video/interface/i420_video_frame.h"
#include "common_types.h"
#include "engine_configurations.h"
#include "modules/audio_coding/main/interface/audio_coding_module_typedefs.h"
#include "modules/interface/module_common_types.h"
#include "modules/media_file/interface/media_file_defines.h"
#include "system_wrappers/interface/tick_util.h"
#include "typedefs.h"

namespace webrtc {

class FileRecorder
{
public:

    
    
    static FileRecorder* CreateFileRecorder(const WebRtc_UWord32 instanceID,
                                            const FileFormats fileFormat);

    static void DestroyFileRecorder(FileRecorder* recorder);

    virtual WebRtc_Word32 RegisterModuleFileCallback(
        FileCallback* callback) = 0;

    virtual FileFormats RecordingFileFormat() const = 0;

    virtual WebRtc_Word32 StartRecordingAudioFile(
        const char* fileName,
        const CodecInst& codecInst,
        WebRtc_UWord32 notification,
        ACMAMRPackingFormat amrFormat = AMRFileStorage) = 0;

    virtual WebRtc_Word32 StartRecordingAudioFile(
        OutStream& destStream,
        const CodecInst& codecInst,
        WebRtc_UWord32 notification,
        ACMAMRPackingFormat amrFormat = AMRFileStorage) = 0;

    
    
    virtual WebRtc_Word32 StopRecording() = 0;

    
    
    virtual bool IsRecording() const = 0;

    virtual WebRtc_Word32 codec_info(CodecInst& codecInst) const = 0;

    
    virtual WebRtc_Word32 RecordAudioToFile(
        const AudioFrame& frame,
        const TickTime* playoutTS = NULL) = 0;

    
    
    
    
    
    
    virtual WebRtc_Word32 StartRecordingVideoFile(
        const char* fileName,
        const CodecInst& audioCodecInst,
        const VideoCodec& videoCodecInst,
        ACMAMRPackingFormat amrFormat = AMRFileStorage,
        bool videoOnly = false) = 0;

    
    virtual WebRtc_Word32 RecordVideoToFile(
        const I420VideoFrame& videoFrame) = 0;

protected:
    virtual ~FileRecorder() {}

};
} 
#endif 
