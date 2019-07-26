















#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_MAC_QTKIT_VIDEO_CAPTURE_QTKIT_UTILITY_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_MAC_QTKIT_VIDEO_CAPTURE_QTKIT_UTILITY_H_

#define MAX_NAME_LENGTH                1024

#define QTKIT_MIN_WIDTH                0
#define QTKIT_MAX_WIDTH                2560
#define QTKIT_DEFAULT_WIDTH            352

#define QTKIT_MIN_HEIGHT            0
#define QTKIT_MAX_HEIGHT            1440
#define QTKIT_DEFAULT_HEIGHT        288

#define QTKIT_MIN_FRAME_RATE        1
#define QTKIT_MAX_FRAME_RATE        60
#define QTKIT_DEFAULT_FRAME_RATE    30

#define RELEASE_AND_CLEAR(p)        if (p) { (p) -> Release () ; (p) = NULL ; }

#endif  
