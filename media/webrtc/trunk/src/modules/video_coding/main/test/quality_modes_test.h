









#ifndef WEBRTC_MODULES_VIDEO_CODING_TEST_QUALITY_MODSE_TEST_H_
#define WEBRTC_MODULES_VIDEO_CODING_TEST_QUALITY_MODSE_TEST_H_

#include "video_processing.h"
#include "normal_test.h"
#include "video_coding_defines.h"

int qualityModeTest();

class QualityModesTest : public NormalTest
{
public:
    QualityModesTest(webrtc::VideoCodingModule* vcm,
                     webrtc::TickTimeBase* clock);
    virtual ~QualityModesTest();
    WebRtc_Word32 Perform();

private:

    void Setup();
    void Print();
    void Teardown();
    void SsimComp();

    webrtc::VideoProcessingModule*  _vpm;

    WebRtc_UWord32                      _width;
    WebRtc_UWord32                      _height;
    float                               _frameRate;
    WebRtc_UWord32                      _nativeWidth;
    WebRtc_UWord32                      _nativeHeight;
    float                               _nativeFrameRate;

    WebRtc_UWord32                      _numFramesDroppedVPM;
    bool                                _flagSSIM;

}; 


class VCMQMDecodeCompleCallback: public webrtc::VCMReceiveCallback
{
public:
    VCMQMDecodeCompleCallback(FILE* decodedFile);
    virtual ~VCMQMDecodeCompleCallback();
    void SetUserReceiveCallback(webrtc::VCMReceiveCallback* receiveCallback);
    
    WebRtc_Word32 FrameToRender(webrtc::VideoFrame& videoFrame);
    WebRtc_Word32 DecodedBytes();
    void SetOriginalFrameDimensions(WebRtc_Word32 width, WebRtc_Word32 height);
    WebRtc_Word32 buildInterpolator();
private:
    FILE*                _decodedFile;
    WebRtc_UWord32       _decodedBytes;
   
    WebRtc_UWord32       _origWidth;
    WebRtc_UWord32       _origHeight;
    WebRtc_UWord32       _decWidth;
    WebRtc_UWord32       _decHeight;

    WebRtc_UWord8*       _decBuffer;
    WebRtc_UWord32       _frameCnt; 

}; 

class QMTestVideoSettingsCallback : public webrtc::VCMQMSettingsCallback
{
public:
    QMTestVideoSettingsCallback();
    
    WebRtc_Word32 SetVideoQMSettings(const WebRtc_UWord32 frameRate,
                                     const WebRtc_UWord32 width,
                                     const WebRtc_UWord32 height);
    
    void RegisterVPM(webrtc::VideoProcessingModule* vpm);
    void RegisterVCM(webrtc::VideoCodingModule* vcm);
    bool Updated();

private:
    webrtc::VideoProcessingModule*         _vpm;
    webrtc::VideoCodingModule*             _vcm;
    bool                                   _updated;
};


#endif 
