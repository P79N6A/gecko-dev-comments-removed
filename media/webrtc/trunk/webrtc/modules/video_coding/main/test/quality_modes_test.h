









#ifndef WEBRTC_MODULES_VIDEO_CODING_TEST_QUALITY_MODSE_TEST_H_
#define WEBRTC_MODULES_VIDEO_CODING_TEST_QUALITY_MODSE_TEST_H_

#include "video_processing.h"
#include "normal_test.h"
#include "system_wrappers/interface/data_log.h"
#include "video_coding_defines.h"

int qualityModeTest(const CmdArgs& args);

class QualityModesTest : public NormalTest
{
public:
    QualityModesTest(webrtc::VideoCodingModule* vcm,
                     webrtc::TickTimeBase* clock);
    virtual ~QualityModesTest();
    WebRtc_Word32 Perform(const CmdArgs& args);

private:

    void Setup(const CmdArgs& args);
    void Print();
    void Teardown();
    void SsimComp();

    webrtc::VideoProcessingModule*  _vpm;

    int                                 _width;
    int                                 _height;
    float                               _frameRate;
    int                                 _nativeWidth;
    int                                 _nativeHeight;
    float                               _nativeFrameRate;

    WebRtc_UWord32                      _numFramesDroppedVPM;
    bool                                _flagSSIM;
    std::string                         filename_testvideo_;
    std::string                         fv_outfilename_;

    std::string                         feature_table_name_;

}; 

class VCMQMDecodeCompleCallback: public webrtc::VCMReceiveCallback
{
public:
    VCMQMDecodeCompleCallback(
        FILE* decodedFile,
        int frame_rate,
        std::string feature_table_name);
    virtual ~VCMQMDecodeCompleCallback();
    void SetUserReceiveCallback(webrtc::VCMReceiveCallback* receiveCallback);
    
    WebRtc_Word32 FrameToRender(webrtc::I420VideoFrame& videoFrame);
    WebRtc_Word32 DecodedBytes();
    void SetOriginalFrameDimensions(WebRtc_Word32 width, WebRtc_Word32 height);
    WebRtc_Word32 buildInterpolator();
    
    void WriteEnd(int input_tot_frame_count);

private:
    FILE*                _decodedFile;
    WebRtc_UWord32       _decodedBytes;
   
    int                  _origWidth;
    int                  _origHeight;
    int                  _decWidth;
    int                  _decHeight;

    WebRtc_UWord8*       _decBuffer;
    WebRtc_UWord32       _frameCnt; 
    webrtc::I420VideoFrame last_frame_;
    int                  frame_rate_;
    int                  frames_cnt_since_drop_;
    std::string          feature_table_name_;



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
