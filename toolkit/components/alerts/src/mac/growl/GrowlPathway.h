








#import <Foundation/Foundation.h>

@protocol GrowlNotificationProtocol
- (oneway void) registerApplicationWithDictionary:(bycopy NSDictionary *)dict;
- (oneway void) postNotificationWithDictionary:(bycopy NSDictionary *)notification;
- (bycopy NSString *) growlVersion;
@end

@interface GrowlPathway : NSObject <GrowlNotificationProtocol> {
}

- (void) registerApplicationWithDictionary:(NSDictionary *)dict;
- (void) postNotificationWithDictionary:(NSDictionary *)dict;

@end
