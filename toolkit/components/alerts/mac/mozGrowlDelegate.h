



#import "GrowlApplicationBridge.h"

#include "nscore.h"
#include "nsStringGlue.h"

#import <Cocoa/Cocoa.h>

#define GROWL_STRING_BUNDLE_LOCATION \
  "chrome://alerts/locale/notificationNames.properties"

#define OBSERVER_KEY      @"ALERT_OBSERVER"
#define COOKIE_KEY        @"ALERT_COOKIE"

class nsIObserver;
class nsIDOMWindow;

@interface mozGrowlDelegate : NSObject <GrowlApplicationBridgeDelegate>
{
@private
  uint32_t mKey;
  NSMutableDictionary *mDict;
  NSMutableArray *mNames;
  NSMutableArray *mEnabled;
}











+ (void) notifyWithName:(const nsAString&)aName
                  title:(const nsAString&)aTitle
            description:(const nsAString&)aText
               iconData:(NSData*)aImage
                    key:(uint32_t)aKey
                 cookie:(const nsAString&)aCookie;






- (void) addNotificationNames:(NSArray*)aNames;






- (void) addEnabledNotifications:(NSArray*)aEnabled;







- (uint32_t) addObserver:(nsIObserver*)aObserver;




- (NSString*) applicationNameForGrowl;





- (NSDictionary *) registrationDictionaryForGrowl;






- (void) growlNotificationTimedOut:(id)clickContext;






- (void) growlNotificationWasClicked:(id)clickContext;






- (void) forgetObserversForWindow:(nsIDOMWindow*)window;




- (void) forgetObservers;

@end
