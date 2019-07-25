














#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_MAC_QTKIT_VIDEO_CAPTURE_RECURSIVE_LOCK_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_MAC_QTKIT_VIDEO_CAPTURE_RECURSIVE_LOCK_H_

#import <Foundation/Foundation.h>

@interface VideoCaptureRecursiveLock : NSRecursiveLock <NSLocking> {
    BOOL _locked;
}

@property BOOL locked;

- (void)lock;
- (void)unlock;

@end

#endif 
