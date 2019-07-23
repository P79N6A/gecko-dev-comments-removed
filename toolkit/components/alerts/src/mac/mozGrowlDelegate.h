



































#import "GrowlApplicationBridge.h"
#include "nsIObserver.h"

#import <Cocoa/Cocoa.h>


#define NOTIFICATION_NAME @"Application Notice"
#define OBSERVER_KEY      @"ALERT_OBSERVER"
#define COOKIE_KEY        @"ALERT_COOKIE"

@interface mozGrowlDelegate : NSObject <GrowlApplicationBridgeDelegate>
{
@private
  PRUint32 mKey;
  NSMutableDictionary* mDict;
}










+ (void) notifyWithTitle:(const nsAString&)aTitle
             description:(const nsAString&)aText
                iconData:(NSData*)aImage
                     key:(PRUint32)aKey
                  cookie:(const nsAString&)aCookie;







- (PRUint32) addObserver:(nsIObserver*)aObserver;




- (NSString*) applicationNameForGrowl;





- (NSDictionary *) registrationDictionaryForGrowl;






- (void) growlNotificationTimedOut:(id)clickContext;






- (void) growlNotificationWasClicked:(id)clickContext;

@end
