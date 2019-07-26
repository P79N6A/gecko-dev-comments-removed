











































#ifndef GOOGLE_BREAKPAD_PROCESSOR_FAST_SOURCE_LINE_RESOLVER_H__
#define GOOGLE_BREAKPAD_PROCESSOR_FAST_SOURCE_LINE_RESOLVER_H__

#include <map>
#include <string>

#include "google_breakpad/processor/source_line_resolver_base.h"

namespace google_breakpad {

using std::map;

class FastSourceLineResolver : public SourceLineResolverBase {
 public:
  FastSourceLineResolver();
  virtual ~FastSourceLineResolver() { }

  using SourceLineResolverBase::FillSourceLineInfo;
  using SourceLineResolverBase::FindCFIFrameInfo;
  using SourceLineResolverBase::FindWindowsFrameInfo;
  using SourceLineResolverBase::HasModule;
  using SourceLineResolverBase::LoadModule;
  using SourceLineResolverBase::LoadModuleUsingMapBuffer;
  using SourceLineResolverBase::LoadModuleUsingMemoryBuffer;
  using SourceLineResolverBase::UnloadModule;

 private:
  
  friend class ModuleComparer;
  friend class ModuleSerializer;
  friend class FastModuleFactory;

  
  
  struct Line;
  struct Function;
  struct PublicSymbol;
  class Module;

  
  static WindowsFrameInfo CopyWFI(const char *raw_memory);

  
  
  
  virtual bool ShouldDeleteMemoryBufferAfterLoadModule();

  
  FastSourceLineResolver(const FastSourceLineResolver&);
  void operator=(const FastSourceLineResolver&);
};

}  

#endif  
