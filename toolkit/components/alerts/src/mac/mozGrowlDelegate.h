



































#import "GrowlApplicationBridge.h"
#include "nsIObserver.h"

#import <Cocoa/Cocoa.h>

#define GROWL_STRING_BUNDLE_LOCATION \
  "chrome://alerts/locale/notificationNames.properties"

#define OBSERVER_KEY      @"ALERT_OBSERVER"
#define COOKIE_KEY        @"ALERT_COOKIE"

@interface mozGrowlDelegate : NSObject <GrowlApplicationBridgeDelegate>
{
@private
  PRUint32 mKey;
  NSMutableDictionary *mDict;
  NSMutableArray *mNames;
  NSMutableArray *mEnabled;
}











+ (void) notifyWithName:(const nsAString&)aName
                  title:(const nsAString&)aTitle
            description:(const nsAString&)aText
               iconData:(NSData*)aImage
                    key:(PRUint32)aKey
                 cookie:(const nsAString&)aCookie;






- (void) addNotificationNames:(NSArray*)aNames;






- (void) addEnabledNotifications:(NSArray*)aEnabled;







- (PRUint32) addObserver:(nsIObserver*)aObserver;




- (NSString*) applicationNameForGrowl;





- (NSDictionary *) registrationDictionaryForGrowl;






- (void) growlNotificationTimedOut:(id)clickContext;






- (void) growlNotificationWasClicked:(id)clickContext;

@end
