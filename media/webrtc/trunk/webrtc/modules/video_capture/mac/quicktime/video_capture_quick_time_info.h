














#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_MAC_QUICKTIME_VIDEO_CAPTURE_QUICK_TIME_INFO_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_MAC_QUICKTIME_VIDEO_CAPTURE_QUICK_TIME_INFO_H_

#include <QuickTime/QuickTime.h>

#include "../../video_capture_impl.h"
#include "../../device_info_impl.h"
#include "list_wrapper.h"
#include "map_wrapper.h"

class VideoRenderCallback;

namespace webrtc
{
class CriticalSectionWrapper;
class EventWrapper;
class ThreadWrapper;
class Trace;

class VideoCaptureMacQuickTimeInfo: public DeviceInfoImpl
{
public:

    static DeviceInfo* Create(const int32_t id);
    static void Destroy(DeviceInfo* deviceInfo);

    VideoCaptureMacQuickTimeInfo(const int32_t id);
    virtual ~VideoCaptureMacQuickTimeInfo();

    int32_t Init();

    virtual uint32_t NumberOfDevices();

    








    virtual int32_t GetDeviceName(
        uint32_t deviceNumber, char* deviceNameUTF8,
        uint32_t deviceNameLength, char* deviceUniqueIdUTF8,
        uint32_t deviceUniqueIdUTF8Length,
        char* productUniqueIdUTF8 = 0,
        uint32_t productUniqueIdUTF8Length = 0);


    

    


    virtual int32_t NumberOfCapabilities(const char* deviceUniqueIdUTF8);

    


    virtual int32_t GetCapability(
        const char* deviceUniqueIdUTF8,
        const uint32_t deviceCapabilityNumber,
        VideoCaptureCapability& capability);

    



    virtual int32_t GetBestMatchedCapability(
        const char* deviceUniqueIdUTF8,
        const VideoCaptureCapability& requested,
        VideoCaptureCapability& resulting);

    


    virtual int32_t DisplayCaptureSettingsDialogBox(
        const char* deviceUniqueIdUTF8,
        const char* dialogTitleUTF8, void* parentWindow,
        uint32_t positionX, uint32_t positionY);

protected:
    virtual int32_t CreateCapabilityMap(
        const char* deviceUniqueIdUTF8);

private:

    struct VideoCaptureMacName
    {
        VideoCaptureMacName();

        enum
        {
            kVideoCaptureMacNameMaxSize = 64
        };
        char _name[kVideoCaptureMacNameMaxSize];
        CFIndex _size;
    };

    enum
    {
        kVideoCaptureMacDeviceListTimeout = 5000
    }; 
    enum
    {
        kYuy2_1280_1024_length = 2621440
    }; 

private:
    

    int GetCaptureDevices(uint32_t deviceNumber,
                          char* deviceNameUTF8,
                          uint32_t deviceNameUTF8Length,
                          char* deviceUniqueIdUTF8,
                          uint32_t deviceUniqueIdUTF8Length,
                          char* productUniqueIdUTF8,
                          uint32_t productUniqueIdUTF8Length,
                          int& numberOfDevices);

    static CFIndex PascalStringToCString(const unsigned char* pascalString,
                                         char* cString, CFIndex bufferSize);

private:
    
    int32_t _id;
    bool _terminated;
    CriticalSectionWrapper* _grabberCritsect;
    webrtc::Trace* _trace;
    webrtc::ThreadWrapper* _grabberUpdateThread;
    webrtc::EventWrapper* _grabberUpdateEvent;
    SeqGrabComponent _captureGrabber;
    Component _captureDevice;
    char _captureDeviceDisplayName[64];
    bool _captureIsInitialized;
    GWorldPtr _gWorld;
    SGChannel _captureChannel;
    ImageSequence _captureSequence;
    bool _sgPrepared;
    bool _sgStarted;
    int _codecWidth;
    int _codecHeight;
    int _trueCaptureWidth;
    int _trueCaptureHeight;
    ListWrapper _captureDeviceList;
    int64_t _captureDeviceListTime;
    ListWrapper _captureCapabilityList;
};
}  
#endif  
