







































#include <stdio.h>

#include <map>
#include <string>

#include "google_breakpad/common/breakpad_types.h"
#include "google_breakpad/processor/source_line_resolver_base.h"
#include "google_breakpad/processor/stack_frame.h"
#include "processor/cfi_frame_info.h"
#include "processor/windows_frame_info.h"

#ifndef PROCESSOR_SOURCE_LINE_RESOLVER_BASE_TYPES_H__
#define PROCESSOR_SOURCE_LINE_RESOLVER_BASE_TYPES_H__

namespace google_breakpad {

class SourceLineResolverBase::AutoFileCloser {
 public:
  explicit AutoFileCloser(FILE *file) : file_(file) {}
  ~AutoFileCloser() {
    if (file_)
      fclose(file_);
  }

 private:
  FILE *file_;
};

struct SourceLineResolverBase::Line {
  Line() { }
  Line(MemAddr addr, MemAddr code_size, int file_id, int source_line)
      : address(addr)
      , size(code_size)
      , source_file_id(file_id)
      , line(source_line) { }

  MemAddr address;
  MemAddr size;
  int32_t source_file_id;
  int32_t line;
};

struct SourceLineResolverBase::Function {
  Function() { }
  Function(const string &function_name,
           MemAddr function_address,
           MemAddr code_size,
           int set_parameter_size)
      : name(function_name), address(function_address), size(code_size),
        parameter_size(set_parameter_size) { }

  string name;
  MemAddr address;
  MemAddr size;

  
  int32_t parameter_size;
};

struct SourceLineResolverBase::PublicSymbol {
  PublicSymbol() { }
  PublicSymbol(const string& set_name,
               MemAddr set_address,
               int set_parameter_size)
      : name(set_name),
        address(set_address),
        parameter_size(set_parameter_size) {}

  string name;
  MemAddr address;

  
  
  
  int32_t parameter_size;
};

class SourceLineResolverBase::Module {
 public:
  virtual ~Module() { };
  
  
  
  virtual bool LoadMapFromMemory(char *memory_buffer) = 0;

  
  
  virtual void LookupAddress(StackFrame *frame) const = 0;

  
  
  
  
  
  virtual WindowsFrameInfo *
  FindWindowsFrameInfo(const StackFrame *frame) const = 0;

  
  
  
  
  virtual CFIFrameInfo *FindCFIFrameInfo(const StackFrame *frame) const = 0;
 protected:
  virtual bool ParseCFIRuleSet(const string &rule_set,
                               CFIFrameInfo *frame_info) const;
};

}  

#endif  
