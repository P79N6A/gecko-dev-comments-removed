


































#ifndef nsMacCursor_h_
#define nsMacCursor_h_

#import <Cocoa/Cocoa.h>







@interface nsMacCursor : NSObject
{
  @private
  NSTimer *mTimer;
  @protected
  int mFrameCounter;    
}







+ (nsMacCursor *) cursorWithCursor: (NSCursor *) aCursor;











+ (nsMacCursor *) cursorWithImageNamed: (NSString *) aCursorImage hotSpot: (NSPoint) aPoint;









+ (nsMacCursor *) cursorWithFrames: (NSArray *) aCursorFrames;








+ (NSCursor *) cocoaCursorWithImageNamed: (NSString *) imageName hotSpot: (NSPoint) aPoint;







- (BOOL) isSet;





- (void) set;





- (void) unset;






- (BOOL) isAnimated;

@end

#endif 
