





































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








@interface TopLevelWindowData : NSObject
{
@private

}

- (id)initWithWindow:(NSWindow*)inWindow;

@end

#endif 
