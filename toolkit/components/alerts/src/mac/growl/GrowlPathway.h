








#import <Foundation/Foundation.h>

@protocol GrowlNotificationProtocol
- (oneway void) registerApplicationWithDictionary:(bycopy NSDictionary *)dict;
- (oneway void) postNotificationWithDictionary:(bycopy NSDictionary *)notification;
- (bycopy NSString *) growlVersion;
@end

@class GrowlApplicationController;

@interface GrowlPathway : NSObject <GrowlNotificationProtocol> {
}

@end
