









#ifndef WEBRTC_MODULES_VIDEO_CODING_TEST_QUALITY_MODSE_TEST_H_
#define WEBRTC_MODULES_VIDEO_CODING_TEST_QUALITY_MODSE_TEST_H_

#include "webrtc/modules/video_coding/main/interface/video_coding_defines.h"
#include "webrtc/modules/video_coding/main/test/normal_test.h"
#include "webrtc/modules/video_processing/main/interface/video_processing.h"
#include "webrtc/system_wrappers/interface/data_log.h"

int qualityModeTest(const CmdArgs& args);

class QualityModesTest : public NormalTest
{
public:
    QualityModesTest(webrtc::VideoCodingModule* vcm,
                     webrtc::Clock* clock);
    virtual ~QualityModesTest();
    int32_t Perform(const CmdArgs& args);

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

    uint32_t                      _numFramesDroppedVPM;
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
    
    int32_t FrameToRender(webrtc::I420VideoFrame& videoFrame);
    int32_t DecodedBytes();
    void SetOriginalFrameDimensions(int32_t width, int32_t height);
    int32_t buildInterpolator();
    
    void WriteEnd(int input_tot_frame_count);

private:
    FILE*                _decodedFile;
    uint32_t       _decodedBytes;
   
    int                  _origWidth;
    int                  _origHeight;
    int                  _decWidth;
    int                  _decHeight;

    uint8_t*       _decBuffer;
    uint32_t       _frameCnt; 
    webrtc::I420VideoFrame last_frame_;
    int                  frame_rate_;
    int                  frames_cnt_since_drop_;
    std::string          feature_table_name_;



}; 

class QMTestVideoSettingsCallback : public webrtc::VCMQMSettingsCallback
{
public:
    QMTestVideoSettingsCallback();
    
    int32_t SetVideoQMSettings(const uint32_t frameRate,
                                     const uint32_t width,
                                     const uint32_t height);
    
    void RegisterVPM(webrtc::VideoProcessingModule* vpm);
    void RegisterVCM(webrtc::VideoCodingModule* vcm);
    bool Updated();

private:
    webrtc::VideoProcessingModule*         _vpm;
    webrtc::VideoCodingModule*             _vcm;
    bool                                   _updated;
};


#endif 
