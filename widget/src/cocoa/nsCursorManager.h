


































#ifndef nsCursorManager_h_
#define nsCursorManager_h_

#import <Foundation/Foundation.h>
#include "nsIWidget.h"
#include "nsMacCursor.h"







@interface nsCursorManager : NSObject
{
  @private
  NSMutableDictionary *mCursors;
  nsCursor mCurrentCursor;
}







- (void) setCursor: (nsCursor) aCursor;






+ (nsCursorManager *) sharedInstance;





+ (void) dispose;
@end

#endif 
