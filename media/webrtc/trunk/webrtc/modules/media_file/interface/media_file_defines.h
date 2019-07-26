









#ifndef WEBRTC_MODULES_MEDIA_FILE_INTERFACE_MEDIA_FILE_DEFINES_H_
#define WEBRTC_MODULES_MEDIA_FILE_INTERFACE_MEDIA_FILE_DEFINES_H_

#include "webrtc/engine_configurations.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class FileCallback
{
public:
    virtual ~FileCallback(){}

    
    
    
    virtual void PlayNotification(const int32_t id,
                                  const uint32_t durationMs) = 0;

    
    
    
    virtual void RecordNotification(const int32_t id,
                                    const uint32_t durationMs) = 0;

    
    
    
    virtual void PlayFileEnded(const int32_t id) = 0;

    
    
    
    virtual void RecordFileEnded(const int32_t id) = 0;

protected:
    FileCallback() {}
};
}  
#endif 
