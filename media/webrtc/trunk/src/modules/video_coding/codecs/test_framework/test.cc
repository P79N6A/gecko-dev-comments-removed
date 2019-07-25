









#include "test.h"

#include <cstring>
#include <iostream>

#include "testsupport/metrics/video_metrics.h"

using namespace webrtc;

long filesize(const char *filename); 

Test::Test(std::string name, std::string description)
:
_bitRate(0),
_inname(""),
_outname(""),
_encodedName(""),
_name(name),
_description(description)
{
    memset(&_inst, 0, sizeof(_inst));
    unsigned int seed = static_cast<unsigned int>(0);
    std::srand(seed);
}

Test::Test(std::string name, std::string description, WebRtc_UWord32 bitRate)
:
_bitRate(bitRate),
_inname(""),
_outname(""),
_encodedName(""),
_name(name),
_description(description)
{
    memset(&_inst, 0, sizeof(_inst));
    unsigned int seed = static_cast<unsigned int>(0);
    std::srand(seed);
}

void
Test::Print()
{
    std::cout << _name << " completed!" << std::endl;
    (*_log) << _name << std::endl;
    (*_log) << _description << std::endl;
    (*_log) << "Input file: " << _inname << std::endl;
    (*_log) << "Output file: " << _outname << std::endl;
    webrtc::test::QualityMetricsResult psnr;
    webrtc::test::QualityMetricsResult ssim;
    I420PSNRFromFiles(_inname.c_str(), _outname.c_str(), _inst.width,
                      _inst.height, &psnr);
    I420SSIMFromFiles(_inname.c_str(), _outname.c_str(), _inst.width,
                      _inst.height, &ssim);

    (*_log) << "PSNR: " << psnr.average << std::endl;
    std::cout << "PSNR: " << psnr.average << std::endl << std::endl;
    (*_log) << "SSIM: " << ssim.average << std::endl;
    std::cout << "SSIM: " << ssim.average << std::endl << std::endl;
    (*_log) << std::endl;
}

void
Test::Setup()
{
    int widhei          = _inst.width*_inst.height;
    _lengthSourceFrame  = 3*widhei/2;
    _sourceBuffer       = new unsigned char[_lengthSourceFrame];
}

void
Test::CodecSettings(int width, int height, WebRtc_UWord32 frameRate , WebRtc_UWord32 bitRate )
{
    if (bitRate > 0)
    {
        _bitRate = bitRate;
    }
    else if (_bitRate == 0)
    {
        _bitRate = 600;
    }
    _inst.codecType = kVideoCodecVP8;
    _inst.codecSpecific.VP8.feedbackModeOn = true;
    _inst.maxFramerate = (unsigned char)frameRate;
    _inst.startBitrate = (int)_bitRate;
    _inst.maxBitrate = 8000;
    _inst.width = width;
    _inst.height = height;
}

void
Test::Teardown()
{
    delete [] _sourceBuffer;
}

void
Test::SetEncoder(webrtc::VideoEncoder*encoder)
{
    _encoder = encoder;
}

void
Test::SetDecoder(VideoDecoder*decoder)
{
    _decoder = decoder;
}

void
Test::SetLog(std::fstream* log)
{
    _log = log;
}

double Test::ActualBitRate(int nFrames)
{
    return 8.0 * _sumEncBytes / (nFrames / _inst.maxFramerate);
}

bool Test::PacketLoss(double lossRate, int )
{
    return RandUniform() < lossRate;
}

void
Test::VideoBufferToRawImage(TestVideoBuffer& videoBuffer, RawImage &image)
{
    image._buffer = videoBuffer.GetBuffer();
    image._size = videoBuffer.GetSize();
    image._length = videoBuffer.GetLength();
    image._width = videoBuffer.GetWidth();
    image._height = videoBuffer.GetHeight();
    image._timeStamp = videoBuffer.GetTimeStamp();
}
void
Test::VideoEncodedBufferToEncodedImage(TestVideoEncodedBuffer& videoBuffer, EncodedImage &image)
{
    image._buffer = videoBuffer.GetBuffer();
    image._length = videoBuffer.GetLength();
    image._size = videoBuffer.GetSize();
    image._frameType = static_cast<VideoFrameType>(videoBuffer.GetFrameType());
    image._timeStamp = videoBuffer.GetTimeStamp();
    image._encodedWidth = videoBuffer.GetCaptureWidth();
    image._encodedHeight = videoBuffer.GetCaptureHeight();
    image._completeFrame = true;
}

long filesize(const char *filename)
{
    FILE *f = fopen(filename,"rb");  
    long size = 0;
    if (fseek(f,0,SEEK_END)==0) 
        size = ftell(f);
    fclose(f);
    return size;
}
