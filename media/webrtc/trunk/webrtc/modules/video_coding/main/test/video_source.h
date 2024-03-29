









#ifndef WEBRTC_MODULES_VIDEO_CODING_TEST_VIDEO_SOURCE_H_
#define WEBRTC_MODULES_VIDEO_CODING_TEST_VIDEO_SOURCE_H_

#include "webrtc/common_video/libyuv/include/webrtc_libyuv.h"
#include "webrtc/typedefs.h"

#include <string>

enum VideoSize
    {
        kUndefined,
        kSQCIF,     
        kQQVGA,     
        kQCIF,      
        kCGA,       
        kQVGA,      
        kSIF,       
        kWQVGA,     
        kCIF,       
        kW288p,     
        k448p,      
        kVGA,       
        k432p,      
        kW432p,     
        k4SIF,      
        kW448p,     
        kNTSC,		
        kFW448p,    
        kWVGA,      
        k4CIF,      
        kSVGA,      
        kW544p,     
        kW576p,     
        kHD,        
        kXGA,       
        kWHD,       
        kFullHD,   
        kWFullHD,  

        kNumberOfVideoSizes
    };


class VideoSource
{
public:
  VideoSource();
  VideoSource(std::string fileName, VideoSize size, float frameRate, webrtc::VideoType type = webrtc::kI420);
  VideoSource(std::string fileName, uint16_t width, uint16_t height,
      float frameRate = 30, webrtc::VideoType type = webrtc::kI420);

    std::string GetFileName() const { return _fileName; }
    uint16_t  GetWidth() const { return _width; }
    uint16_t GetHeight() const { return _height; }
    webrtc::VideoType GetType() const { return _type; }
    float GetFrameRate() const { return _frameRate; }
    int GetWidthHeight( VideoSize size);

    
    std::string GetName() const;

    int32_t GetFrameLength() const;

private:
    std::string         _fileName;
    uint16_t      _width;
    uint16_t      _height;
    webrtc::VideoType   _type;
    float               _frameRate;
};

#endif 
