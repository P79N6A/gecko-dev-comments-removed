







































#ifndef GOOGLE_BREAKPAD_PROCESSOR_SOURCE_LINE_RESOLVER_BASE_H__
#define GOOGLE_BREAKPAD_PROCESSOR_SOURCE_LINE_RESOLVER_BASE_H__

#include <map>
#include <string>

#include "google_breakpad/processor/source_line_resolver_interface.h"

namespace google_breakpad {

using std::map;




class ModuleFactory;

class SourceLineResolverBase : public SourceLineResolverInterface {
 public:
  
  
  
  
  
  static bool ReadSymbolFile(char **symbol_data, const string &file_name);

 protected:
  
  SourceLineResolverBase(ModuleFactory *module_factory);
  virtual ~SourceLineResolverBase();

  
  virtual bool LoadModule(const CodeModule *module, const string &map_file);
  virtual bool LoadModuleUsingMapBuffer(const CodeModule *module,
                                        const string &map_buffer);
  virtual bool LoadModuleUsingMemoryBuffer(const CodeModule *module,
                                           char *memory_buffer);
  virtual bool ShouldDeleteMemoryBufferAfterLoadModule();
  virtual void UnloadModule(const CodeModule *module);
  virtual bool HasModule(const CodeModule *module);
  virtual void FillSourceLineInfo(StackFrame *frame);
  virtual WindowsFrameInfo *FindWindowsFrameInfo(const StackFrame *frame);
  virtual CFIFrameInfo *FindCFIFrameInfo(const StackFrame *frame);

  
  struct Line;
  struct Function;
  struct PublicSymbol;
  struct CompareString {
    bool operator()(const string &s1, const string &s2) const;
  };
  
  class Module;
  class AutoFileCloser;

  
  typedef map<string, Module*, CompareString> ModuleMap;
  ModuleMap *modules_;

  
  typedef std::map<string, char*, CompareString> MemoryMap;
  MemoryMap *memory_buffers_;

  
  ModuleFactory *module_factory_;

 private:
  
  friend class ModuleFactory;

  
  SourceLineResolverBase(const SourceLineResolverBase&);
  void operator=(const SourceLineResolverBase&);
};

}  

#endif  
