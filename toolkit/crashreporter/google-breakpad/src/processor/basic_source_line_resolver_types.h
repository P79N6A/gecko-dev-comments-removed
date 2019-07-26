



































#ifndef PROCESSOR_BASIC_SOURCE_LINE_RESOLVER_TYPES_H__
#define PROCESSOR_BASIC_SOURCE_LINE_RESOLVER_TYPES_H__

#include <map>
#include <string>

#include "common/scoped_ptr.h"
#include "google_breakpad/processor/basic_source_line_resolver.h"
#include "processor/source_line_resolver_base_types.h"

#include "processor/address_map-inl.h"
#include "processor/range_map-inl.h"
#include "processor/contained_range_map-inl.h"

#include "processor/linked_ptr.h"
#include "google_breakpad/processor/stack_frame.h"
#include "processor/cfi_frame_info.h"
#include "processor/windows_frame_info.h"

namespace google_breakpad {

struct
BasicSourceLineResolver::Function : public SourceLineResolverBase::Function {
  Function(const string &function_name,
           MemAddr function_address,
           MemAddr code_size,
           int set_parameter_size) : Base(function_name,
                                          function_address,
                                          code_size,
                                          set_parameter_size),
                                     lines() { }
  RangeMap< MemAddr, linked_ptr<Line> > lines;
 private:
  typedef SourceLineResolverBase::Function Base;
};


class BasicSourceLineResolver::Module : public SourceLineResolverBase::Module {
 public:
  explicit Module(const string &name) : name_(name) { }
  virtual ~Module() { }

  
  
  virtual bool LoadMapFromMemory(char *memory_buffer);

  
  
  virtual void LookupAddress(StackFrame *frame) const;

  
  
  
  
  
  virtual WindowsFrameInfo *FindWindowsFrameInfo(const StackFrame *frame) const;

  
  
  
  
  virtual CFIFrameInfo *FindCFIFrameInfo(const StackFrame *frame) const;

 private:
  
  friend class BasicSourceLineResolver;
  friend class ModuleComparer;
  friend class ModuleSerializer;

  typedef std::map<int, string> FileMap;

  
  bool ParseFile(char *file_line);

  
  Function* ParseFunction(char *function_line);

  
  Line* ParseLine(char *line_line);

  
  
  bool ParsePublicSymbol(char *public_line);

  
  
  bool ParseStackInfo(char *stack_info_line);

  
  bool ParseCFIFrameInfo(char *stack_info_line);

  string name_;
  FileMap files_;
  RangeMap< MemAddr, linked_ptr<Function> > functions_;
  AddressMap< MemAddr, linked_ptr<PublicSymbol> > public_symbols_;

  
  
  
  
  ContainedRangeMap< MemAddr, linked_ptr<WindowsFrameInfo> >
    windows_frame_info_[WindowsFrameInfo::STACK_INFO_LAST];

  
  
  
  
  

  
  
  
  RangeMap<MemAddr, string> cfi_initial_rules_;

  
  
  
  
  
  std::map<MemAddr, string> cfi_delta_rules_;
};

}  

#endif  
