



#ifndef BASE_CHROME_APPLICATION_MAC_H_
#define BASE_CHROME_APPLICATION_MAC_H_

#import <AppKit/AppKit.h>

#include "base/basictypes.h"
#include "base/scoped_nsobject.h"


@protocol CrApplicationEventHookProtocol
- (void)hookForEvent:(NSEvent*)theEvent;
@end


@interface CrApplication : NSApplication {
 @private
  BOOL handlingSendEvent_;
 
  scoped_nsobject<NSMutableArray> eventHooks_;
}
@property(readonly,
          getter=isHandlingSendEvent,
          nonatomic) BOOL handlingSendEvent;









- (void)addEventHook:(id<CrApplicationEventHookProtocol>)hook;
- (void)removeEventHook:(id<CrApplicationEventHookProtocol>)hook;

+ (NSApplication*)sharedApplication;
@end

namespace chrome_application_mac {



class ScopedSendingEvent {
 public:
  ScopedSendingEvent();
  ~ScopedSendingEvent();

 private:
  CrApplication* app_;
  BOOL handling_;
  DISALLOW_COPY_AND_ASSIGN(ScopedSendingEvent);
};

}  

#endif  
