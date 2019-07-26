




















































#import "GTMDefines.h"

#if (!GTM_IPHONE_SDK) || (GTM_IPHONE_USE_SENTEST)
#import <SenTestingKit/SenTestingKit.h>
#else
#import <Foundation/Foundation.h>
#ifdef __cplusplus
extern "C" {
#endif
  
#if defined __clang__



NSString *STComposeString(NSString *, ...) NS_FORMAT_FUNCTION(1, 2);
#else
NSString *STComposeString(NSString *, ...);
#endif  
  
#ifdef __cplusplus
}
#endif

#endif







#define STAssertNoErr(a1, description, ...) \
do { \
  @try { \
    OSStatus a1value = (a1); \
    if (a1value != noErr) { \
      NSString *_expression = [NSString stringWithFormat:@"Expected noErr, got %ld for (%s)", (long)a1value, #a1]; \
      [self failWithException:([NSException failureInCondition:_expression \
                       isTrue:NO \
                       inFile:[NSString stringWithUTF8String:__FILE__] \
                       atLine:__LINE__ \
              withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)])]; \
    } \
  } \
  @catch (id anException) { \
    [self failWithException:[NSException failureInRaise:[NSString stringWithFormat:@"(%s) == noErr fails", #a1] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
                                        withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]; \
  } \
} while(0)








#define STAssertErr(a1, a2, description, ...) \
do { \
  @try { \
    OSStatus a1value = (a1); \
    OSStatus a2value = (a2); \
    if (a1value != a2value) { \
      NSString *_expression = [NSString stringWithFormat:@"Expected %s(%ld) but got %ld for (%s)", #a2, (long)a2value, (long)a1value, #a1]; \
      [self failWithException:([NSException failureInCondition:_expression \
                       isTrue:NO \
                       inFile:[NSString stringWithUTF8String:__FILE__] \
                       atLine:__LINE__ \
              withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)])]; \
    } \
  } \
  @catch (id anException) { \
    [self failWithException:[NSException failureInRaise:[NSString stringWithFormat:@"(%s) == (%s) fails", #a1, #a2] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
                                        withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]; \
  } \
} while(0)








#define STAssertNotNULL(a1, description, ...) \
do { \
  @try { \
    __typeof__(a1) a1value = (a1); \
    if (a1value == (__typeof__(a1))NULL) { \
      NSString *_expression = [NSString stringWithFormat:@"((%s) != NULL)", #a1]; \
      [self failWithException:([NSException failureInCondition:_expression \
                       isTrue:NO \
                       inFile:[NSString stringWithUTF8String:__FILE__] \
                       atLine:__LINE__ \
              withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)])]; \
    } \
  } \
  @catch (id anException) { \
    [self failWithException:[NSException failureInRaise:[NSString stringWithFormat:@"(%s) != NULL fails", #a1] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
                                        withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]; \
  } \
} while(0)







#define STAssertNULL(a1, description, ...) \
do { \
  @try { \
    __typeof__(a1) a1value = (a1); \
    if (a1value != (__typeof__(a1))NULL) { \
      NSString *_expression = [NSString stringWithFormat:@"((%s) == NULL)", #a1]; \
      [self failWithException:([NSException failureInCondition:_expression \
                                                        isTrue:NO \
                                                        inFile:[NSString stringWithUTF8String:__FILE__] \
                                                        atLine:__LINE__ \
                                               withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)])]; \
    } \
  } \
  @catch (id anException) { \
    [self failWithException:[NSException failureInRaise:[NSString stringWithFormat:@"(%s) == NULL fails", #a1] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
                                        withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]; \
  } \
} while(0)









#define STAssertNotEquals(a1, a2, description, ...) \
do { \
  @try { \
    if (strcmp(@encode(__typeof__(a1)), @encode(__typeof__(a2)))) { \
      [self failWithException:[NSException failureInFile:[NSString stringWithUTF8String:__FILE__] \
                                                  atLine:__LINE__ \
                                         withDescription:@"Type mismatch -- %@", STComposeString(description, ##__VA_ARGS__)]]; \
    } else { \
      __typeof__(a1) a1value = (a1); \
      __typeof__(a2) a2value = (a2); \
      NSValue *a1encoded = [NSValue value:&a1value withObjCType:@encode(__typeof__(a1))]; \
      NSValue *a2encoded = [NSValue value:&a2value withObjCType:@encode(__typeof__(a2))]; \
      if ([a1encoded isEqualToValue:a2encoded]) { \
        NSString *_expression = [NSString stringWithFormat:@"((%s) != (%s))", #a1, #a2]; \
        [self failWithException:([NSException failureInCondition:_expression \
                         isTrue:NO \
                         inFile:[NSString stringWithUTF8String:__FILE__] \
                         atLine:__LINE__ \
                withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)])]; \
      }\
    } \
  } \
  @catch (id anException) { \
    [self failWithException:[NSException failureInRaise:[NSString stringWithFormat:@"(%s) != (%s)", #a1, #a2] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
            withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]; \
  } \
} while(0)








#define STAssertNotEqualObjects(a1, a2, description, ...) \
do { \
  @try {\
    id a1value = (a1); \
    id a2value = (a2); \
    if ( (strcmp(@encode(__typeof__(a1value)), @encode(id)) == 0) && \
         (strcmp(@encode(__typeof__(a2value)), @encode(id)) == 0) && \
         (![(id)a1value isEqual:(id)a2value]) ) continue; \
    [self failWithException:([NSException failureInEqualityBetweenObject:a1value \
                  andObject:a2value \
                     inFile:[NSString stringWithUTF8String:__FILE__] \
                     atLine:__LINE__ \
            withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)])]; \
  }\
  @catch (id anException) {\
    [self failWithException:([NSException failureInRaise:[NSString stringWithFormat:@"(%s) != (%s)", #a1, #a2] \
                                               exception:anException \
                                                  inFile:[NSString stringWithUTF8String:__FILE__] \
                                                  atLine:__LINE__ \
                                         withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)])]; \
  }\
} while(0)









#define STAssertOperation(a1, a2, op, description, ...) \
do { \
  @try { \
    if (strcmp(@encode(__typeof__(a1)), @encode(__typeof__(a2)))) { \
      [self failWithException:[NSException failureInFile:[NSString stringWithUTF8String:__FILE__] \
                                                  atLine:__LINE__ \
                                         withDescription:@"Type mismatch -- %@", STComposeString(description, ##__VA_ARGS__)]]; \
    } else { \
      __typeof__(a1) a1value = (a1); \
      __typeof__(a2) a2value = (a2); \
      if (!(a1value op a2value)) { \
        double a1DoubleValue = a1value; \
        double a2DoubleValue = a2value; \
        NSString *_expression = [NSString stringWithFormat:@"(%s (%lg) %s %s (%lg))", #a1, a1DoubleValue, #op, #a2, a2DoubleValue]; \
        [self failWithException:([NSException failureInCondition:_expression \
                         isTrue:NO \
                         inFile:[NSString stringWithUTF8String:__FILE__] \
                         atLine:__LINE__ \
                withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)])]; \
      } \
    } \
  } \
  @catch (id anException) { \
    [self failWithException:[NSException \
             failureInRaise:[NSString stringWithFormat:@"(%s) %s (%s)", #a1, #op, #a2] \
                  exception:anException \
                     inFile:[NSString stringWithUTF8String:__FILE__] \
                     atLine:__LINE__ \
            withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]; \
  } \
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
  @try { \
    id a1value = (a1); \
    id a2value = (a2); \
    if (a1value == a2value) continue; \
    if ([a1value isKindOfClass:[NSString class]] && \
        [a2value isKindOfClass:[NSString class]] && \
        [a1value compare:a2value options:0] == NSOrderedSame) continue; \
     [self failWithException:[NSException failureInEqualityBetweenObject:a1value \
                                                               andObject:a2value \
                                                                  inFile:[NSString stringWithUTF8String:__FILE__] \
                                                                  atLine:__LINE__ \
                                                         withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]; \
  } \
  @catch (id anException) { \
    [self failWithException:[NSException failureInRaise:[NSString stringWithFormat:@"(%s) == (%s)", #a1, #a2] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
                                        withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]; \
  } \
} while(0)












#define STAssertNotEqualStrings(a1, a2, description, ...) \
do { \
  @try { \
    id a1value = (a1); \
    id a2value = (a2); \
    if ([a1value isKindOfClass:[NSString class]] && \
        [a2value isKindOfClass:[NSString class]] && \
        [a1value compare:a2value options:0] != NSOrderedSame) continue; \
     [self failWithException:[NSException failureInEqualityBetweenObject:a1value \
                                                               andObject:a2value \
                                                                  inFile:[NSString stringWithUTF8String:__FILE__] \
                                                                  atLine:__LINE__ \
                                                         withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]; \
  } \
  @catch (id anException) { \
    [self failWithException:[NSException failureInRaise:[NSString stringWithFormat:@"(%s) != (%s)", #a1, #a2] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
                                        withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]; \
  } \
} while(0)








#define STAssertEqualCStrings(a1, a2, description, ...) \
do { \
  @try { \
    const char* a1value = (a1); \
    const char* a2value = (a2); \
    if (a1value == a2value) continue; \
    if (strcmp(a1value, a2value) == 0) continue; \
    [self failWithException:[NSException failureInEqualityBetweenObject:[NSString stringWithUTF8String:a1value] \
                                                              andObject:[NSString stringWithUTF8String:a2value] \
                                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                                 atLine:__LINE__ \
                                                        withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]; \
  } \
  @catch (id anException) { \
    [self failWithException:[NSException failureInRaise:[NSString stringWithFormat:@"(%s) == (%s)", #a1, #a2] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
                                        withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]; \
  } \
} while(0)








#define STAssertNotEqualCStrings(a1, a2, description, ...) \
do { \
  @try { \
    const char* a1value = (a1); \
    const char* a2value = (a2); \
    if (strcmp(a1value, a2value) != 0) continue; \
    [self failWithException:[NSException failureInEqualityBetweenObject:[NSString stringWithUTF8String:a1value] \
                                                              andObject:[NSString stringWithUTF8String:a2value] \
                                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                                 atLine:__LINE__ \
                                                        withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]; \
  } \
  @catch (id anException) { \
    [self failWithException:[NSException failureInRaise:[NSString stringWithFormat:@"(%s) != (%s)", #a1, #a2] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
                                        withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]; \
  } \
} while(0)















#define STInternalAssertEqualGLKVectorsOrMatrices(a1, a2, accuracy, description, ...) \
do { \
  @try { \
    if (strcmp(@encode(__typeof__(a1)), @encode(__typeof__(a2)))) { \
      [self failWithException:[NSException failureInFile:[NSString stringWithUTF8String:__FILE__] \
                                                                                 atLine:__LINE__ \
                                                                        withDescription:@"Type mismatch -- %@", STComposeString(description, ##__VA_ARGS__)]]; \
    } else { \
      __typeof__(a1) a1GLKValue = (a1); \
      __typeof__(a2) a2GLKValue = (a2); \
      __typeof__(accuracy) accuracyvalue = (accuracy); \
      float *a1FloatValue = ((float*)&a1GLKValue); \
      float *a2FloatValue = ((float*)&a2GLKValue); \
      for (size_t i = 0; i < sizeof(__typeof__(a1)) / sizeof(float); ++i) { \
        float a1value = a1FloatValue[i]; \
        float a2value = a2FloatValue[i]; \
        if (STAbsoluteDifference(a1value, a2value) > accuracyvalue) { \
          NSMutableArray *strings = [NSMutableArray arrayWithCapacity:sizeof(a1) / sizeof(float)]; \
          NSString *string; \
          for (size_t j = 0; j < sizeof(__typeof__(a1)) / sizeof(float); ++j) { \
            string = [NSString stringWithFormat:@"(%0.3f == %0.3f)", a1FloatValue[j], a2FloatValue[j]]; \
            [strings addObject:string]; \
          } \
          string = [strings componentsJoinedByString:@", "]; \
          NSString *desc = STComposeString(description, ##__VA_ARGS__); \
          desc = [NSString stringWithFormat:@"%@ With Accuracy %0.3f: %@", string, (float)accuracyvalue, desc]; \
          [self failWithException:[NSException failureInFile:[NSString stringWithUTF8String:__FILE__] \
                                                      atLine:__LINE__ \
                                             withDescription:@"%@", desc]]; \
          break; \
        } \
      } \
    } \
  } \
  @catch (id anException) { \
    [self failWithException:[NSException failureInRaise:[NSString stringWithFormat:@"(%s) == (%s)", #a1, #a2] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
                                        withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]; \
  } \
} while(0)

#define STAssertEqualGLKVectors(a1, a2, accuracy, description, ...) \
     STInternalAssertEqualGLKVectorsOrMatrices(a1, a2, accuracy, description, ##__VA_ARGS__)

#define STAssertEqualGLKMatrices(a1, a2, accuracy, description, ...) \
     STInternalAssertEqualGLKVectorsOrMatrices(a1, a2, accuracy, description, ##__VA_ARGS__)

#define STAssertEqualGLKQuaternions(a1, a2, accuracy, description, ...) \
     STInternalAssertEqualGLKVectorsOrMatrices(a1, a2, accuracy, description, ##__VA_ARGS__)

#if GTM_IPHONE_SDK && !GTM_IPHONE_USE_SENTEST











#define STAssertEqualObjects(a1, a2, description, ...) \
do { \
  @try { \
    id a1value = (a1); \
    id a2value = (a2); \
    if (a1value == a2value) continue; \
    if ((strcmp(@encode(__typeof__(a1value)), @encode(id)) == 0) && \
        (strcmp(@encode(__typeof__(a2value)), @encode(id)) == 0) && \
        [(id)a1value isEqual:(id)a2value]) continue; \
    [self failWithException:[NSException failureInEqualityBetweenObject:a1value \
                                                              andObject:a2value \
                                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                                 atLine:__LINE__ \
                                                        withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]; \
  } \
  @catch (id anException) { \
    [self failWithException:[NSException failureInRaise:[NSString stringWithFormat:@"(%s) == (%s)", #a1, #a2] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
                                        withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]; \
  } \
} while(0)










#define STAssertEquals(a1, a2, description, ...) \
do { \
  @try { \
    if (strcmp(@encode(__typeof__(a1)), @encode(__typeof__(a2)))) { \
      [self failWithException:[NSException failureInFile:[NSString stringWithUTF8String:__FILE__] \
                                                                                 atLine:__LINE__ \
                                                                        withDescription:@"Type mismatch -- %@", STComposeString(description, ##__VA_ARGS__)]]; \
    } else { \
      __typeof__(a1) a1value = (a1); \
      __typeof__(a2) a2value = (a2); \
      NSValue *a1encoded = [NSValue value:&a1value withObjCType:@encode(__typeof__(a1))]; \
      NSValue *a2encoded = [NSValue value:&a2value withObjCType:@encode(__typeof__(a2))]; \
      if (![a1encoded isEqualToValue:a2encoded]) { \
        [self failWithException:[NSException failureInEqualityBetweenValue:a1encoded \
                                                                  andValue:a2encoded \
                                                              withAccuracy:nil \
                                                                    inFile:[NSString stringWithUTF8String:__FILE__] \
                                                                    atLine:__LINE__ \
                                                           withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]; \
      } \
    } \
  } \
  @catch (id anException) { \
    [self failWithException:[NSException failureInRaise:[NSString stringWithFormat:@"(%s) == (%s)", #a1, #a2] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
                                        withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]; \
  } \
} while(0)

#define STAbsoluteDifference(left,right) (MAX(left,right)-MIN(left,right))














#define STAssertEqualsWithAccuracy(a1, a2, accuracy, description, ...) \
do { \
  @try { \
    if (strcmp(@encode(__typeof__(a1)), @encode(__typeof__(a2)))) { \
      [self failWithException:[NSException failureInFile:[NSString stringWithUTF8String:__FILE__] \
                                                                                 atLine:__LINE__ \
                                                                        withDescription:@"Type mismatch -- %@", STComposeString(description, ##__VA_ARGS__)]]; \
    } else { \
      __typeof__(a1) a1value = (a1); \
      __typeof__(a2) a2value = (a2); \
      __typeof__(accuracy) accuracyvalue = (accuracy); \
      if (STAbsoluteDifference(a1value, a2value) > accuracyvalue) { \
              NSValue *a1encoded = [NSValue value:&a1value withObjCType:@encode(__typeof__(a1))]; \
              NSValue *a2encoded = [NSValue value:&a2value withObjCType:@encode(__typeof__(a2))]; \
              NSValue *accuracyencoded = [NSValue value:&accuracyvalue withObjCType:@encode(__typeof__(accuracy))]; \
              [self failWithException:[NSException failureInEqualityBetweenValue:a1encoded \
                                                                        andValue:a2encoded \
                                                                    withAccuracy:accuracyencoded \
                                                                          inFile:[NSString stringWithUTF8String:__FILE__] \
                                                                          atLine:__LINE__ \
                                                                 withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]; \
      } \
    } \
  } \
  @catch (id anException) { \
    [self failWithException:[NSException failureInRaise:[NSString stringWithFormat:@"(%s) == (%s)", #a1, #a2] \
                                                                         exception:anException \
                                                                            inFile:[NSString stringWithUTF8String:__FILE__] \
                                                                            atLine:__LINE__ \
                                                                   withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]; \
  } \
} while(0)








#define STFail(description, ...) \
[self failWithException:[NSException failureInFile:[NSString stringWithUTF8String:__FILE__] \
                                            atLine:__LINE__ \
                                   withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]









#define STAssertNil(a1, description, ...) \
do { \
  @try { \
    id a1value = (a1); \
    if (a1value != nil) { \
      NSString *_a1 = [NSString stringWithUTF8String:#a1]; \
      NSString *_expression = [NSString stringWithFormat:@"((%@) == nil)", _a1]; \
      [self failWithException:[NSException failureInCondition:_expression \
                                                       isTrue:NO \
                                                       inFile:[NSString stringWithUTF8String:__FILE__] \
                                                       atLine:__LINE__ \
                                              withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]; \
    } \
  } \
  @catch (id anException) { \
    [self failWithException:[NSException failureInRaise:[NSString stringWithFormat:@"(%s) == nil fails", #a1] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
                                        withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]; \
  } \
} while(0)








#define STAssertNotNil(a1, description, ...) \
do { \
  @try { \
    id a1value = (a1); \
    if (a1value == nil) { \
      NSString *_a1 = [NSString stringWithUTF8String:#a1]; \
      NSString *_expression = [NSString stringWithFormat:@"((%@) != nil)", _a1]; \
      [self failWithException:[NSException failureInCondition:_expression \
                                                       isTrue:NO \
                                                       inFile:[NSString stringWithUTF8String:__FILE__] \
                                                       atLine:__LINE__ \
                                              withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]; \
    } \
  } \
  @catch (id anException) { \
    [self failWithException:[NSException failureInRaise:[NSString stringWithFormat:@"(%s) != nil fails", #a1] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
                                        withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]; \
  } \
} while(0)








#define STAssertTrue(expr, description, ...) \
do { \
  BOOL _evaluatedExpression = (expr); \
  if (!_evaluatedExpression) { \
    NSString *_expression = [NSString stringWithUTF8String:#expr]; \
    [self failWithException:[NSException failureInCondition:_expression \
                                                     isTrue:NO \
                                                     inFile:[NSString stringWithUTF8String:__FILE__] \
                                                     atLine:__LINE__ \
                                            withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]; \
  } \
} while (0)









#define STAssertTrueNoThrow(expr, description, ...) \
do { \
  @try { \
    BOOL _evaluatedExpression = (expr); \
    if (!_evaluatedExpression) { \
      NSString *_expression = [NSString stringWithUTF8String:#expr]; \
      [self failWithException:[NSException failureInCondition:_expression \
                                                       isTrue:NO \
                                                       inFile:[NSString stringWithUTF8String:__FILE__] \
                                                       atLine:__LINE__ \
                                              withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]; \
    } \
  } \
  @catch (id anException) { \
    [self failWithException:[NSException failureInRaise:[NSString stringWithFormat:@"(%s) ", #expr] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
                                        withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]; \
  } \
} while (0)








#define STAssertFalse(expr, description, ...) \
do { \
  BOOL _evaluatedExpression = (expr); \
  if (_evaluatedExpression) { \
    NSString *_expression = [NSString stringWithUTF8String:#expr]; \
    [self failWithException:[NSException failureInCondition:_expression \
                                                     isTrue:YES \
                                                     inFile:[NSString stringWithUTF8String:__FILE__] \
                                                     atLine:__LINE__ \
                                            withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]; \
  } \
} while (0)









#define STAssertFalseNoThrow(expr, description, ...) \
do { \
  @try { \
    BOOL _evaluatedExpression = (expr); \
    if (_evaluatedExpression) { \
      NSString *_expression = [NSString stringWithUTF8String:#expr]; \
      [self failWithException:[NSException failureInCondition:_expression \
                                                       isTrue:YES \
                                                       inFile:[NSString stringWithUTF8String:__FILE__] \
                                                       atLine:__LINE__ \
                                              withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]; \
    } \
  } \
  @catch (id anException) { \
    [self failWithException:[NSException failureInRaise:[NSString stringWithFormat:@"!(%s) ", #expr] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
                                        withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]; \
  } \
} while (0)








#define STAssertThrows(expr, description, ...) \
do { \
  @try { \
    (expr); \
  } \
  @catch (id anException) { \
    continue; \
  } \
  [self failWithException:[NSException failureInRaise:[NSString stringWithUTF8String:#expr] \
                                            exception:nil \
                                               inFile:[NSString stringWithUTF8String:__FILE__] \
                                               atLine:__LINE__ \
                                      withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]; \
} while (0)










#define STAssertThrowsSpecific(expr, specificException, description, ...) \
do { \
  @try { \
    (expr); \
  } \
  @catch (specificException *anException) { \
    continue; \
  } \
  @catch (id anException) { \
    NSString *_descrip = STComposeString(@"(Expected exception: %@) %@", NSStringFromClass([specificException class]), description); \
    [self failWithException:[NSException failureInRaise:[NSString stringWithUTF8String:#expr] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
                                        withDescription:@"%@", STComposeString(_descrip, ##__VA_ARGS__)]]; \
                                            continue; \
  } \
  NSString *_descrip = STComposeString(@"(Expected exception: %@) %@", NSStringFromClass([specificException class]), description); \
  [self failWithException:[NSException failureInRaise:[NSString stringWithUTF8String:#expr] \
                                            exception:nil \
                                               inFile:[NSString stringWithUTF8String:__FILE__] \
                                               atLine:__LINE__ \
                                      withDescription:@"%@", STComposeString(_descrip, ##__VA_ARGS__)]]; \
} while (0)














#define STAssertThrowsSpecificNamed(expr, specificException, aName, description, ...) \
do { \
  @try { \
    (expr); \
  } \
  @catch (specificException *anException) { \
    if ([aName isEqualToString:[anException name]]) continue; \
    NSString *_descrip = STComposeString(@"(Expected exception: %@ (name: %@)) %@", NSStringFromClass([specificException class]), aName, description); \
    [self failWithException: \
      [NSException failureInRaise:[NSString stringWithUTF8String:#expr] \
                        exception:anException \
                           inFile:[NSString stringWithUTF8String:__FILE__] \
                           atLine:__LINE__ \
                  withDescription:@"%@", STComposeString(_descrip, ##__VA_ARGS__)]]; \
    continue; \
  } \
  @catch (id anException) { \
    NSString *_descrip = STComposeString(@"(Expected exception: %@) %@", NSStringFromClass([specificException class]), description); \
    [self failWithException: \
      [NSException failureInRaise:[NSString stringWithUTF8String:#expr] \
                        exception:anException \
                           inFile:[NSString stringWithUTF8String:__FILE__] \
                           atLine:__LINE__ \
                  withDescription:@"%@", STComposeString(_descrip, ##__VA_ARGS__)]]; \
    continue; \
  } \
  NSString *_descrip = STComposeString(@"(Expected exception: %@) %@", NSStringFromClass([specificException class]), description); \
  [self failWithException: \
    [NSException failureInRaise:[NSString stringWithUTF8String:#expr] \
                      exception:nil \
                         inFile:[NSString stringWithUTF8String:__FILE__] \
                         atLine:__LINE__ \
                withDescription:@"%@", STComposeString(_descrip, ##__VA_ARGS__)]]; \
} while (0)








#define STAssertNoThrow(expr, description, ...) \
do { \
  @try { \
    (expr); \
  } \
  @catch (id anException) { \
    [self failWithException:[NSException failureInRaise:[NSString stringWithUTF8String:#expr] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
                                        withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]; \
  } \
} while (0)










#define STAssertNoThrowSpecific(expr, specificException, description, ...) \
do { \
  @try { \
    (expr); \
  } \
  @catch (specificException *anException) { \
    [self failWithException:[NSException failureInRaise:[NSString stringWithUTF8String:#expr] \
                                              exception:anException \
                                                 inFile:[NSString stringWithUTF8String:__FILE__] \
                                                 atLine:__LINE__ \
                                        withDescription:@"%@", STComposeString(description, ##__VA_ARGS__)]]; \
  } \
  @catch (id anythingElse) { \
    ; \
  } \
} while (0)














#define STAssertNoThrowSpecificNamed(expr, specificException, aName, description, ...) \
do { \
  @try { \
    (expr); \
  } \
  @catch (specificException *anException) { \
    if ([aName isEqualToString:[anException name]]) { \
      NSString *_descrip = STComposeString(@"(Expected exception: %@ (name: %@)) %@", NSStringFromClass([specificException class]), aName, description); \
      [self failWithException: \
        [NSException failureInRaise:[NSString stringWithUTF8String:#expr] \
                          exception:anException \
                             inFile:[NSString stringWithUTF8String:__FILE__] \
                             atLine:__LINE__ \
                    withDescription:@"%@", STComposeString(_descrip, ##__VA_ARGS__)]]; \
    } \
    continue; \
  } \
  @catch (id anythingElse) { \
    ; \
  } \
} while (0)



@interface NSException (GTMSenTestAdditions)
+ (NSException *)failureInFile:(NSString *)filename
                        atLine:(int)lineNumber
               withDescription:(NSString *)formatString, ... NS_FORMAT_FUNCTION(3, 4);
+ (NSException *)failureInCondition:(NSString *)condition
                             isTrue:(BOOL)isTrue
                             inFile:(NSString *)filename
                             atLine:(int)lineNumber
                    withDescription:(NSString *)formatString, ... NS_FORMAT_FUNCTION(5, 6);
+ (NSException *)failureInEqualityBetweenObject:(id)left
                                      andObject:(id)right
                                         inFile:(NSString *)filename
                                         atLine:(int)lineNumber
                                withDescription:(NSString *)formatString, ... NS_FORMAT_FUNCTION(5, 6);
+ (NSException *)failureInEqualityBetweenValue:(NSValue *)left
                                      andValue:(NSValue *)right
                                  withAccuracy:(NSValue *)accuracy
                                        inFile:(NSString *)filename
                                        atLine:(int) ineNumber
                               withDescription:(NSString *)formatString, ... NS_FORMAT_FUNCTION(6, 7);
+ (NSException *)failureInRaise:(NSString *)expression
                         inFile:(NSString *)filename
                         atLine:(int)lineNumber
                withDescription:(NSString *)formatString, ... NS_FORMAT_FUNCTION(4, 5);
+ (NSException *)failureInRaise:(NSString *)expression
                      exception:(NSException *)exception
                         inFile:(NSString *)filename
                         atLine:(int)lineNumber
                withDescription:(NSString *)formatString, ... NS_FORMAT_FUNCTION(5, 6);
@end



@protocol SenTestCase
+ (id)testCaseWithInvocation:(NSInvocation *)anInvocation;
- (id)initWithInvocation:(NSInvocation *)anInvocation;
- (void)setUp;
- (void)invokeTest;
- (void)tearDown;
- (void)performTest;
- (void)failWithException:(NSException*)exception;
- (NSInvocation *)invocation;
- (SEL)selector;
+ (NSArray *)testInvocations;
@end

@interface SenTestCase : NSObject<SenTestCase> {
 @private
  NSInvocation *invocation_;
}
@end

GTM_EXTERN NSString *const SenTestFailureException;

GTM_EXTERN NSString *const SenTestFilenameKey;
GTM_EXTERN NSString *const SenTestLineNumberKey;

#endif 




@interface GTMTestCase : SenTestCase





















+ (BOOL)isAbstractTestCase;

@end
