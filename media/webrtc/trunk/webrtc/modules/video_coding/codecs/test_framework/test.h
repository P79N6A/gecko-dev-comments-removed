









#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_TEST_FRAWEWORK_TEST_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_TEST_FRAWEWORK_TEST_H_

#include "modules/interface/module_common_types.h"
#include "video_codec_interface.h"
#include <string>
#include <fstream>
#include <cstdlib>

class CodecTest
{
public:
    CodecTest(std::string name, std::string description);
    CodecTest(std::string name, std::string description,
              WebRtc_UWord32 bitRate);
    virtual ~CodecTest() {};
    virtual void Setup();
    virtual void Perform()=0;
    virtual void Print();
    void SetEncoder(webrtc::VideoEncoder *encoder);
    void SetDecoder(webrtc::VideoDecoder *decoder);
    void SetLog(std::fstream* log);

protected:
    virtual void CodecSettings(int width,
                               int height,
                               WebRtc_UWord32 frameRate=30,
                               WebRtc_UWord32 bitRate=0);
    virtual void Teardown();
    double ActualBitRate(int nFrames);
    virtual bool PacketLoss(double lossRate, int );
    static double RandUniform() { return (std::rand() + 1.0)/(RAND_MAX + 1.0); }
    static void VideoEncodedBufferToEncodedImage(
        webrtc::VideoFrame& videoBuffer,
        webrtc::EncodedImage &image);

    webrtc::VideoEncoder*   _encoder;
    webrtc::VideoDecoder*   _decoder;
    WebRtc_UWord32          _bitRate;
    unsigned int            _lengthSourceFrame;
    unsigned char*          _sourceBuffer;
    webrtc::I420VideoFrame  _inputVideoBuffer;
    
    
    webrtc::VideoFrame      _encodedVideoBuffer;
    webrtc::I420VideoFrame  _decodedVideoBuffer;
    webrtc::VideoCodec      _inst;
    std::fstream*           _log;
    std::string             _inname;
    std::string             _outname;
    std::string             _encodedName;
    int                     _sumEncBytes;
    int                     _width;
    int                     _halfWidth;
    int                     _height;
    int                     _halfHeight;
    int                     _sizeY;
    int                     _sizeUv;
private:
    std::string             _name;
    std::string             _description;

};

#endif 
