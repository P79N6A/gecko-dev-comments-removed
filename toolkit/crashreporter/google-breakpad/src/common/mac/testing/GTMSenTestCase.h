




















































#import "GTMDefines.h"

#if (!GTM_IPHONE_SDK) 
#import <SenTestingKit/SenTestingKit.h>
#else
#import <Foundation/Foundation.h>
NSString *STComposeString(NSString *, ...);
#endif







#define STAssertNoErr(a1, description, ...) \
do { \
  @try {\
    OSStatus a1value = (a1); \
    if (a1value != noErr) { \
      NSString *_expression = [NSString stringWithFormat:@"Expected noErr, got %ld for (%s)", a1value, #a1]; \
      if (description) { \
        _expression = [NSString stringWithFormat:@"%@: %@", _expression, STComposeString(description, ##__VA_ARGS__)]; \
      } \
      [self failWithException:[NSException failureInFile:[NSString stringWithUTF8String:__FILE__] \
                                                  atLine:__LINE__ \
                                         withDescription:_expression]]; \
    } \
  }\
  @catch (id anException) {\
    [self failWithException:[NSException failureInRaise:[NSString stringWithFormat:@"(%s) == noErr fails", #a1] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
                                        withDescription:STComposeString(description, ##__VA_ARGS__)]]; \
  }\
} while(0)








#define STAssertErr(a1, a2, description, ...) \
do { \
  @try {\
    OSStatus a1value = (a1); \
    OSStatus a2value = (a2); \
    if (a1value != a2value) { \
      NSString *_expression = [NSString stringWithFormat:@"Expected %s(%ld) but got %ld for (%s)", #a2, a2value, a1value, #a1]; \
      if (description) { \
        _expression = [NSString stringWithFormat:@"%@: %@", _expression, STComposeString(description, ##__VA_ARGS__)]; \
      } \
      [self failWithException:[NSException failureInFile:[NSString stringWithUTF8String:__FILE__] \
                                                  atLine:__LINE__ \
                                         withDescription:_expression]]; \
    } \
  }\
  @catch (id anException) {\
    [self failWithException:[NSException failureInRaise:[NSString stringWithFormat:@"(%s) == (%s) fails", #a1, #a2] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
                                        withDescription:STComposeString(description, ##__VA_ARGS__)]]; \
  }\
} while(0)








#define STAssertNotNULL(a1, description, ...) \
do { \
  @try {\
    const void* a1value = (a1); \
    if (a1value == NULL) { \
      NSString *_expression = [NSString stringWithFormat:@"(%s) != NULL", #a1]; \
      if (description) { \
        _expression = [NSString stringWithFormat:@"%@: %@", _expression, STComposeString(description, ##__VA_ARGS__)]; \
      } \
      [self failWithException:[NSException failureInFile:[NSString stringWithUTF8String:__FILE__] \
                                                  atLine:__LINE__ \
                                         withDescription:_expression]]; \
    } \
  }\
  @catch (id anException) {\
    [self failWithException:[NSException failureInRaise:[NSString stringWithFormat:@"(%s) != NULL fails", #a1] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
                                        withDescription:STComposeString(description, ##__VA_ARGS__)]]; \
  }\
} while(0)







#define STAssertNULL(a1, description, ...) \
do { \
  @try {\
    const void* a1value = (a1); \
    if (a1value != NULL) { \
      NSString *_expression = [NSString stringWithFormat:@"(%s) == NULL", #a1]; \
      if (description) { \
        _expression = [NSString stringWithFormat:@"%@: %@", _expression, STComposeString(description, ##__VA_ARGS__)]; \
      } \
      [self failWithException:[NSException failureInFile:[NSString stringWithUTF8String:__FILE__] \
                                                  atLine:__LINE__ \
                                         withDescription:_expression]]; \
    } \
  }\
  @catch (id anException) {\
    [self failWithException:[NSException failureInRaise:[NSString stringWithFormat:@"(%s) == NULL fails", #a1] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
                                        withDescription:STComposeString(description, ##__VA_ARGS__)]]; \
  }\
} while(0)









#define STAssertNotEquals(a1, a2, description, ...) \
do { \
  @try {\
    if (@encode(__typeof__(a1)) != @encode(__typeof__(a2))) { \
      [self failWithException:[NSException failureInFile:[NSString stringWithUTF8String:__FILE__] \
                                                  atLine:__LINE__ \
                                         withDescription:[[[NSString stringWithFormat:@"Type mismatch (%@/%@) -- ",@encode(__typeof__(a1)),@encode(__typeof__(a2))] stringByAppendingString:STComposeString(description, ##__VA_ARGS__)]]]; \
    } else { \
      __typeof__(a1) a1value = (a1); \
      __typeof__(a2) a2value = (a2); \
      NSValue *a1encoded = [NSValue value:&a1value withObjCType:@encode(__typeof__(a1))]; \
      NSValue *a2encoded = [NSValue value:&a2value withObjCType:@encode(__typeof__(a2))]; \
      if ([a1encoded isEqualToValue:a2encoded]) { \
        NSString *_expression = [NSString stringWithFormat:@"(%s) != (%s)", #a1, #a2]; \
        if (description) { \
          _expression = [NSString stringWithFormat:@"%@: %@", _expression, STComposeString(description, ##__VA_ARGS__)]; \
        } \
        [self failWithException:[NSException failureInFile:[NSString stringWithUTF8String:__FILE__] \
                                                    atLine:__LINE__ \
                                          withDescription:_expression]]; \
      } \
    } \
  } \
  @catch (id anException) {\
    [self failWithException:[NSException failureInRaise:[NSString stringWithFormat:@"(%s) != (%s)", #a1, #a2] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
            withDescription:STComposeString(description, ##__VA_ARGS__)]]; \
  }\
} while(0)








#define STAssertNotEqualObjects(a1, a2, desc, ...) \
do { \
  @try {\
    id a1value = (a1); \
    id a2value = (a2); \
    if ( (@encode(__typeof__(a1value)) == @encode(id)) && \
         (@encode(__typeof__(a2value)) == @encode(id)) && \
         ![(id)a1value isEqual:(id)a2value] ) continue; \
         NSString *_expression = [NSString stringWithFormat:@"%s('%@') != %s('%@')", #a1, [a1 description], #a2, [a2 description]]; \
         if (desc) { \
           _expression = [NSString stringWithFormat:@"%@: %@", _expression, STComposeString(desc, ##__VA_ARGS__)]; \
         } \
         [self failWithException:[NSException failureInFile:[NSString stringWithUTF8String:__FILE__] \
                                                     atLine:__LINE__ \
                                            withDescription:_expression]]; \
  }\
  @catch (id anException) {\
    [self failWithException:[NSException failureInRaise:[NSString stringWithFormat: @"(%s) != (%s)", #a1, #a2] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
                                        withDescription:STComposeString(desc, ##__VA_ARGS__)]]; \
  }\
} while(0)









#define STAssertOperation(a1, a2, op, description, ...) \
do { \
  @try {\
    if (@encode(__typeof__(a1)) != @encode(__typeof__(a2))) { \
      [self failWithException:[NSException failureInFile:[NSString stringWithUTF8String:__FILE__] \
                                                  atLine:__LINE__ \
                                         withDescription:[[[NSString stringWithFormat:@"Type mismatch (%@/%@) -- ",@encode(__typeof__(a1)),@encode(__typeof__(a2))] stringByAppendingString:STComposeString(description, ##__VA_ARGS__)]]]; \
    } else { \
      __typeof__(a1) a1value = (a1); \
      __typeof__(a2) a2value = (a2); \
      if (!(a1value op a2value)) { \
        double a1DoubleValue = a1value; \
        double a2DoubleValue = a2value; \
        NSString *_expression = [NSString stringWithFormat:@"%s (%lg) %s %s (%lg)", #a1, a1DoubleValue, #op, #a2, a2DoubleValue]; \
        if (description) { \
          _expression = [NSString stringWithFormat:@"%@: %@", _expression, STComposeString(description, ##__VA_ARGS__)]; \
        } \
        [self failWithException:[NSException failureInFile:[NSString stringWithUTF8String:__FILE__] \
                                                    atLine:__LINE__ \
                                           withDescription:_expression]]; \
      } \
    } \
  } \
  @catch (id anException) {\
    [self failWithException:[NSException \
             failureInRaise:[NSString stringWithFormat:@"(%s) %s (%s)", #a1, #op, #a2] \
                  exception:anException \
                     inFile:[NSString stringWithUTF8String:__FILE__] \
                     atLine:__LINE__ \
            withDescription:STComposeString(description, ##__VA_ARGS__)]]; \
  }\
} while(0)









#define STAssertGreaterThan(a1, a2, description, ...) \
  STAssertOperation(a1, a2, >, description, ##__VA_ARGS__)









#define STAssertGreaterThanOrEqual(a1, a2, description, ...) \
  STAssertOperation(a1, a2, >=, description, ##__VA_ARGS__)









#define STAssertLessThan(a1, a2, description, ...) \
  STAssertOperation(a1, a2, <, description, ##__VA_ARGS__)









#define STAssertLessThanOrEqual(a1, a2, description, ...) \
  STAssertOperation(a1, a2, <=, description, ##__VA_ARGS__)












#define STAssertEqualStrings(a1, a2, description, ...) \
do { \
  @try {\
    id a1value = (a1); \
    id a2value = (a2); \
    if (a1value == a2value) continue; \
    if ([a1value isKindOfClass:[NSString class]] && \
        [a2value isKindOfClass:[NSString class]] && \
        [a1value compare:a2value options:0] == NSOrderedSame) continue; \
     [self failWithException:[NSException failureInEqualityBetweenObject: a1value \
                                                               andObject: a2value \
                                                                  inFile: [NSString stringWithUTF8String:__FILE__] \
                                                                  atLine: __LINE__ \
                                                         withDescription: STComposeString(description, ##__VA_ARGS__)]]; \
  }\
  @catch (id anException) {\
    [self failWithException:[NSException failureInRaise:[NSString stringWithFormat: @"(%s) == (%s)", #a1, #a2] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
                                        withDescription:STComposeString(description, ##__VA_ARGS__)]]; \
  }\
} while(0)












#define STAssertNotEqualStrings(a1, a2, description, ...) \
do { \
  @try {\
    id a1value = (a1); \
    id a2value = (a2); \
    if ([a1value isKindOfClass:[NSString class]] && \
        [a2value isKindOfClass:[NSString class]] && \
        [a1value compare:a2value options:0] != NSOrderedSame) continue; \
     [self failWithException:[NSException failureInEqualityBetweenObject: a1value \
                                                               andObject: a2value \
                                                                  inFile: [NSString stringWithUTF8String:__FILE__] \
                                                                  atLine: __LINE__ \
                                                         withDescription: STComposeString(description, ##__VA_ARGS__)]]; \
  }\
  @catch (id anException) {\
    [self failWithException:[NSException failureInRaise:[NSString stringWithFormat: @"(%s) != (%s)", #a1, #a2] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
                                        withDescription:STComposeString(description, ##__VA_ARGS__)]]; \
  }\
} while(0)








#define STAssertEqualCStrings(a1, a2, description, ...) \
do { \
  @try {\
    const char* a1value = (a1); \
    const char* a2value = (a2); \
    if (a1value == a2value) continue; \
    if (strcmp(a1value, a2value) == 0) continue; \
    [self failWithException:[NSException failureInEqualityBetweenObject: [NSString stringWithUTF8String:a1value] \
                                                              andObject: [NSString stringWithUTF8String:a2value] \
                                                                 inFile: [NSString stringWithUTF8String:__FILE__] \
                                                                 atLine: __LINE__ \
                                                        withDescription: STComposeString(description, ##__VA_ARGS__)]]; \
  }\
  @catch (id anException) {\
    [self failWithException:[NSException failureInRaise:[NSString stringWithFormat: @"(%s) == (%s)", #a1, #a2] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
                                        withDescription:STComposeString(description, ##__VA_ARGS__)]]; \
  }\
} while(0)








#define STAssertNotEqualCStrings(a1, a2, description, ...) \
do { \
  @try {\
    const char* a1value = (a1); \
    const char* a2value = (a2); \
    if (strcmp(a1value, a2value) != 0) continue; \
    [self failWithException:[NSException failureInEqualityBetweenObject: [NSString stringWithUTF8String:a1value] \
                                                              andObject: [NSString stringWithUTF8String:a2value] \
                                                                 inFile: [NSString stringWithUTF8String:__FILE__] \
                                                                 atLine: __LINE__ \
                                                        withDescription: STComposeString(description, ##__VA_ARGS__)]]; \
  }\
  @catch (id anException) {\
    [self failWithException:[NSException failureInRaise:[NSString stringWithFormat: @"(%s) != (%s)", #a1, #a2] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
                                        withDescription:STComposeString(description, ##__VA_ARGS__)]]; \
  }\
} while(0)

#if GTM_IPHONE_SDK










#define STAssertEqualObjects(a1, a2, description, ...) \
do { \
  @try {\
    id a1value = (a1); \
    id a2value = (a2); \
    if (a1value == a2value) continue; \
    if ( (@encode(__typeof__(a1value)) == @encode(id)) && \
         (@encode(__typeof__(a2value)) == @encode(id)) && \
         [(id)a1value isEqual: (id)a2value] ) continue; \
    [self failWithException:[NSException failureInEqualityBetweenObject: a1value \
                                                              andObject: a2value \
                                                                 inFile: [NSString stringWithUTF8String:__FILE__] \
                                                                 atLine: __LINE__ \
                                                        withDescription: STComposeString(description, ##__VA_ARGS__)]]; \
  }\
  @catch (id anException) {\
    [self failWithException:[NSException failureInRaise:[NSString stringWithFormat: @"(%s) == (%s)", #a1, #a2] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
                                        withDescription:STComposeString(description, ##__VA_ARGS__)]]; \
  }\
} while(0)










#define STAssertEquals(a1, a2, description, ...) \
do { \
  @try {\
    if (@encode(__typeof__(a1)) != @encode(__typeof__(a2))) { \
      [self failWithException:[NSException failureInFile:[NSString stringWithUTF8String:__FILE__] \
                                                                                 atLine:__LINE__ \
			       withDescription:[[NSString stringWithFormat:@"Type mismatch (%@/%@) -- ",@encode(__typeof__(a1)),@encode(__typeof__(a2))] stringByAppendingString:STComposeString(description, ##__VA_ARGS__)]]]; \
    } else { \
      __typeof__(a1) a1value = (a1); \
      __typeof__(a2) a2value = (a2); \
      NSValue *a1encoded = [NSValue value:&a1value withObjCType: @encode(__typeof__(a1))]; \
      NSValue *a2encoded = [NSValue value:&a2value withObjCType: @encode(__typeof__(a2))]; \
      if (![a1encoded isEqualToValue:a2encoded]) { \
        [self failWithException:[NSException failureInEqualityBetweenValue: a1encoded \
                                                                  andValue: a2encoded \
                                                              withAccuracy: nil \
                                                                    inFile: [NSString stringWithUTF8String:__FILE__] \
                                                                    atLine: __LINE__ \
                                                           withDescription: STComposeString(description, ##__VA_ARGS__)]]; \
      } \
    } \
  } \
  @catch (id anException) {\
    [self failWithException:[NSException failureInRaise:[NSString stringWithFormat: @"(%s) == (%s)", #a1, #a2] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
                                        withDescription:STComposeString(description, ##__VA_ARGS__)]]; \
  }\
} while(0)

#define STAbsoluteDifference(left,right) (MAX(left,right)-MIN(left,right))














#define STAssertEqualsWithAccuracy(a1, a2, accuracy, description, ...) \
do { \
  @try {\
    if (@encode(__typeof__(a1)) != @encode(__typeof__(a2))) { \
      [self failWithException:[NSException failureInFile:[NSString stringWithUTF8String:__FILE__] \
                                                                                 atLine:__LINE__ \
                                                                        withDescription:[[[NSString stringWithFormat:@"Type mismatch (%@/%@) -- ",@encode(__typeof__(a1)),@encode(__typeof__(a2))] stringByAppendingString:STComposeString(description, ##__VA_ARGS__)]]]; \
    } else { \
      __typeof__(a1) a1value = (a1); \
      __typeof__(a2) a2value = (a2); \
      __typeof__(accuracy) accuracyvalue = (accuracy); \
      if (STAbsoluteDifference(a1value, a2value) > accuracyvalue) { \
              NSValue *a1encoded = [NSValue value:&a1value withObjCType:@encode(__typeof__(a1))]; \
              NSValue *a2encoded = [NSValue value:&a2value withObjCType:@encode(__typeof__(a2))]; \
              NSValue *accuracyencoded = [NSValue value:&accuracyvalue withObjCType:@encode(__typeof__(accuracy))]; \
              [self failWithException:[NSException failureInEqualityBetweenValue: a1encoded \
                                                                        andValue: a2encoded \
                                                                    withAccuracy: accuracyencoded \
                                                                          inFile: [NSString stringWithUTF8String:__FILE__] \
                                                                          atLine: __LINE__ \
                                                                 withDescription: STComposeString(description, ##__VA_ARGS__)]]; \
      } \
    } \
  } \
  @catch (id anException) {\
    [self failWithException:[NSException failureInRaise:[NSString stringWithFormat: @"(%s) == (%s)", #a1, #a2] \
                                                                         exception:anException \
                                                                            inFile:[NSString stringWithUTF8String:__FILE__] \
                                                                            atLine:__LINE__ \
                                                                   withDescription:STComposeString(description, ##__VA_ARGS__)]]; \
  }\
} while(0)








#define STFail(description, ...) \
[self failWithException:[NSException failureInFile: [NSString stringWithUTF8String:__FILE__] \
                                            atLine: __LINE__ \
                                   withDescription: STComposeString(description, ##__VA_ARGS__)]]









#define STAssertNil(a1, description, ...) \
do { \
  @try {\
    id a1value = (a1); \
    if (a1value != nil) { \
      NSString *_a1 = [NSString stringWithUTF8String: #a1]; \
      NSString *_expression = [NSString stringWithFormat:@"((%@) == nil)", _a1]; \
      [self failWithException:[NSException failureInCondition: _expression \
                                                       isTrue: NO \
                                                       inFile: [NSString stringWithUTF8String:__FILE__] \
                                                       atLine: __LINE__ \
                                              withDescription: STComposeString(description, ##__VA_ARGS__)]]; \
    } \
  }\
  @catch (id anException) {\
    [self failWithException:[NSException failureInRaise:[NSString stringWithFormat: @"(%s) == nil fails", #a1] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
                                        withDescription:STComposeString(description, ##__VA_ARGS__)]]; \
  }\
} while(0)








#define STAssertNotNil(a1, description, ...) \
do { \
  @try {\
    id a1value = (a1); \
    if (a1value == nil) { \
      NSString *_a1 = [NSString stringWithUTF8String: #a1]; \
      NSString *_expression = [NSString stringWithFormat:@"((%@) != nil)", _a1]; \
      [self failWithException:[NSException failureInCondition: _expression \
                                                       isTrue: NO \
                                                       inFile: [NSString stringWithUTF8String:__FILE__] \
                                                       atLine: __LINE__ \
                                              withDescription: STComposeString(description, ##__VA_ARGS__)]]; \
    } \
  }\
  @catch (id anException) {\
    [self failWithException:[NSException failureInRaise:[NSString stringWithFormat: @"(%s) != nil fails", #a1] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
                                        withDescription:STComposeString(description, ##__VA_ARGS__)]]; \
  }\
} while(0)








#define STAssertTrue(expr, description, ...) \
do { \
  BOOL _evaluatedExpression = (expr);\
  if (!_evaluatedExpression) {\
    NSString *_expression = [NSString stringWithUTF8String: #expr];\
    [self failWithException:[NSException failureInCondition: _expression \
                                                     isTrue: NO \
                                                     inFile: [NSString stringWithUTF8String:__FILE__] \
                                                     atLine: __LINE__ \
                                            withDescription: STComposeString(description, ##__VA_ARGS__)]]; \
  } \
} while (0)









#define STAssertTrueNoThrow(expr, description, ...) \
do { \
  @try {\
    BOOL _evaluatedExpression = (expr);\
    if (!_evaluatedExpression) {\
      NSString *_expression = [NSString stringWithUTF8String: #expr];\
      [self failWithException:[NSException failureInCondition: _expression \
                                                       isTrue: NO \
                                                       inFile: [NSString stringWithUTF8String:__FILE__] \
                                                       atLine: __LINE__ \
                                              withDescription: STComposeString(description, ##__VA_ARGS__)]]; \
    } \
  } \
  @catch (id anException) {\
    [self failWithException:[NSException failureInRaise:[NSString stringWithFormat: @"(%s) ", #expr] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
                                        withDescription:STComposeString(description, ##__VA_ARGS__)]]; \
  }\
} while (0)








#define STAssertFalse(expr, description, ...) \
do { \
  BOOL _evaluatedExpression = (expr);\
  if (_evaluatedExpression) {\
    NSString *_expression = [NSString stringWithUTF8String: #expr];\
    [self failWithException:[NSException failureInCondition: _expression \
                                                     isTrue: YES \
                                                     inFile: [NSString stringWithUTF8String:__FILE__] \
                                                     atLine: __LINE__ \
                                            withDescription: STComposeString(description, ##__VA_ARGS__)]]; \
  } \
} while (0)









#define STAssertFalseNoThrow(expr, description, ...) \
do { \
  @try {\
    BOOL _evaluatedExpression = (expr);\
    if (_evaluatedExpression) {\
      NSString *_expression = [NSString stringWithUTF8String: #expr];\
      [self failWithException:[NSException failureInCondition: _expression \
                                                       isTrue: YES \
                                                       inFile: [NSString stringWithUTF8String:__FILE__] \
                                                       atLine: __LINE__ \
                                              withDescription: STComposeString(description, ##__VA_ARGS__)]]; \
    } \
  } \
  @catch (id anException) {\
    [self failWithException:[NSException failureInRaise:[NSString stringWithFormat: @"!(%s) ", #expr] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
                                        withDescription:STComposeString(description, ##__VA_ARGS__)]]; \
  }\
} while (0)








#define STAssertThrows(expr, description, ...) \
do { \
  @try { \
    (expr);\
  } \
  @catch (id anException) { \
    continue; \
  }\
  [self failWithException:[NSException failureInRaise: [NSString stringWithUTF8String:#expr] \
                                            exception: nil \
                                               inFile: [NSString stringWithUTF8String:__FILE__] \
                                               atLine: __LINE__ \
                                      withDescription: STComposeString(description, ##__VA_ARGS__)]]; \
} while (0)










#define STAssertThrowsSpecific(expr, specificException, description, ...) \
do { \
  @try { \
    (expr);\
  } \
  @catch (specificException *anException) { \
    continue; \
  }\
  @catch (id anException) {\
    NSString *_descrip = STComposeString(@"(Expected exception: %@) %@", NSStringFromClass([specificException class]), description);\
    [self failWithException:[NSException failureInRaise: [NSString stringWithUTF8String:#expr] \
                                              exception: anException \
                                                 inFile: [NSString stringWithUTF8String:__FILE__] \
                                                 atLine: __LINE__ \
                                        withDescription: STComposeString(_descrip, ##__VA_ARGS__)]]; \
                                            continue; \
  }\
  NSString *_descrip = STComposeString(@"(Expected exception: %@) %@", NSStringFromClass([specificException class]), description);\
  [self failWithException:[NSException failureInRaise: [NSString stringWithUTF8String:#expr] \
                                            exception: nil \
                                               inFile: [NSString stringWithUTF8String:__FILE__] \
                                               atLine: __LINE__ \
                                      withDescription: STComposeString(_descrip, ##__VA_ARGS__)]]; \
} while (0)














#define STAssertThrowsSpecificNamed(expr, specificException, aName, description, ...) \
do { \
  @try { \
    (expr);\
  } \
  @catch (specificException *anException) { \
    if ([aName isEqualToString: [anException name]]) continue; \
    NSString *_descrip = STComposeString(@"(Expected exception: %@ (name: %@)) %@", NSStringFromClass([specificException class]), aName, description);\
    [self failWithException: \
      [NSException failureInRaise: [NSString stringWithUTF8String:#expr] \
                        exception: anException \
                           inFile: [NSString stringWithUTF8String:__FILE__] \
                           atLine: __LINE__ \
                  withDescription: STComposeString(_descrip, ##__VA_ARGS__)]]; \
    continue; \
  }\
  @catch (id anException) {\
    NSString *_descrip = STComposeString(@"(Expected exception: %@) %@", NSStringFromClass([specificException class]), description);\
    [self failWithException: \
      [NSException failureInRaise: [NSString stringWithUTF8String:#expr] \
                        exception: anException \
                           inFile: [NSString stringWithUTF8String:__FILE__] \
                           atLine: __LINE__ \
                  withDescription: STComposeString(_descrip, ##__VA_ARGS__)]]; \
    continue; \
  }\
  NSString *_descrip = STComposeString(@"(Expected exception: %@) %@", NSStringFromClass([specificException class]), description);\
  [self failWithException: \
    [NSException failureInRaise: [NSString stringWithUTF8String:#expr] \
                      exception: nil \
                         inFile: [NSString stringWithUTF8String:__FILE__] \
                         atLine: __LINE__ \
                withDescription: STComposeString(_descrip, ##__VA_ARGS__)]]; \
} while (0)








#define STAssertNoThrow(expr, description, ...) \
do { \
  @try { \
    (expr);\
  } \
  @catch (id anException) { \
    [self failWithException:[NSException failureInRaise: [NSString stringWithUTF8String:#expr] \
                                              exception: anException \
                                                 inFile: [NSString stringWithUTF8String:__FILE__] \
                                                 atLine: __LINE__ \
                                        withDescription: STComposeString(description, ##__VA_ARGS__)]]; \
  }\
} while (0)










#define STAssertNoThrowSpecific(expr, specificException, description, ...) \
do { \
  @try { \
    (expr);\
  } \
  @catch (specificException *anException) { \
    [self failWithException:[NSException failureInRaise: [NSString stringWithUTF8String:#expr] \
                                              exception: anException \
                                                 inFile: [NSString stringWithUTF8String:__FILE__] \
                                                 atLine: __LINE__ \
                                        withDescription: STComposeString(description, ##__VA_ARGS__)]]; \
  }\
  @catch (id anythingElse) {\
    ; \
  }\
} while (0)














#define STAssertNoThrowSpecificNamed(expr, specificException, aName, description, ...) \
do { \
  @try { \
    (expr);\
  } \
  @catch (specificException *anException) { \
    if ([aName isEqualToString: [anException name]]) { \
      NSString *_descrip = STComposeString(@"(Expected exception: %@ (name: %@)) %@", NSStringFromClass([specificException class]), aName, description);\
      [self failWithException: \
        [NSException failureInRaise: [NSString stringWithUTF8String:#expr] \
                          exception: anException \
                             inFile: [NSString stringWithUTF8String:__FILE__] \
                             atLine: __LINE__ \
                    withDescription: STComposeString(_descrip, ##__VA_ARGS__)]]; \
    } \
    continue; \
  }\
  @catch (id anythingElse) {\
    ; \
  }\
} while (0)



@interface NSException (GTMSenTestAdditions)
+ (NSException *)failureInFile:(NSString *)filename 
                        atLine:(int)lineNumber 
               withDescription:(NSString *)formatString, ...;
+ (NSException *)failureInCondition:(NSString *)condition 
                             isTrue:(BOOL)isTrue 
                             inFile:(NSString *)filename 
                             atLine:(int)lineNumber 
                    withDescription:(NSString *)formatString, ...;
+ (NSException *)failureInEqualityBetweenObject:(id)left
                                      andObject:(id)right
                                         inFile:(NSString *)filename
                                         atLine:(int)lineNumber
                                withDescription:(NSString *)formatString, ...;
+ (NSException *)failureInEqualityBetweenValue:(NSValue *)left 
                                      andValue:(NSValue *)right 
                                  withAccuracy:(NSValue *)accuracy 
                                        inFile:(NSString *)filename 
                                        atLine:(int) ineNumber
                               withDescription:(NSString *)formatString, ...;
+ (NSException *)failureInRaise:(NSString *)expression 
                         inFile:(NSString *)filename 
                         atLine:(int)lineNumber
                withDescription:(NSString *)formatString, ...;
+ (NSException *)failureInRaise:(NSString *)expression 
                      exception:(NSException *)exception 
                         inFile:(NSString *)filename 
                         atLine:(int)lineNumber 
                withDescription:(NSString *)formatString, ...;
@end



@interface SenTestCase : NSObject {
  SEL currentSelector_;
}

- (void)setUp;
- (void)invokeTest;
- (void)tearDown;
- (void)performTest:(SEL)sel;
- (void)failWithException:(NSException*)exception;
@end

GTM_EXTERN NSString *const SenTestFailureException;

GTM_EXTERN NSString *const SenTestFilenameKey;
GTM_EXTERN NSString *const SenTestLineNumberKey;

#endif 




@interface GTMTestCase : SenTestCase
@end
