

































#include <Foundation/Foundation.h>

#import "common/mac/GTMDefines.h"

#define kClientIdPreferenceKey @"clientid"

extern NSString *const kGoogleServerType;
extern NSString *const kSocorroServerType;
extern NSString *const kDefaultServerType;

@interface Uploader : NSObject {
 @private
  NSMutableDictionary *parameters_;        
  NSData *minidumpContents_;               
  NSData *logFileData_;                    
                                           
  NSMutableDictionary *serverDictionary_;  
                                           
                                           
                                           
  NSMutableDictionary *socorroDictionary_; 
                                           
  NSMutableDictionary *googleDictionary_;  
                                           
  NSMutableDictionary *extraServerVars_;   
                                           
                                           
                                           
                                           
}

- (id)initWithConfigFile:(const char *)configFile;

- (id)initWithConfig:(NSDictionary *)config;

- (NSMutableDictionary *)parameters;

- (void)report;


- (void)uploadData:(NSData *)data name:(NSString *)name;



- (void)addServerParameter:(id)value forKey:(NSString *)key;

@end
