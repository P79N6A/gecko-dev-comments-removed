








#import <Foundation/Foundation.h>

@interface NSURL (GrowlAdditions)


+ (NSURL *) fileURLWithAliasData:(NSData *)aliasData;
- (NSData *) aliasData;


+ (NSURL *) fileURLWithDockDescription:(NSDictionary *)dict;

- (NSDictionary *) dockDescription;

@end
