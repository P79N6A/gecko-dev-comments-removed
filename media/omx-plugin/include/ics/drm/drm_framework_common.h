















#ifndef __DRM_FRAMEWORK_COMMON_H__
#define __DRM_FRAMEWORK_COMMON_H__

#include <utils/Vector.h>
#include <utils/KeyedVector.h>
#include <utils/RefBase.h>
#include <utils/String8.h>
#include <utils/Errors.h>

#define INVALID_VALUE -1

namespace android {




enum {
    
    
    ERROR_BASE = -2000,

    DRM_ERROR_UNKNOWN                       = ERROR_BASE,
    DRM_ERROR_NO_LICENSE                    = ERROR_BASE - 1,
    DRM_ERROR_LICENSE_EXPIRED               = ERROR_BASE - 2,
    DRM_ERROR_SESSION_NOT_OPENED            = ERROR_BASE - 3,
    DRM_ERROR_DECRYPT_UNIT_NOT_INITIALIZED  = ERROR_BASE - 4,
    DRM_ERROR_DECRYPT                       = ERROR_BASE - 5,
    DRM_ERROR_CANNOT_HANDLE                 = ERROR_BASE - 6,
    DRM_ERROR_TAMPER_DETECTED               = ERROR_BASE - 7,

    DRM_NO_ERROR                            = NO_ERROR
};




enum DrmCopyControl {
    DRM_COPY_CONTROL_BASE = 1000,
    
    
    
    DRM_COPY_CONTROL_HDCP = DRM_COPY_CONTROL_BASE
};




class DrmBuffer {
public:
    char* data;
    int length;

    DrmBuffer() :
        data(NULL),
        length(0) {
    }

    DrmBuffer(char* dataBytes, int dataLength) :
        data(dataBytes),
        length(dataLength) {
    }

};




class ActionDescription {
public:
    ActionDescription(int _outputType, int _configuration) :
        outputType(_outputType),
        configuration(_configuration) {
    }

public:
    int outputType;   
    int configuration; 
};




class DrmObjectType {
private:
    DrmObjectType();

public:
    


    static const int UNKNOWN = 0x00;
    


    static const int CONTENT = 0x01;
    


    static const int RIGHTS_OBJECT = 0x02;
    


    static const int TRIGGER_OBJECT = 0x03;
};




class Playback {
private:
    Playback();

public:
    


    static const int START = 0x00;
    


    static const int STOP = 0x01;
    


    static const int PAUSE = 0x02;
    


    static const int RESUME = 0x03;
};




class Action {
private:
    Action();

public:
    


    static const int DEFAULT = 0x00;
    


    static const int PLAY = 0x01;
    


    static const int RINGTONE = 0x02;
    


    static const int TRANSFER = 0x03;
    


    static const int OUTPUT = 0x04;
    


    static const int PREVIEW = 0x05;
    


    static const int EXECUTE = 0x06;
    


    static const int DISPLAY = 0x07;
};




class RightsStatus {
private:
    RightsStatus();

public:
    


    static const int RIGHTS_VALID = 0x00;
    


    static const int RIGHTS_INVALID = 0x01;
    


    static const int RIGHTS_EXPIRED = 0x02;
    


    static const int RIGHTS_NOT_ACQUIRED = 0x03;
};




class DecryptApiType {
private:
    DecryptApiType();

public:
    


    static const int NON_ENCRYPTED = 0x00;
    


    static const int ELEMENTARY_STREAM_BASED = 0x01;
    


    static const int CONTAINER_BASED = 0x02;
    


    static const int WV_BASED = 0x3;
};




class DecryptInfo {
public:
    


    int decryptBufferLength;
    


};




class DecryptHandle : public RefBase {
public:
    


    int decryptId;
    



    String8 mimeType;
    
















    int decryptApiType;
    



    int status;
    



    DecryptInfo* decryptInfo;
    



    KeyedVector<DrmCopyControl, int> copyControlVector;

    



    KeyedVector<String8, String8> extendedData;

public:
    DecryptHandle():
            decryptId(INVALID_VALUE),
            mimeType(""),
            decryptApiType(INVALID_VALUE),
            status(INVALID_VALUE),
            decryptInfo(NULL) {

    }

    ~DecryptHandle() {
        delete decryptInfo; decryptInfo = NULL;
    }

    bool operator<(const DecryptHandle& handle) const {
        return (decryptId < handle.decryptId);
    }

    bool operator==(const DecryptHandle& handle) const {
        return (decryptId == handle.decryptId);
    }
};

};

#endif 

