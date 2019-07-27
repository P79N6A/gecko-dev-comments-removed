









#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_WINDOWS_VIDEO_CAPTURE_DS_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_WINDOWS_VIDEO_CAPTURE_DS_H_

#include "webrtc/modules/video_capture/video_capture_impl.h"
#include "webrtc/modules/video_capture/windows/device_info_ds.h"

#define CAPTURE_FILTER_NAME L"VideoCaptureFilter"
#define SINK_FILTER_NAME L"SinkFilter"

namespace webrtc
{
namespace videocapturemodule
{

class CaptureSinkFilter;

class VideoCaptureDS: public VideoCaptureImpl
{
public:
    VideoCaptureDS(const int32_t id);

    virtual int32_t Init(const int32_t id, const char* deviceUniqueIdUTF8);

    




    virtual int32_t
        StartCapture(const VideoCaptureCapability& capability);
    virtual int32_t StopCapture();

    





    virtual bool CaptureStarted();
    virtual int32_t CaptureSettings(VideoCaptureCapability& settings);

protected:
    virtual ~VideoCaptureDS();

    
    int32_t
        SetCameraOutputIfNeeded(const VideoCaptureCapability& requestedCapability);
    int32_t
        SetCameraOutput(const VideoCaptureCapability& requestedCapability,
                        int32_t capabilityIndex);

    int32_t DisconnectGraph();
    HRESULT VideoCaptureDS::ConnectDVCamera();

    DeviceInfoDS _dsInfo;
    VideoCaptureCapability _activeCapability;

    IBaseFilter* _captureFilter;
    IGraphBuilder* _graphBuilder;
    IMediaControl* _mediaControl;
    CaptureSinkFilter* _sinkFilter;
    IPin* _inputSendPin;
    IPin* _outputCapturePin;

    
    IBaseFilter* _dvFilter;
    IPin* _inputDvPin;
    IPin* _outputDvPin;

};
}  
}  
#endif 
