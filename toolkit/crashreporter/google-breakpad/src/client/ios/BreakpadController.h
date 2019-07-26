




























#ifndef CLIENT_IOS_HANDLER_IOS_BREAKPAD_CONTROLLER_H_
#define CLIENT_IOS_HANDLER_IOS_BREAKPAD_CONTROLLER_H_

#import <Foundation/Foundation.h>

#import "client/ios/Breakpad.h"





@interface BreakpadController : NSObject {
 @private
  
  dispatch_queue_t queue_;

  
  
  BreakpadRef breakpadRef_;

  
  
  
  NSMutableDictionary* configuration_;

  
  BOOL enableUploads_;

  
  
  BOOL started_;

  
  
  int uploadIntervalInSeconds_;
}


+ (BreakpadController*)sharedInstance;



- (void)updateConfiguration:(NSDictionary*)configuration;



- (void)setUploadingURL:(NSString*)url;




- (void)setUploadInterval:(int)intervalInSeconds;



- (void)addUploadParameter:(NSString*)value forKey:(NSString*)key;



- (void)removeUploadParameterForKey:(NSString*)key;




- (void)withBreakpadRef:(void(^)(BreakpadRef))callback;




- (void)start:(BOOL)onCurrentThread;


- (void)stop;



- (void)setUploadingEnabled:(BOOL)enabled;


- (void)hasReportToUpload:(void(^)(BOOL))callback;

@end

#endif  
