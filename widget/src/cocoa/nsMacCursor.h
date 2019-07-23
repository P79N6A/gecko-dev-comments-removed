


































#ifndef nsMacCursor_h_
#define nsMacCursor_h_

#import <Cocoa/Cocoa.h>
#import "nsIWidget.h"







@interface nsMacCursor : NSObject
{
  @private
  NSTimer *mTimer;
  @protected
  nsCursor mType;
  int mFrameCounter;    
}








+ (nsMacCursor *) cursorWithCursor: (NSCursor *) aCursor type: (nsCursor) aType;












+ (nsMacCursor *) cursorWithImageNamed: (NSString *) aCursorImage hotSpot: (NSPoint) aPoint type: (nsCursor) aType;










+ (nsMacCursor *) cursorWithFrames: (NSArray *) aCursorFrames type: (nsCursor) aType;








+ (NSCursor *) cocoaCursorWithImageNamed: (NSString *) imageName hotSpot: (NSPoint) aPoint;







- (BOOL) isSet;





- (void) set;





- (void) unset;






- (BOOL) isAnimated;








- (nsCursor) type;
@end

#endif 
