














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

    static DeviceInfo* Create(const WebRtc_Word32 id);
    static void Destroy(DeviceInfo* deviceInfo);

    VideoCaptureMacQuickTimeInfo(const WebRtc_Word32 id);
    virtual ~VideoCaptureMacQuickTimeInfo();

    WebRtc_Word32 Init();

    virtual WebRtc_UWord32 NumberOfDevices();

    








    virtual WebRtc_Word32 GetDeviceName(
        WebRtc_UWord32 deviceNumber, char* deviceNameUTF8,
        WebRtc_UWord32 deviceNameLength, char* deviceUniqueIdUTF8,
        WebRtc_UWord32 deviceUniqueIdUTF8Length,
        char* productUniqueIdUTF8 = 0,
        WebRtc_UWord32 productUniqueIdUTF8Length = 0);


    

    


    virtual WebRtc_Word32 NumberOfCapabilities(const char* deviceUniqueIdUTF8);

    


    virtual WebRtc_Word32 GetCapability(
        const char* deviceUniqueIdUTF8,
        const WebRtc_UWord32 deviceCapabilityNumber,
        VideoCaptureCapability& capability);

    



    virtual WebRtc_Word32 GetBestMatchedCapability(
        const char* deviceUniqueIdUTF8,
        const VideoCaptureCapability& requested,
        VideoCaptureCapability& resulting);

    


    virtual WebRtc_Word32 DisplayCaptureSettingsDialogBox(
        const char* deviceUniqueIdUTF8,
        const char* dialogTitleUTF8, void* parentWindow,
        WebRtc_UWord32 positionX, WebRtc_UWord32 positionY);

protected:
    virtual WebRtc_Word32 CreateCapabilityMap(
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
    

    int GetCaptureDevices(WebRtc_UWord32 deviceNumber,
                          char* deviceNameUTF8,
                          WebRtc_UWord32 deviceNameUTF8Length,
                          char* deviceUniqueIdUTF8,
                          WebRtc_UWord32 deviceUniqueIdUTF8Length,
                          char* productUniqueIdUTF8,
                          WebRtc_UWord32 productUniqueIdUTF8Length,
                          int& numberOfDevices);

    static CFIndex PascalStringToCString(const unsigned char* pascalString,
                                         char* cString, CFIndex bufferSize);

private:
    
    WebRtc_Word32 _id;
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
    WebRtc_Word64 _captureDeviceListTime;
    ListWrapper _captureCapabilityList;
};
}  
#endif  
