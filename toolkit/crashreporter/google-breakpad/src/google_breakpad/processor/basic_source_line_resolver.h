




































#ifndef GOOGLE_BREAKPAD_PROCESSOR_BASIC_SOURCE_LINE_RESOLVER_H__
#define GOOGLE_BREAKPAD_PROCESSOR_BASIC_SOURCE_LINE_RESOLVER_H__

#include <map>
#include <string>

#include "common/using_std_string.h"
#include "google_breakpad/processor/source_line_resolver_base.h"

namespace google_breakpad {

using std::map;

class BasicSourceLineResolver : public SourceLineResolverBase {
 public:
  BasicSourceLineResolver();
  virtual ~BasicSourceLineResolver() { }

  using SourceLineResolverBase::LoadModule;
  using SourceLineResolverBase::LoadModuleUsingMapBuffer;
  using SourceLineResolverBase::LoadModuleUsingMemoryBuffer;
  using SourceLineResolverBase::ShouldDeleteMemoryBufferAfterLoadModule;
  using SourceLineResolverBase::UnloadModule;
  using SourceLineResolverBase::HasModule;
  using SourceLineResolverBase::FillSourceLineInfo;
  using SourceLineResolverBase::FindWindowsFrameInfo;
  using SourceLineResolverBase::FindCFIFrameInfo;

 private:
  
  friend class BasicModuleFactory;
  friend class ModuleComparer;
  friend class ModuleSerializer;
  template<class> friend class SimpleSerializer;

  
  struct Function;
  
  class Module;

  
  BasicSourceLineResolver(const BasicSourceLineResolver&);
  void operator=(const BasicSourceLineResolver&);
};

}  

#endif  
