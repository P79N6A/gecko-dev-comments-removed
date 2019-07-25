



#import <Cocoa/Cocoa.h>

#include "nscore.h"
#include "nsStringGlue.h"
#include "nsNotificationCenterCompat.h"

#define OBSERVER_KEY @"ALERT_OBSERVER"
#define COOKIE_KEY   @"ALERT_COOKIE"

class nsIObserver;
class nsIDOMWindow;

@interface mozNotificationCenterDelegate : NSObject <NSUserNotificationCenterDelegate>
{
@private
  PRUint32 mKey;
  NSMutableDictionary *mObserverDict;
  id<FakeNSUserNotificationCenter> mCenter;
}









- (void)notifyWithTitle:(const nsAString&)aTitle
            description:(const nsAString&)aText
                    key:(PRUint32)aKey
                 cookie:(const nsAString&)aCookie;







- (PRUint32)addObserver:(nsIObserver*)aObserver;






- (void)forgetObserversForWindow:(nsIDOMWindow*)window;




- (void)forgetObservers;

@end
