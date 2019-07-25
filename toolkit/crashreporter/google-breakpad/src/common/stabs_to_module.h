




































#ifndef COMMON_LINUX_DUMP_STABS_H__
#define COMMON_LINUX_DUMP_STABS_H__

#include <stdint.h>

#include <string>
#include <vector>

#include "common/module.h"
#include "common/stabs_reader.h"

namespace google_breakpad {

using std::string;
using std::vector;







class StabsToModule: public google_breakpad::StabsHandler {
 public:
  
  
  StabsToModule(Module *module) :
      module_(module),
      in_compilation_unit_(false),
      comp_unit_base_address_(0),
      current_function_(NULL),
      current_source_file_(NULL),
      current_source_file_name_(NULL) { }
  ~StabsToModule();

  
  bool StartCompilationUnit(const char *name, uint64_t address,
                            const char *build_directory);
  bool EndCompilationUnit(uint64_t address);
  bool StartFunction(const string &name, uint64_t address);
  bool EndFunction(uint64_t address);
  bool Line(uint64_t address, const char *name, int number);
  void Warning(const char *format, ...);

  
  
  
  
  
  
  
  void Finalize();

 private:

  
  
  static const uint64_t kFallbackSize = 0x10000000;

  
  Module *module_;

  
  
  
  
  
  
  
  vector<Module::Function *> functions_;

  
  
  
  vector<Module::Address> boundaries_;

  
  
  
  bool in_compilation_unit_;

  
  
  
  
  Module::Address comp_unit_base_address_;

  
  Module::Function *current_function_;

  
  Module::File *current_source_file_;

  
  
  
  
  const char *current_source_file_name_;
};

} 

#endif 
