





#import <Foundation/Foundation.h>

@interface SkEventNotifier : NSObject
- (void)receiveSkEvent:(NSNotification*)notification;
+ (void)postTimedSkEvent:(NSTimeInterval)ti;
+ (void)timerFireMethod:(NSTimer*)theTimer;
@end
