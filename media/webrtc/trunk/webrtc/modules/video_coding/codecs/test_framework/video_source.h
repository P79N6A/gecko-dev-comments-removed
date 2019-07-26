









#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_TEST_FRAMEWORK_VIDEO_SOURCE_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_TEST_FRAMEWORK_VIDEO_SOURCE_H_

#include <string>
#include "common_video/libyuv/include/webrtc_libyuv.h"

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
    VideoSource(std::string fileName, VideoSize size, int frameRate = 30,
        webrtc::VideoType type = webrtc::kI420);
    VideoSource(std::string fileName, int width, int height, int frameRate = 30,
                webrtc::VideoType type = webrtc::kI420);

    std::string GetFileName() const { return _fileName; }
    int GetWidth() const { return _width; }
    int GetHeight() const { return _height; }
    webrtc::VideoType GetType() const { return _type; }
    int GetFrameRate() const { return _frameRate; }

    
    std::string GetFilePath() const;

    
    std::string GetName() const;

    VideoSize GetSize() const;
    static VideoSize GetSize(WebRtc_UWord16 width, WebRtc_UWord16 height);
    unsigned int GetFrameLength() const;

    
    static const char* GetSizeString(VideoSize size);
    const char* GetMySizeString() const;

    
    
    
    void Convert(const VideoSource& target, bool force = false) const;
    static bool FileExists(const char* fileName);
private:
    static int GetWidthHeight( VideoSize size, int& width, int& height);
    std::string _fileName;
    int _width;
    int _height;
    webrtc::VideoType _type;
    int _frameRate;
};

class FrameDropper
{
public:
    FrameDropper();
    bool DropFrame();
    unsigned int DropsBetweenRenders();
    void SetFrameRate(double frameRate, double maxFrameRate);

private:
    unsigned int _dropsBetweenRenders;
    unsigned int _frameCounter;
};


#endif 

