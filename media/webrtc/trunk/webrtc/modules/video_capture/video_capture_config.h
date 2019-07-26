









#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_VIDEO_CAPTURE_CONFIG_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_VIDEO_CAPTURE_CONFIG_H_

namespace webrtc
{
namespace videocapturemodule
{
enum {kDefaultWidth = 640};  
enum {kDefaultHeight = 480}; 
enum {kDefaultFrameRate = 30}; 

enum {kMaxFrameRate =60}; 

enum {kDefaultCaptureDelay = 120}; 
enum {kMaxCaptureDelay = 270}; 

enum {kProcessInterval = 300}; 
enum {kFrameRateCallbackInterval = 1000}; 
enum {kFrameRateCountHistorySize = 90};
enum {kFrameRateHistoryWindowMs = 2000};
}  
}  

#endif 
