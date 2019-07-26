


















































#import <Foundation/Foundation.h>
#import "GTMDefines.h"


@protocol GTMLogWriter, GTMLogFormatter, GTMLogFilter;
























































































































































@interface GTMLogger : NSObject {
 @private
  id<GTMLogWriter> writer_;
  id<GTMLogFormatter> formatter_;
  id<GTMLogFilter> filter_;
}









+ (id)sharedLogger;



+ (void)setSharedLogger:(GTMLogger *)logger;







+ (id)standardLogger;


+ (id)standardLoggerWithStderr;



+ (id)standardLoggerWithStdoutAndStderr;




+ (id)standardLoggerWithPath:(NSString *)path;



+ (id)loggerWithWriter:(id<GTMLogWriter>)writer
             formatter:(id<GTMLogFormatter>)formatter
                filter:(id<GTMLogFilter>)filter;






+ (id)logger;




- (id)initWithWriter:(id<GTMLogWriter>)writer
           formatter:(id<GTMLogFormatter>)formatter
              filter:(id<GTMLogFilter>)filter;






- (void)logDebug:(NSString *)fmt, ... NS_FORMAT_FUNCTION(1, 2);

- (void)logInfo:(NSString *)fmt, ... NS_FORMAT_FUNCTION(1, 2);

- (void)logError:(NSString *)fmt, ... NS_FORMAT_FUNCTION(1, 2);

- (void)logAssert:(NSString *)fmt, ... NS_FORMAT_FUNCTION(1, 2);








- (id<GTMLogWriter>)writer;
- (void)setWriter:(id<GTMLogWriter>)writer;




- (id<GTMLogFormatter>)formatter;
- (void)setFormatter:(id<GTMLogFormatter>)formatter;



- (id<GTMLogFilter>)filter;
- (void)setFilter:(id<GTMLogFilter>)filter;

@end  




@interface GTMLogger (GTMLoggerMacroHelpers)
- (void)logFuncDebug:(const char *)func msg:(NSString *)fmt, ...
  NS_FORMAT_FUNCTION(2, 3);
- (void)logFuncInfo:(const char *)func msg:(NSString *)fmt, ...
  NS_FORMAT_FUNCTION(2, 3);
- (void)logFuncError:(const char *)func msg:(NSString *)fmt, ...
  NS_FORMAT_FUNCTION(2, 3);
- (void)logFuncAssert:(const char *)func msg:(NSString *)fmt, ...
  NS_FORMAT_FUNCTION(2, 3);
@end  



#ifndef GTMLoggerInfo




#define GTMLoggerDebug(...)  \
  [[GTMLogger sharedLogger] logFuncDebug:__func__ msg:__VA_ARGS__]
#define GTMLoggerInfo(...)   \
  [[GTMLogger sharedLogger] logFuncInfo:__func__ msg:__VA_ARGS__]
#define GTMLoggerError(...)  \
  [[GTMLogger sharedLogger] logFuncError:__func__ msg:__VA_ARGS__]
#define GTMLoggerAssert(...) \
  [[GTMLogger sharedLogger] logFuncAssert:__func__ msg:__VA_ARGS__]



#ifndef DEBUG
#undef GTMLoggerDebug
#define GTMLoggerDebug(...) do {} while(0)
#endif

#endif  


typedef enum {
  kGTMLoggerLevelUnknown,
  kGTMLoggerLevelDebug,
  kGTMLoggerLevelInfo,
  kGTMLoggerLevelError,
  kGTMLoggerLevelAssert,
} GTMLoggerLevel;







@protocol GTMLogWriter <NSObject>

- (void)logMessage:(NSString *)msg level:(GTMLoggerLevel)level;
@end  






@interface NSFileHandle (GTMFileHandleLogWriter) <GTMLogWriter>


+ (id)fileHandleForLoggingAtPath:(NSString *)path mode:(mode_t)mode;
@end  












@interface NSArray (GTMArrayCompositeLogWriter) <GTMLogWriter>
@end  









@interface GTMLogger (GTMLoggerLogWriter) <GTMLogWriter>
@end  







@protocol GTMLogFormatter <NSObject>


- (NSString *)stringForFunc:(NSString *)func
                 withFormat:(NSString *)fmt
                     valist:(va_list)args
                      level:(GTMLoggerLevel)level NS_FORMAT_FUNCTION(2, 0);
@end  





@interface GTMLogBasicFormatter : NSObject <GTMLogFormatter>


- (NSString *)prettyNameForFunc:(NSString *)func;

@end  






@interface GTMLogStandardFormatter : GTMLogBasicFormatter {
 @private
  NSDateFormatter *dateFormatter_;  
  NSString *pname_;
  pid_t pid_;
}
@end  







@protocol GTMLogFilter <NSObject>

- (BOOL)filterAllowsMessage:(NSString *)msg level:(GTMLoggerLevel)level;
@end  







@interface GTMLogLevelFilter : NSObject <GTMLogFilter>
@end  




@interface GTMLogNoFilter : NSObject <GTMLogFilter>
@end  




@interface GTMLogAllowedLevelFilter : NSObject <GTMLogFilter> {
 @private
  NSIndexSet *allowedLevels_;
}
@end



@interface GTMLogMininumLevelFilter : GTMLogAllowedLevelFilter


- (id)initWithMinimumLevel:(GTMLoggerLevel)level;

@end




@interface GTMLogMaximumLevelFilter : GTMLogAllowedLevelFilter


- (id)initWithMaximumLevel:(GTMLoggerLevel)level;

@end



@interface GTMLogger (PrivateMethods)

- (void)logInternalFunc:(const char *)func
                 format:(NSString *)fmt
                 valist:(va_list)args
                  level:(GTMLoggerLevel)level NS_FORMAT_FUNCTION(2, 0);

@end

