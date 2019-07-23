
































#import <Foundation/Foundation.h>
#include <mach-o/loader.h>
#include "common/mac/dwarf/dwarf2reader.h"



typedef map<string, dwarf2reader::SectionMap *> ArchSectionMap;

@interface DumpSymbols : NSObject {
 @protected
  NSString *sourcePath_;              
  NSString *architecture_;            
  NSMutableDictionary *addresses_;    
  NSMutableSet *functionAddresses_;   
  NSMutableDictionary *sources_;      
  NSMutableDictionary *headers_;      
  NSMutableDictionary *sectionData_; 
  uint32_t   lastStartAddress_;
  ArchSectionMap *sectionsForArch_;
}

- (id)initWithContentsOfFile:(NSString *)machoFile;

- (NSArray *)availableArchitectures;




- (BOOL)setArchitecture:(NSString *)architecture;
- (NSString *)architecture;


- (BOOL)writeSymbolFile:(NSString *)symbolFilePath;

@end

@interface MachSection : NSObject {
 @protected
  struct section *sect_;
  uint32_t sectionNumber_;
}
- (id)initWithMachSection:(struct section *)sect andNumber:(uint32_t)sectionNumber;
- (struct section*)sectionPointer;
- (uint32_t)sectionNumber;

@end
