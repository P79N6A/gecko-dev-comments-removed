


































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









- (nsresult) setCursorWithImage: (imgIContainer*) aCursorImage hotSpotX: (PRUint32) aHotspotX hotSpotY: (PRUint32) aHotspotY;







+ (nsCursorManager *) sharedInstance;





+ (void) dispose;
@end

#endif 
