

































#import <Foundation/Foundation.h>

@interface DumpSymbols : NSObject {
 @protected
  NSString *sourcePath_;              
  NSString *architecture_;            
  NSMutableDictionary *addresses_;    
  NSMutableDictionary *sources_;      
  NSMutableArray *cppAddresses_;      
  NSMutableDictionary *headers_;      
  NSMutableDictionary *lastFunctionStartDict_; 
  NSMutableDictionary *sectionNumbers_; 
}

- (id)initWithContentsOfFile:(NSString *)machoFile;

- (NSArray *)availableArchitectures;




- (BOOL)setArchitecture:(NSString *)architecture;
- (NSString *)architecture;


- (BOOL)writeSymbolFile:(NSString *)symbolFilePath;

@end
