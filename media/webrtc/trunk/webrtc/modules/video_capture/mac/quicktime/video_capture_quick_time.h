














#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_MAC_QUICKTIME_VIDEO_CAPTURE_QUICK_TIME_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_MAC_QUICKTIME_VIDEO_CAPTURE_QUICK_TIME_H_

#include <QuickTime/QuickTime.h>


#include "../../device_info_impl.h"
#include "../../video_capture_impl.h"
#include "list_wrapper.h"


#define START_CODEC_WIDTH 352
#define START_CODEC_HEIGHT 288
#define SLEEP(x) usleep(x * 1000);

namespace webrtc
{
class CriticalSectionWrapper;
class EventWrapper;
class ThreadWrapper;

class VideoCaptureMacQuickTime : public VideoCaptureImpl
{

public:
	VideoCaptureMacQuickTime(const WebRtc_Word32 id);
	virtual ~VideoCaptureMacQuickTime();

	static void Destroy(VideoCaptureModule* module);

    WebRtc_Word32 Init(const WebRtc_Word32 id,
                       const WebRtc_UWord8* deviceUniqueIdUTF8);
	virtual WebRtc_Word32 StartCapture(
	    const VideoCaptureCapability& capability);
    virtual WebRtc_Word32 StopCapture();
	virtual bool CaptureStarted();
	virtual WebRtc_Word32 CaptureSettings(VideoCaptureCapability& settings);

    
    int VideoCaptureInitThreadContext();
    int VideoCaptureTerminate();
    int VideoCaptureSetCaptureDevice(const char* deviceName, int size);
	int UpdateCaptureSettings(int channel, webrtc::VideoCodec& inst, bool def);
    int VideoCaptureRun();
    int VideoCaptureStop();

protected:

private: 

    struct VideoCaptureMacName
    {
        VideoCaptureMacName();

        enum { kVideoCaptureMacNameMaxSize = 64};
        char _name[kVideoCaptureMacNameMaxSize];
        CFIndex _size;
    };

    
    enum { kVideoCaptureDeviceListTimeout =     5000};
    
    enum { kYuy2_1280_1024_length = 2621440};

private:

    
    static OSErr SendProcess(SGChannel sgChannel, Ptr p, long len, long *offset,
                             long chRefCon, TimeValue time, short writeType,
                             long refCon);
    int SendFrame(SGChannel sgChannel, char* data, long length, TimeValue time);

    
    int CreateLocalGWorld(int width, int height);
    int RemoveLocalGWorld();
    int ConnectCaptureDevice();
    int DisconnectCaptureDevice();
    virtual bool IsCaptureDeviceSelected();

    
    static bool GrabberUpdateThread(void*);
    bool GrabberUpdateProcess();

    
    int StartQuickTimeCapture();
    int StopQuickTimeCapture(bool* wasCapturing = NULL);

    static CFIndex PascalStringToCString(const unsigned char* pascalString,
                                         char* cString,
                                         CFIndex bufferSize);

private: 
	WebRtc_Word32			_id;
	bool					_isCapturing;
	VideoCaptureCapability	_captureCapability;
    CriticalSectionWrapper* _grabberCritsect;
    CriticalSectionWrapper* _videoMacCritsect;
    bool                    _terminated;
    webrtc::ThreadWrapper*  _grabberUpdateThread;
    webrtc::EventWrapper*           _grabberUpdateEvent;
    SeqGrabComponent        _captureGrabber;
    Component               _captureDevice;
    char                    _captureDeviceDisplayName[64];
	RawVideoType		    _captureVideoType;
    bool                    _captureIsInitialized;
    GWorldPtr               _gWorld;
    SGChannel               _captureChannel;
    ImageSequence           _captureSequence;
    bool                    _sgPrepared;
    bool                    _sgStarted;
    int                     _trueCaptureWidth;
    int                     _trueCaptureHeight;
    ListWrapper             _captureDeviceList;
    unsigned long           _captureDeviceListTime;
    ListWrapper             _captureCapabilityList;
};
}  
#endif  
