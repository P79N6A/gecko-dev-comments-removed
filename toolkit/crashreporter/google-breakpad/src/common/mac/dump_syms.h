




































#include <Foundation/Foundation.h>
#include <mach-o/loader.h>
#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <vector>

#include "common/byte_cursor.h"
#include "common/mac/macho_reader.h"
#include "common/module.h"

namespace google_breakpad {

class DumpSymbols {
 public:
  DumpSymbols() 
      : input_pathname_(),
        object_filename_(), 
        contents_(),
        selected_object_file_(),
        selected_object_name_() { }
  ~DumpSymbols() {
    [input_pathname_ release];
    [object_filename_ release];
    [contents_ release];
  }

  
  
  
  
  
  
  
  
  bool Read(NSString *filename);

  
  
  
  
  
  
  
  
  
  bool SetArchitecture(cpu_type_t cpu_type, cpu_subtype_t cpu_subtype);
  
  
  
  
  
  
  
  
  
  bool SetArchitecture(const std::string &arch_name);
  
  
  
  
  
  
  
  
  const struct fat_arch *AvailableArchitectures(size_t *count) {
    *count = object_files_.size();
    if (object_files_.size() > 0)
      return &object_files_[0];
    return NULL;
  }

  
  
  
  bool WriteSymbolFile(FILE *stream);

 private:
  
  class DumperLineToModule;
  class LoadCommandDumper;

  
  std::string Identifier();

  
  
  
  bool ReadDwarf(google_breakpad::Module *module,
                 const mach_o::Reader &macho_reader,
                 const mach_o::SectionMap &dwarf_sections) const;

  
  
  
  
  
  bool ReadCFI(google_breakpad::Module *module,
               const mach_o::Reader &macho_reader,
               const mach_o::Section &section,
               bool eh_frame) const;

  
  
  NSString *input_pathname_;

  
  
  
  
  NSString *object_filename_;

  
  NSData *contents_;

  
  
  
  
  vector<struct fat_arch> object_files_;

  
  
  const struct fat_arch *selected_object_file_;

  
  
  
  
  string selected_object_name_;
};

}  
