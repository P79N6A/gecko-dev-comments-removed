

































#import <Cocoa/Cocoa.h>

@interface HTTPMultipartUpload : NSObject {
 @protected
  NSURL *url_;                  
  NSDictionary *parameters_;    
  NSMutableDictionary *files_;  
  NSString *boundary_;          
  NSHTTPURLResponse *response_; 
}

- (id)initWithURL:(NSURL *)url;

- (NSURL *)URL;

- (void)setParameters:(NSDictionary *)parameters;
- (NSDictionary *)parameters;

- (void)addFileAtPath:(NSString *)path name:(NSString *)name;
- (void)addFileContents:(NSData *)data name:(NSString *)name;
- (NSDictionary *)files;


- (NSData *)send:(NSError **)error;
- (NSHTTPURLResponse *)response;

@end
