














#ifndef WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_MAC_QTKIT_VIDEO_CAPTURE_QTKIT_OBJC_H_
#define WEBRTC_MODULES_VIDEO_CAPTURE_MAIN_SOURCE_MAC_QTKIT_VIDEO_CAPTURE_QTKIT_OBJC_H_

#import <Foundation/Foundation.h>
#import <QTKit/QTKit.h>
#import <AppKit/AppKit.h>
#import <CoreData/CoreData.h>
#import <CoreFoundation/CoreFoundation.h>
#import <CoreVideo/CoreVideo.h>

#import "video_capture_recursive_lock.h"

#include "video_capture_qtkit.h"

@interface VideoCaptureMacQTKitObjC : NSObject{
    
    bool                                    _capturing;
    int                                    _counter;
    int                                    _frameRate;
    int                                    _frameWidth;
    int                                    _frameHeight;
    int                                    _framesDelivered;
    int                                    _framesRendered;
    bool                                _OSSupported;
    bool                                _captureInitialized;
    
    
    webrtc::videocapturemodule::VideoCaptureMacQTKit* _owner;
    VideoCaptureRecursiveLock*            _rLock;
    
    
    QTCaptureSession*                    _captureSession;
    QTCaptureDeviceInput*                _captureVideoDeviceInput;
    QTCaptureDecompressedVideoOutput*    _captureDecompressedVideoOutput;
    NSArray*                            _captureDevices;
    int                                    _captureDeviceCount;
    int                                    _captureDeviceIndex;
    NSString*                            _captureDeviceName;
    char                                _captureDeviceNameUTF8[1024];
    char                                _captureDeviceNameUniqueID[1024];
    char                                _captureDeviceNameProductID[1024];
    NSString*                            _key;
    NSNumber*                            _val;
    NSDictionary*                        _videoSettings;
    NSString*                            _captureQuality;

}






- (NSNumber*)getCaptureDevices;
- (NSNumber*)initializeVideoCapture;
- (NSNumber*)initializeVariables;
- (void)checkOSSupported;









- (NSNumber*)registerOwner:(webrtc::videocapturemodule::VideoCaptureMacQTKit*)owner;
- (NSNumber*)setCaptureDeviceById:(char*)uniqueId;
- (NSNumber*)setCaptureHeight:(int)height AndWidth:(int)width AndFrameRate:(int)frameRate;
- (NSNumber*)startCapture;
- (NSNumber*)stopCapture;

@end

#endif  
