




































#import <Foundation/Foundation.h>
#import "nscore.h"

class nsAString;

typedef enum
{
  kTruncateAtStart,
  kTruncateAtMiddle,
  kTruncateAtEnd
} ETruncationType;



@interface NSString (ChimeraStringUtils)

+ (id)ellipsisString;
+ (id)stringWithPRUnichars:(const PRUnichar*)inString;
+ (id)stringWith_nsAString:(const nsAString&)inString;
- (void)assignTo_nsAString:(nsAString&)ioString;

- (NSString *)stringByRemovingCharactersInSet:(NSCharacterSet*)characterSet;
- (NSString *)stringByReplacingCharactersInSet:(NSCharacterSet*)characterSet withString:(NSString*)string;
- (NSString *)stringByTruncatingTo:(unsigned int)maxCharacters at:(ETruncationType)truncationType;



- (PRUnichar*)createNewUnicodeBuffer;
@end

@interface NSMutableString (ChimeraMutableStringUtils)

- (void)truncateTo:(unsigned)maxCharacters at:(ETruncationType)truncationType;
- (void)truncateToWidth:(float)maxWidth at:(ETruncationType)truncationType withAttributes:(NSDictionary *)attributes;

@end
