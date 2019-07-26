



#ifndef nsCursorManager_h_
#define nsCursorManager_h_

#import <Foundation/Foundation.h>
#include "nsIWidget.h"
#include "nsMacCursor.h"







@interface nsCursorManager : NSObject
{
  @private
  NSMutableDictionary *mCursors;
  nsMacCursor *mCurrentMacCursor;
}







- (nsresult) setCursor: (nsCursor) aCursor;










- (nsresult) setCursorWithImage: (imgIContainer*) aCursorImage hotSpotX: (uint32_t) aHotspotX hotSpotY: (uint32_t) aHotspotY scaleFactor: (CGFloat) scaleFactor;







+ (nsCursorManager *) sharedInstance;





+ (void) dispose;
@end

@interface NSCursor (Undocumented)


+ (NSCursor*)busyButClickableCursor;
@end

#endif 
