









#ifndef WEBRTC_MODULES_MEDIA_FILE_INTERFACE_MEDIA_FILE_DEFINES_H_
#define WEBRTC_MODULES_MEDIA_FILE_INTERFACE_MEDIA_FILE_DEFINES_H_

#include "engine_configurations.h"
#include "module_common_types.h"
#include "typedefs.h"

namespace webrtc {

class FileCallback
{
public:
    virtual ~FileCallback(){}

    
    
    
    virtual void PlayNotification(const WebRtc_Word32 id,
                                  const WebRtc_UWord32 durationMs) = 0;

    
    
    
    virtual void RecordNotification(const WebRtc_Word32 id,
                                    const WebRtc_UWord32 durationMs) = 0;

    
    
    
    virtual void PlayFileEnded(const WebRtc_Word32 id) = 0;

    
    
    
    virtual void RecordFileEnded(const WebRtc_Word32 id) = 0;

protected:
    FileCallback() {}
};
} 
#endif 
