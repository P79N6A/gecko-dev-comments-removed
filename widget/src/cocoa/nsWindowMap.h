





































#ifndef nsWindowMap_h_
#define nsWindowMap_h_

#import <Cocoa/Cocoa.h>















@interface WindowDataMap : NSObject
{
@private
  NSMutableDictionary*    mWindowMap;   
}

+ (WindowDataMap*)sharedWindowDataMap;

- (id)dataForWindow:(NSWindow*)inWindow;



- (void)setData:(id)inData forWindow:(NSWindow*)inWindow;


- (void)removeDataForWindow:(NSWindow*)inWindow;

@end


@class ChildView;







@interface TopLevelWindowData : NSObject
{
@private
}

- (id)initWithWindow:(NSWindow*)inWindow;
+ (void)activateInWindow:(NSWindow*)aWindow;
+ (void)deactivateInWindow:(NSWindow*)aWindow;
+ (void)activateInWindowViews:(NSWindow*)aWindow;
+ (void)deactivateInWindowViews:(NSWindow*)aWindow;

@end

#endif 
