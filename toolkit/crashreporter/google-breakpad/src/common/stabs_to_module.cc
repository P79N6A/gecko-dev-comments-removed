
































#include <assert.h>
#include <cxxabi.h>
#include <stdarg.h>
#include <stdio.h>

#include <algorithm>

#include "common/stabs_to_module.h"
#include "common/using_std_string.h"

namespace google_breakpad {



static string Demangle(const string &mangled) {
  int status = 0;
  char *demangled = abi::__cxa_demangle(mangled.c_str(), NULL, NULL, &status);
  if (status == 0 && demangled != NULL) {
    string str(demangled);
    free(demangled);
    return str;
  }
  return string(mangled);
}

StabsToModule::~StabsToModule() {
  
  for (vector<Module::Function *>::const_iterator func_it = functions_.begin();
       func_it != functions_.end(); func_it++)
    delete *func_it;
  
  delete current_function_;
}

bool StabsToModule::StartCompilationUnit(const char *name, uint64_t address,
                                         const char *build_directory) {
  assert(!in_compilation_unit_);
  in_compilation_unit_ = true;
  current_source_file_name_ = name;
  current_source_file_ = module_->FindFile(name);
  comp_unit_base_address_ = address;
  boundaries_.push_back(static_cast<Module::Address>(address));
  return true;
}

bool StabsToModule::EndCompilationUnit(uint64_t address) {
  assert(in_compilation_unit_);
  in_compilation_unit_ = false;
  comp_unit_base_address_ = 0;
  current_source_file_ = NULL;
  current_source_file_name_ = NULL;
  if (address)
    boundaries_.push_back(static_cast<Module::Address>(address));
  return true;
}

bool StabsToModule::StartFunction(const string &name,
                                  uint64_t address) {
  assert(!current_function_);
  Module::Function *f = new Module::Function;
  f->name = Demangle(name);
  f->address = address;
  f->size = 0;           
  f->parameter_size = 0; 
  current_function_ = f;
  boundaries_.push_back(static_cast<Module::Address>(address));
  return true;
}

bool StabsToModule::EndFunction(uint64_t address) {
  assert(current_function_);
  
  
  
  
  if (current_function_->address >= comp_unit_base_address_)
    functions_.push_back(current_function_);
  else
    delete current_function_;
  current_function_ = NULL;
  if (address)
    boundaries_.push_back(static_cast<Module::Address>(address));
  return true;
}

bool StabsToModule::Line(uint64_t address, const char *name, int number) {
  assert(current_function_);
  assert(current_source_file_);
  if (name != current_source_file_name_) {
    current_source_file_ = module_->FindFile(name);
    current_source_file_name_ = name;
  }
  Module::Line line;
  line.address = address;
  line.size = 0;  
  line.file = current_source_file_;
  line.number = number;
  current_function_->lines.push_back(line);
  return true;
}

bool StabsToModule::Extern(const string &name, uint64_t address) {
  Module::Extern *ext = new Module::Extern;
  
  
  if (name.compare(0, 3, "__Z") == 0) {
    ext->name = Demangle(name.substr(1));
  } else if (name[0] == '_') {
    ext->name = name.substr(1);
  } else {
    ext->name = name;
  }
  ext->address = address;
  module_->AddExtern(ext);
  return true;
}

void StabsToModule::Warning(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
}

void StabsToModule::Finalize() {
  
  sort(boundaries_.begin(), boundaries_.end());
  
  sort(functions_.begin(), functions_.end(),
       Module::Function::CompareByAddress);

  for (vector<Module::Function *>::const_iterator func_it = functions_.begin();
       func_it != functions_.end();
       func_it++) {
    Module::Function *f = *func_it;
    
    vector<Module::Address>::const_iterator boundary
        = std::upper_bound(boundaries_.begin(), boundaries_.end(), f->address);
    if (boundary != boundaries_.end())
      f->size = *boundary - f->address;
    else
      
      
      
      
      
      f->size = kFallbackSize;

    
    if (!f->lines.empty()) {
      stable_sort(f->lines.begin(), f->lines.end(),
                  Module::Line::CompareByAddress);
      vector<Module::Line>::iterator last_line = f->lines.end() - 1;
      for (vector<Module::Line>::iterator line_it = f->lines.begin();
           line_it != last_line; line_it++)
        line_it[0].size = line_it[1].address - line_it[0].address;
      
      last_line->size = (f->address + f->size) - last_line->address;
    }
  }
  
  
  module_->AddFunctions(functions_.begin(), functions_.end());
  functions_.clear();
}

} 
