


































#ifndef nsMacCursor_h_
#define nsMacCursor_h_

#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>







@interface nsMacCursor : NSObject
{
  @private
  NSTimer *mTimer;
  @protected
  int mFrameCounter;    
}







+ (nsMacCursor *) cursorWithThemeCursor: (ThemeCursor) aCursor;







+ (nsMacCursor *) cursorWithCursor: (NSCursor *) aCursor;











+ (nsMacCursor *) cursorWithImageNamed: (NSString *) aCursorImage hotSpot: (NSPoint) aPoint;









+ (nsMacCursor *) cursorWithFrames: (NSArray *) aCursorFrames;












+ (nsMacCursor *) cursorWithResources: (int) aFirstFrame lastFrame: (int) aLastFrame;





- (void) set;





- (void) unset;






- (BOOL) isAnimated;

@end

#endif 
